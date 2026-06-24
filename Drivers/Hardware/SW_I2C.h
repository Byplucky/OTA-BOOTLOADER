#ifndef __SW_I2C_H__
#define __SW_I2C_H__
#include<stdint.h>

void SWI2C_W_SCL(uint8_t BitValue);
void SWI2C_W_SDA(uint8_t BitValue);
uint8_t SWI2C_R_SDA(void);

void SWI2C_Start(void);

void SWI2C_Stop(void);

void SWI2C_SendByte(uint8_t Byte);

uint8_t SWI2C_ReceiveByte(void);

void SWI2C_SendAck(uint8_t Ack);

uint8_t SWI2C_ReceiveAck(void);

#endif
