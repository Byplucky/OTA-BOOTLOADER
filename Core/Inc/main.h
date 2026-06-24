/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Bootloader.h"
#include "ESP8266.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define KEY_Pin GPIO_PIN_14
#define KEY_GPIO_Port GPIOB
#define KEY_EXTI_IRQn EXTI15_10_IRQn
#define I2C_SCL_Pin GPIO_PIN_6
#define I2C_SCL_GPIO_Port GPIOB
#define I2C_SDA_Pin GPIO_PIN_7
#define I2C_SDA_GPIO_Port GPIOB
#define SPI_SS_Pin GPIO_PIN_9
#define SPI_SS_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define OTA_FLAG_UPDATA 0x11223344
#define OTA_Status_UpdataA 0x00000001
#define XMODEM_SEND_C      0x00000002
#define XMODEN_DATA_Decode 0x00000004

#define OTA_CTL_SIZE   sizeof(OTA_CtlTypeDef)
typedef enum {
	OTA_APP = 0,
	OTA_Backup
}UpdataType;

typedef struct {
	uint32_t OTA_Flag;
	uint32_t OTA_UpdataSize[2];
	uint32_t HttpCRC;
	uint32_t Version;
}OTA_CtlTypeDef;
#define RX_SIZE 2048    
#define RX_MAX  256    //
#define NUM     10     //
extern OTA_CtlTypeDef OTA_CtlStructure;
extern ManifestTypeDef ManifestHandle;
extern uint32_t OTA_StatusFlag;
extern uint8_t Rx_Buffer[RX_SIZE];
extern uint8_t Rx_OTABuffer[RX_SIZE];
extern OTA_RecvTypeDef OTA_RecvHandle;
typedef struct {
	uint8_t OTA_UpdataABuffer[1024];
	UpdataType UpdataEnum;
	uint32_t  XmodemTimer;
	uint32_t  XmodemPocketNum;
	uint16_t  XmodemCRC;
}OTA_UpdataATypeDef;
extern OTA_UpdataATypeDef OTA_UpdataA;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
