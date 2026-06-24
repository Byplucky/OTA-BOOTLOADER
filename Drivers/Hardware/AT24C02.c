#include "gpio.h"
#include "SW_I2C.h"
#include "AT24C02.h"
#include "main.h"
#include "string.h"


AT24C02_Status AT24C02_WaitBusy(void){
	uint16_t timeout = 10000;
    uint8_t  busy;
      do {
          SWI2C_Start();
          SWI2C_SendByte(AT24C02_WRITE);          
          busy = SWI2C_ReceiveAck();        
          SWI2C_Stop();                     
          timeout--;
      } while (busy && timeout);
	SWI2C_Stop();
	  return (timeout == 0)? AT24C02_TIMEOUT:AT24C02_OK;
}

AT24C02_Status AT24C02_WriteByte(uint8_t addr,uint8_t data){
	 uint8_t cmd = AT24C02_WRITE;
	 SWI2C_Start();
	 SWI2C_SendByte(cmd);
	 if (SWI2C_ReceiveAck()) { SWI2C_Stop(); return AT24C02_WRITE_ERROR; }
	 SWI2C_SendByte(addr);
	 if (SWI2C_ReceiveAck()) { SWI2C_Stop(); return AT24C02_WRITE_ERROR; }
	 SWI2C_SendByte(data);
	 if (SWI2C_ReceiveAck()) { SWI2C_Stop(); return AT24C02_WRITE_ERROR; }
	 SWI2C_Stop();
	 AT24C02_WaitBusy();
	 return AT24C02_WRITE_OK;
}
AT24C02_Status AT24C02_WritePage(uint8_t addr,uint8_t *data,uint8_t len){
	uint8_t cmd = AT24C02_WRITE;
	if(len > (AT24C02_PAGE_SIZE - addr%AT24C02_PAGE_SIZE)){
			return AT24C02_WRITE_ERROR;
	}
	 SWI2C_Start();
	 SWI2C_SendByte(cmd);
	 if (SWI2C_ReceiveAck()) { SWI2C_Stop(); return AT24C02_WRITE_ERROR; }
	 SWI2C_SendByte(addr);
	 if (SWI2C_ReceiveAck()) { SWI2C_Stop(); return AT24C02_WRITE_ERROR; }
	 for(uint8_t i = 0; i<len; i++){
	 SWI2C_SendByte(*(data++));
	 if (SWI2C_ReceiveAck()) { SWI2C_Stop(); return AT24C02_WRITE_ERROR; }
	 }
	 SWI2C_Stop();
	 AT24C02_WaitBusy();
	 return AT24C02_WRITE_OK;
}

AT24C02_Status AT24C02_ReadByte(uint8_t addr,uint8_t *data){
	uint8_t cmd = AT24C02_READ;
	SWI2C_Start();
	SWI2C_SendByte(AT24C02_WRITE);
	if (SWI2C_ReceiveAck()) { SWI2C_Stop(); return AT24C02_READ_ERROR; }
	SWI2C_SendByte(addr);
	if (SWI2C_ReceiveAck()) { SWI2C_Stop(); return AT24C02_READ_ERROR; }
	SWI2C_Start();
	SWI2C_SendByte(cmd);
	if (SWI2C_ReceiveAck()) { SWI2C_Stop(); return AT24C02_READ_ERROR; }
	*data = SWI2C_ReceiveByte();
	SWI2C_SendAck(1);
	SWI2C_Stop();
	return AT24C02_READ_OK;
}


AT24C02_Status AT24C02_ReadMulitByte(uint8_t addr,uint8_t *data ,uint8_t len){
	if(addr + len > 256){
			return AT24C02_READ_ERROR;
	}
    uint8_t cmd = AT24C02_READ;
	SWI2C_Start();
	SWI2C_SendByte(AT24C02_WRITE);
	if (SWI2C_ReceiveAck()) { SWI2C_Stop(); return AT24C02_READ_ERROR; }
	SWI2C_SendByte(addr);
	if (SWI2C_ReceiveAck()) { SWI2C_Stop(); return AT24C02_READ_ERROR; }
	SWI2C_Start();
	SWI2C_SendByte(cmd);
	if (SWI2C_ReceiveAck()) { SWI2C_Stop(); return AT24C02_READ_ERROR; }
	for(uint8_t i = 0 ; i < len ; i ++){
	*data = SWI2C_ReceiveByte();
	if(i == len -1){SWI2C_SendAck(1);}
	else{SWI2C_SendAck(0);}
	data++;
	}
	SWI2C_Stop();
	return AT24C02_READ_OK;

}

void AT24C02_READ_OTAInf(void){
	memset(&OTA_CtlStructure,0,OTA_CTL_SIZE);
	uint8_t FlagBuffer[4];
	uint8_t SizeBuffer[8];
	uint8_t VersionBuffer[4];
	uint8_t CRCBuffer[4];
	AT24C02_ReadMulitByte(AT24C02_FLAG_ADDR,FlagBuffer,4);
	AT24C02_ReadMulitByte(AT24C02_OTA_UPDATASIZE_ADDR ,SizeBuffer,8);
	AT24C02_ReadMulitByte(AT24C02_OTA_VERSION_ADDR ,VersionBuffer,4);
	AT24C02_ReadMulitByte(AT24C02_OTA_CRC_ADDR ,CRCBuffer,4);
	OTA_CtlStructure.OTA_Flag = ((uint32_t)FlagBuffer[0] <<24) |
								((uint32_t)FlagBuffer[1] <<16) |
								((uint32_t)FlagBuffer[2] << 8) |
								 (uint32_t)FlagBuffer[3];
	OTA_CtlStructure.OTA_UpdataSize[0] =  ((uint32_t)SizeBuffer[0] <<24) |
										  ((uint32_t)SizeBuffer[1] <<16) |
										  ((uint32_t)SizeBuffer[2] <<8)  |
										   (uint32_t)SizeBuffer[3];
	OTA_CtlStructure.OTA_UpdataSize[1] = ((uint32_t)SizeBuffer[4] <<24) |
										 ((uint32_t)SizeBuffer[5] <<16) |
										 ((uint32_t)SizeBuffer[6] <<8)  |
										 (uint32_t)SizeBuffer[7];
	OTA_CtlStructure.Version = ((uint32_t)VersionBuffer[0] <<24) |
							   ((uint32_t)VersionBuffer[1] <<16) |
							   ((uint32_t)VersionBuffer[2] <<8)  |
							    (uint32_t)VersionBuffer[3];
	OTA_CtlStructure.HttpCRC = ((uint32_t)CRCBuffer[0] <<24) |
							   ((uint32_t)CRCBuffer[1] <<16) |
							   ((uint32_t)CRCBuffer[2] <<8)  |
							    (uint32_t)CRCBuffer[3];
}

void AT24C02_Write_OTAInf(void){
	uint8_t i ;
	uint8_t FlagBuffer[4];
	uint8_t SizeBuffer[8];
	uint8_t VersionBuffer[4];
	uint8_t CRCBuffer[4];
	FlagBuffer[0] = (uint8_t)(OTA_CtlStructure.OTA_Flag >> 24);
	FlagBuffer[1] = (uint8_t)(OTA_CtlStructure.OTA_Flag >> 16);
	FlagBuffer[2] = (uint8_t)(OTA_CtlStructure.OTA_Flag >> 8);
	FlagBuffer[3] = (uint8_t)(OTA_CtlStructure.OTA_Flag);
	SizeBuffer[0] = (uint8_t)(OTA_CtlStructure.OTA_UpdataSize[0] >> 24);
	SizeBuffer[1] = (uint8_t)(OTA_CtlStructure.OTA_UpdataSize[0] >> 16);
	SizeBuffer[2] = (uint8_t)(OTA_CtlStructure.OTA_UpdataSize[0] >> 8);	
	SizeBuffer[3] = (uint8_t)(OTA_CtlStructure.OTA_UpdataSize[0]);
	SizeBuffer[4] = (uint8_t)(OTA_CtlStructure.OTA_UpdataSize[1] >> 24);
	SizeBuffer[5] = (uint8_t)(OTA_CtlStructure.OTA_UpdataSize[1] >> 16);
	SizeBuffer[6] = (uint8_t)(OTA_CtlStructure.OTA_UpdataSize[1] >> 8);
	SizeBuffer[7] = (uint8_t)(OTA_CtlStructure.OTA_UpdataSize[1]);
	VersionBuffer[0] = (uint8_t)(OTA_CtlStructure.Version >> 24);
	VersionBuffer[1] = (uint8_t)(OTA_CtlStructure.Version >> 16);
	VersionBuffer[2] = (uint8_t)(OTA_CtlStructure.Version >> 8);
	VersionBuffer[3] = (uint8_t)(OTA_CtlStructure.Version);
	CRCBuffer[0] = (uint8_t)(OTA_CtlStructure.HttpCRC >> 24);
	CRCBuffer[1] = (uint8_t)(OTA_CtlStructure.HttpCRC >> 16);
	CRCBuffer[2] = (uint8_t)(OTA_CtlStructure.HttpCRC >> 8);
	CRCBuffer[3] = (uint8_t)(OTA_CtlStructure.HttpCRC);
	for(i = 0; i < sizeof(OTA_CtlStructure.OTA_Flag) ; i ++ ){
	AT24C02_WriteByte(AT24C02_FLAG_ADDR+i,FlagBuffer[i]);
	}
    for(i = 0; i < sizeof(OTA_CtlStructure.OTA_UpdataSize); i ++){
	AT24C02_WriteByte(AT24C02_OTA_UPDATASIZE_ADDR + i ,SizeBuffer[i]);
	}
	for(i = 0; i < sizeof(OTA_CtlStructure.Version); i ++ ){
	AT24C02_WriteByte(AT24C02_OTA_VERSION_ADDR+i,VersionBuffer[i]);
	}
	for(i = 0; i < sizeof(OTA_CtlStructure.HttpCRC) ; i ++ ){
	AT24C02_WriteByte(AT24C02_OTA_CRC_ADDR+i,CRCBuffer[i]);
	}
}
