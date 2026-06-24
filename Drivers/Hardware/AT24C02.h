#ifndef __AT24C02_H__
#define __AT24C02_H__

#define AT24C02_DEIVCE_ID 0xA0
#define AT24C02_WRITE  AT24C02_DEIVCE_ID
#define AT24C02_READ  AT24C02_DEIVCE_ID|0x01
#define AT24C02_PAGE_SIZE 8 //byte
#define AT24C02_PAGE_NUM  32
#define AT24C02_MAX_ADDR  0xff

#define AT24C02_FLAG_ADDR             0x00
#define AT24C02_OTA_UPDATASIZE_ADDR   0x08
#define AT24C02_OTA_VERSION_ADDR      0x10
#define AT24C02_OTA_CRC_ADDR          0x14

typedef enum {
	AT24C02_WRITE_OK = 0,
	AT24C02_WRITE_ERROR ,
	AT24C02_READ_OK ,
	AT24C02_READ_ERROR,
	AT24C02_OK,
	AT24C02_TIMEOUT
}AT24C02_Status;

AT24C02_Status AT24C02_WriteByte(uint8_t addr,uint8_t data);
AT24C02_Status AT24C02_WritePage(uint8_t addr,uint8_t *data,uint8_t len);

AT24C02_Status AT24C02_ReadByte(uint8_t addr,uint8_t *data);

AT24C02_Status AT24C02_ReadMulitByte(uint8_t addr,uint8_t *data ,uint8_t len);
void AT24C02_READ_OTAInf(void);
void AT24C02_Write_OTAInf(void);

#endif



