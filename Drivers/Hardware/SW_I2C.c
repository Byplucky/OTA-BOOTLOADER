#include "gpio.h"

static void SWI2C_Delay(void)
{
	volatile uint8_t i;
	for (i = 0; i < 10; i++);
}

void SWI2C_W_SCL(uint8_t BitValue){
	HAL_GPIO_WritePin(I2C_SCL_GPIO_Port,I2C_SCL_Pin,(GPIO_PinState)BitValue);
	SWI2C_Delay();
}
void SWI2C_W_SDA(uint8_t BitValue){
	HAL_GPIO_WritePin(I2C_SDA_GPIO_Port,I2C_SDA_Pin,(GPIO_PinState)BitValue);
	SWI2C_Delay();
}
uint8_t SWI2C_R_SDA(void){
	uint8_t BitValue;
	BitValue = HAL_GPIO_ReadPin(I2C_SDA_GPIO_Port,I2C_SDA_Pin);
	SWI2C_Delay();
	return BitValue;
}

void SWI2C_Start(void){
	SWI2C_W_SDA(1);
	SWI2C_W_SCL(1);
	SWI2C_W_SDA(0);
	SWI2C_W_SCL(0);

}

void SWI2C_Stop(void){
	SWI2C_W_SDA(0);
	SWI2C_W_SCL(1);
	SWI2C_W_SDA(1);

}

void SWI2C_SendByte(uint8_t Byte){
	uint8_t i;
	for(i=0;i<8;i++){
	SWI2C_W_SDA(Byte & (0x80 >> i));
	SWI2C_W_SCL(1);
	SWI2C_W_SCL(0);		
	}
}

uint8_t SWI2C_ReceiveByte(void){
	uint16_t i, Byte = 0x00;
	SWI2C_W_SDA(1);

	for(i=0;i<8;i++){
	SWI2C_W_SCL(1);
	if(SWI2C_R_SDA() == 1){
		Byte |= (0x80 >> i);
	}
	SWI2C_W_SCL(0);
}
	return Byte;
}

void SWI2C_SendAck(uint8_t Ack){
	SWI2C_W_SDA(Ack);
	SWI2C_W_SCL(1);
	SWI2C_W_SCL(0);		
}

uint8_t SWI2C_ReceiveAck(void){
	uint8_t Ack;
	SWI2C_W_SDA(1);
	SWI2C_W_SCL(1);
	Ack = SWI2C_R_SDA();
	SWI2C_W_SCL(0);
	return Ack;
}



