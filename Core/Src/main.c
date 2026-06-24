/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "W25Q64.h"
#include "FMC.h"
#include "AT24C02.h"
#include "Bootloader.h"
#include "ESP8266.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
OTA_CtlTypeDef OTA_CtlStructure;
uint8_t Rx_Buffer[RX_SIZE];
uint8_t Rx_OTABuffer[RX_SIZE];
UCB_CB U0CB;
uint32_t OTA_StatusFlag;
OTA_UpdataATypeDef OTA_UpdataA;
OTA_RecvTypeDef OTA_RecvHandle;
ManifestTypeDef ManifestHandle;
extern uint8_t ESP_Rx_Buf[ESP8266_UART_RX_BUF_SIZE];
extern uint8_t ESP_Tx_Buf[ESP8266_UART_TX_BUF_SIZE];
extern uint8_t esp_RxFlag;
extern uint32_t Rx_CNT;
uint8_t KEYFlag;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void UCB_Init(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
    uint8_t i;
   UCB_Init();
   HAL_UARTEx_ReceiveToIdle_DMA(&huart1,Rx_Buffer,RX_SIZE);
   HAL_UARTEx_ReceiveToIdle_DMA(&huart2,ESP_Rx_Buf,ESP8266_UART_RX_BUF_SIZE);
   AT24C02_READ_OTAInf();
   printf("begin connect...\r\n");
   if(ESP8266_Init()!= ESP8266_ERROR)
   {
   printf("connect success!\r\n");
   if(ESP8266_GetJson(&ManifestHandle)){
	   if(ESP8266_GetBinAndWrite()){
	      ESP8266_WriteInfo();
     //   NVIC_SystemReset();	   
		   }
   }
   }
   else
	{
		printf("connect fail! \r\n");
	}
   BootLoader_OTAJudge();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {		
	  if(KEYFlag){
      HAL_Delay(20);                                       // 消抖
      if(HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == 0){   // 确认
          if(ESP8266_Reset()  != ESP8266_EOK){printf("reset fail\r\n!");}else{printf("reset success\r\n!");}
      }
      KEYFlag = 0;
  }
	HAL_Delay(10);
		if(U0CB.URxDataIn != U0CB.URxDataOut){
			BootLoader_Event(U0CB.URxDataOut->start,U0CB.URxDataOut->end - U0CB.URxDataOut->start+1);
			U0CB.URxDataOut++;
			if(U0CB.URxDataOut == U0CB.URxDataEnd){
			U0CB.URxDataOut = &U0CB.URxDataPtr[0];
			}
		}
		if(OTA_StatusFlag & XMODEM_SEND_C){
		      if(OTA_UpdataA.XmodemTimer > 100){
				printf("C");
				OTA_UpdataA.XmodemTimer = 0;
		      }
			  OTA_UpdataA.XmodemTimer ++;
		}
	  
	  
	  /* 判断更新标志位,从外部FLASH读取数据到内部FLASH*/
		if(OTA_StatusFlag & OTA_Status_UpdataA){
			printf("Size : %d Byte \r\n",OTA_CtlStructure.OTA_UpdataSize[OTA_UpdataA.UpdataEnum]);
			if(OTA_CtlStructure.OTA_UpdataSize[OTA_UpdataA.UpdataEnum] % 4 == 0){
				FMC_FlashPageErase(FMC_APP_START_PAGE,FMC_APP_PAGE_NUM);
				for( i = 0 ; i < OTA_CtlStructure.OTA_UpdataSize[OTA_UpdataA.UpdataEnum] / FMC_PAGE_SIZE ; i++){
					W25Q64_ReadBuffer(i*FMC_PAGE_SIZE + OTA_UpdataA.UpdataEnum * FMC_PAGE_SIZE *64 , 
									  OTA_UpdataA.OTA_UpdataABuffer,
									  FMC_PAGE_SIZE);
					FMC_FlashWrite(FMC_APP_ADDR+i*FMC_PAGE_SIZE,OTA_UpdataA.OTA_UpdataABuffer,FMC_PAGE_SIZE);
					printf("translate %d datapocket\r\n",i+1);
				}
				if(OTA_CtlStructure.OTA_UpdataSize[OTA_UpdataA.UpdataEnum] % FMC_PAGE_SIZE != 0){
					W25Q64_ReadBuffer(i*FMC_PAGE_SIZE + OTA_UpdataA.UpdataEnum * FMC_PAGE_SIZE *64 , 
					OTA_UpdataA.OTA_UpdataABuffer,
					OTA_CtlStructure.OTA_UpdataSize[OTA_UpdataA.UpdataEnum] % FMC_PAGE_SIZE);
					FMC_FlashWrite(FMC_APP_ADDR+i*FMC_PAGE_SIZE,
								   OTA_UpdataA.OTA_UpdataABuffer,
								   OTA_CtlStructure.OTA_UpdataSize[OTA_UpdataA.UpdataEnum] % FMC_PAGE_SIZE);
					printf("translate %d datapocket size : %d byte\r\n" ,i+1,
											(OTA_CtlStructure.OTA_UpdataSize[OTA_UpdataA.UpdataEnum] % FMC_PAGE_SIZE));
				}
				if(OTA_UpdataA.UpdataEnum == OTA_APP){
					OTA_CtlStructure.OTA_Flag = 0;
					AT24C02_Write_OTAInf();
					printf("SUCCESS !");
					OTA_StatusFlag &= ~OTA_Status_UpdataA;
				}
			}
			else{
				printf("Size Error！\r\n");
				OTA_StatusFlag &= ~OTA_Status_UpdataA;
			}
		}  
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void UCB_Init(void){
	U0CB.URxDataIn = &U0CB.URxDataPtr[0];
	U0CB.URxDataOut = &U0CB.URxDataPtr[0];
	U0CB.URxDataEnd = &U0CB.URxDataPtr[NUM-1];
	U0CB.URxDataIn->start = Rx_Buffer;
	U0CB.URxCounter = 0;
}
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size){
	if(huart->Instance == USART1){
		uint16_t remain;
		U0CB.URxCounter += Size; //累加计数，计算总字节数
		U0CB.URxDataIn->end = &Rx_Buffer[U0CB.URxCounter-1]; 
		U0CB.URxDataIn ++ ;
		if(U0CB.URxDataIn == U0CB.URxDataEnd){  //判断是否到成员边界
		U0CB.URxDataIn = &U0CB.URxDataPtr[0];   
		}  
		if(RX_SIZE - U0CB.URxCounter >= RX_MAX){ //判断是否还有字节能够存放一个RX_MAX
		   U0CB.URxDataIn->start = &Rx_Buffer[U0CB.URxCounter]; //有则从当前计数值开始
			remain = RX_SIZE - U0CB.URxCounter;
		}
		else{
			U0CB.URxDataIn->start = Rx_Buffer;    //没有，则重新从头开始
			U0CB.URxCounter = 0;
			remain = RX_SIZE;
		}
		HAL_UARTEx_ReceiveToIdle_DMA(&huart1,U0CB.URxDataIn->start,remain); 
	
	}
	if(huart->Instance == USART2){
	 /* 多帧累加: ESP响应常分多个IDLE帧到达, 接在上次末尾后面, 不覆盖.
	  * Rx_CNT 记录已累计字节数, SendCmd 处理完一条命令后会清零重置. */
	 Rx_CNT += Size;
	 esp_RxFlag = 1;
	 if(Rx_CNT >= ESP8266_UART_RX_BUF_SIZE - 1){
		 Rx_CNT = 0;   /* 防越界, 满了从头开始 */
	 }
	 HAL_UARTEx_ReceiveToIdle_DMA(&huart2, ESP_Rx_Buf + Rx_CNT,
								   ESP8266_UART_RX_BUF_SIZE - Rx_CNT);

	}

}

int fputc(int ch, FILE *f){
	
	HAL_UART_Transmit(&huart1,(uint8_t *)&ch,1,HAL_MAX_DELAY);
	return ch;

}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == GPIO_PIN_14)
      KEYFlag = 1;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
