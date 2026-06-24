/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart2;

/* USER CODE BEGIN Private defines */


typedef struct {
	uint8_t *start;   //成员内起始指针 ，指向接收的起始字节
	uint8_t *end;	  //成员内结束指针，
}UCB_URxBuffptr;

typedef struct {
	uint16_t URxCounter;    //接收字节计数
	UCB_URxBuffptr URxDataPtr[NUM];  //接收缓冲成员结构体组
	UCB_URxBuffptr *URxDataIn;       //成员外输入指针，指向第几个成员
	UCB_URxBuffptr *URxDataOut;		 //成员外输出指针，
	UCB_URxBuffptr *URxDataEnd;		 //限制边界
}UCB_CB;
/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

