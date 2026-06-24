#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__
#include "W25Q64.h"
/* 固件下载累积器: 把零散字节流攒满 1024 写一页 W25Q64, 同时滚动算 CRC32 */
typedef struct {
	uint32_t flash_addr;    /* W25Q64 写入基址 */
	uint32_t total_size;    /* 期望总字节数 (manifest 的 size) */
	uint32_t recv_size;     /* 已累计接收字节数 */
	uint32_t expect_crc;    /* 期望 CRC32 (manifest 的 crc32) */
	uint32_t crc;           /* 滚动 CRC32 中间值 */
	uint16_t buf_len;       /* 1024 桶当前已填字节 */
	uint8_t  buffer[1024];  /* 累积桶, 满了刷一页 */
}OTA_RecvTypeDef;

void BootLoader_OTAJudge(void);
void Bootloader_JumpToApp(void);
void BootLoader_Event(uint8_t *data ,uint16_t len);
uint16_t Xmodem_CRC16(uint8_t *data, uint16_t len);

/* IEEE 802.3 CRC32 (反射多项式 0xEDB88320), 支持流式分段计算 */
void     OTA_CRC32_Init(uint32_t *crc);
void     OTA_CRC32_Update(uint32_t *crc, uint8_t *data, uint32_t len);
uint32_t OTA_CRC32_Final(uint32_t crc);

/* 固件累积器接口 (脱机可测) */
void OTA_RecvInit(OTA_RecvTypeDef *r, uint32_t flash_addr, uint32_t total_size, uint32_t expect_crc);
W25Q64_Status OTA_RecvFeed(OTA_RecvTypeDef *r, uint8_t *data, uint32_t len);
uint8_t  OTA_RecvFinish(OTA_RecvTypeDef *r);  /* 1=成功(CRC与长度都对), 0=失败 */

#endif



