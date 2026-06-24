#include "main.h"
#include <stdio.h>
#include "W25Q64.h"
#include "FMC.h"
#include "AT24C02.h"
#include "Bootloader.h"
#include "string.h"
typedef void (*OTA_LoadAPP)(void);

void Bootloader_JumpToApp(void){
     uint32_t app_sp  = *(volatile uint32_t *)(FMC_APP_ADDR);       // 取 App 栈顶 栈顶的地通常在0x20000000RAM地址
     uint32_t app_pc  = *(volatile uint32_t *)(FMC_APP_ADDR+ 4);   // 取 App 入口地址
     
	if((app_sp > 0x20000000)&&(app_sp < 0x20004fff)){
	  __disable_irq();
      HAL_RCC_DeInit();
      HAL_DeInit();
      SysTick->CTRL = 0;  SysTick->LOAD = 0;  SysTick->VAL = 0;

      __set_MSP(app_sp);

      OTA_LoadAPP app_entry = (OTA_LoadAPP)app_pc; //PC指向reset_handler
      app_entry();
	}else{
		printf("跳转APP区失败！\r\n");
	}
}

uint8_t BootLoader_EnterCli(uint16_t TimeOut){
	printf("Key w to Enter Cli Mode in %d ms \r\n",TimeOut*100);
	while(TimeOut -- ){
		
		HAL_Delay(100);
		if(Rx_Buffer[0] == 'w'){
		return 1;
		}
	}
	return 0;
}

void BootLoader_Info(void){
	printf("-----------Cli Mode -----------\r\n");
	printf("[1] 擦除A区 \r\n");
	printf("[2] 串口IAP下载程序到 A区 \r\n");
	printf("[3] 设置OTA版本号 \r\n");
	printf("[4] 查询OTA版本号 \r\n");
	printf("[5] 外部FLASH主程序下载到A区 \r\n");
	printf("[6] 外部FLASH备份程序下载到A区  \r\n");
	printf("[7] 重启  \r\n");
	printf("-----------Cli Mode -----------\r\n");
}
void BootLoader_OTAJudge(void){
	if(BootLoader_EnterCli(30) == 0){
		if(OTA_CtlStructure.OTA_Flag == OTA_FLAG_UPDATA){
		printf("updata! %08x\r\n",OTA_CtlStructure.OTA_Flag);
			OTA_StatusFlag |= OTA_Status_UpdataA;
			OTA_UpdataA.UpdataEnum = OTA_APP;
		}
		else{
		printf("go APP! %08x\r\n",OTA_CtlStructure.OTA_Flag);
		Bootloader_JumpToApp();
	       } 
	}else {
	BootLoader_Info();
	}
}

void BootLoader_Event(uint8_t *data ,uint16_t len){
	if(OTA_StatusFlag == 0){
		if(data[0] == 0x31 & len == 1){
			FMC_FlashPageErase(FMC_APP_START_PAGE,FMC_APP_PAGE_NUM);
			printf("Flash APP 区已擦除 !\r\n");
		}
		else if(data[0] == 0x32 & len == 1){
			printf("通过Xmodem更新bin文件到APP !\r\n");
			OTA_UpdataA.XmodemTimer = 0;
			OTA_StatusFlag |= XMODEM_SEND_C |XMODEN_DATA_Decode;
			OTA_UpdataA.XmodemPocketNum = 0;
		}	else if(data[0] == 0x33 & len == 1){

		}	else if(data[0] == 0x34 & len == 1){

		}	else if(data[0] == 0x35 & len == 1){

		}
		else if(data[0] == 0x37 & len == 1){
			printf("重启中...\r\n");
			HAL_Delay(100);
			NVIC_SystemReset();
		}
	}
	if(OTA_StatusFlag & XMODEN_DATA_Decode){
		/* 数据包: SOH(0x01) | 包号 | 255-包号 | 128字节数据 | CRC高 | CRC低 */
		if((data[0] == 0x01) && (len == 133)){
			OTA_StatusFlag &= ~XMODEM_SEND_C;            /* 收到首包,停止发 'C' */

			/* 1. 校验包头: data[1] + data[2] 必须 == 0xFF */
			if(((data[1] + data[2]) & 0xFF) != 0xFF){
				printf("\x15");                          /* 包头损坏, NACK */
				return;
			}

			/* 2. CRC 校验: Xmodem 为大端, 高字节在前 */
			OTA_UpdataA.XmodemCRC = Xmodem_CRC16(&data[3], 128);
			if(OTA_UpdataA.XmodemCRC != (uint16_t)(data[131] * 256 + data[132])){
				printf("\x15");                          /* CRC 错误, NACK 请求重发 */
				return;
			}

			/* 3. 包序号判断 (Xmodem 包号 1~255 循环) */
			if(data[1] == (uint8_t)(OTA_UpdataA.XmodemPocketNum & 0xFF)){
				printf("\x06");                          /* 重复包(上次ACK丢失), 重发ACK不重复写 */
				return;
			}
			if(data[1] != (uint8_t)((OTA_UpdataA.XmodemPocketNum + 1) & 0xFF)){
				printf("\x15");                          /* 包序号错乱, NACK */
				return;
			}

			/* 4. 正常包: 缓存到 RAM, 满 8 包(1KB)写入一页 Flash */
			OTA_UpdataA.XmodemPocketNum++;
			memcpy(&OTA_UpdataA.OTA_UpdataABuffer[((OTA_UpdataA.XmodemPocketNum - 1) % 8) * 128],
				   &data[3], 128);
			printf("\x06");                              /* ACK */
			if(OTA_UpdataA.XmodemPocketNum % 8 == 0){
				FMC_FlashWrite(FMC_APP_ADDR + (OTA_UpdataA.XmodemPocketNum / 8 - 1) * FMC_PAGE_SIZE,
							   OTA_UpdataA.OTA_UpdataABuffer, FMC_PAGE_SIZE);
			}
		}
		/* 传输结束: EOT(0x04) */
		if((data[0] == 0x04) && (len == 1)){
			/* flush 最后不足 8 包的残余数据 */
			if(OTA_UpdataA.XmodemPocketNum % 8 != 0){
				FMC_FlashWrite(FMC_APP_ADDR + (OTA_UpdataA.XmodemPocketNum / 8) * FMC_PAGE_SIZE,
							   OTA_UpdataA.OTA_UpdataABuffer,
							   (OTA_UpdataA.XmodemPocketNum % 8) * 128);
			}
			OTA_StatusFlag &= ~XMODEN_DATA_Decode;       /* 清除解码标志 */
			printf("\x06");                              /* 对 EOT 回 ACK */
			HAL_Delay(500);                              /* 等 ACK 发完再复位 */
			NVIC_SystemReset();
		}
	}
}

  uint16_t Xmodem_CRC16(uint8_t *data, uint16_t len)
  {
      uint16_t crc = 0x0000;
      uint16_t poly = 0x1021;

      for (uint16_t i = 0; i < len; i++)
      {
          crc ^= (uint16_t)data[i] << 8;

          for (uint8_t b = 0; b < 8; b++)
          {
              if (crc & 0x8000)
                  crc = (crc << 1) ^ poly;
              else
                  crc <<= 1;
          }
      }
      return crc;
  }

/* ============ IEEE 802.3 CRC32 (流式) ============ */
/* 多项式反射形式 0xEDB88320, 初值 0xFFFFFFFF, 末尾取反.
 * 等价于 Python zlib.crc32 / binascii.crc32, 服务器端务必用同一种. */

void OTA_CRC32_Init(uint32_t *crc)
{
	*crc = 0xFFFFFFFF;
}

void OTA_CRC32_Update(uint32_t *crc, uint8_t *data, uint32_t len)
{
	uint32_t c = *crc;
	for(uint32_t i = 0; i < len; i++){
		c ^= data[i];
		for(uint8_t b = 0; b < 8; b++){
			if(c & 1)
				c = (c >> 1) ^ 0xEDB88320;
			else
				c >>= 1;
		}
	}
	*crc = c;
}

uint32_t OTA_CRC32_Final(uint32_t crc)
{
	return crc ^ 0xFFFFFFFF;
}

/* ============ 固件下载累积器 ============ */
/* 数据流: ESP8266 收到的 body 字节 -> OTA_RecvFeed -> 攒满1024刷W25Q64 -> 滚动算CRC32
 * 调用顺序: OTA_RecvInit (一次) -> OTA_RecvFeed (多次) -> OTA_RecvFinish (一次) */

void OTA_RecvInit(OTA_RecvTypeDef *r, uint32_t flash_addr, uint32_t total_size, uint32_t expect_crc)
{
	r->flash_addr = flash_addr;
	r->total_size = total_size;
	r->expect_crc = expect_crc;
	r->recv_size  = 0;
	r->buf_len    = 0;
	OTA_CRC32_Init(&r->crc);

	/* 写前先擦. W25Q64 最小擦除单位 4KB 扇区, 按 total_size 向上取整擦够 */
	uint32_t sectors = (total_size + 4095) / 4096;
	for(uint32_t s = 0; s < sectors; s++){
		W25Q64_SectorErase(flash_addr + s * 4096);
	}
}

W25Q64_Status OTA_RecvFeed(OTA_RecvTypeDef *r, uint8_t *data, uint32_t len)
{
	/* 收够了还来数据, 多余的丢弃, 防越界 */
	if(r->recv_size + len > r->total_size){
		len = r->total_size - r->recv_size;
	}
	/* 滚动算 CRC: 必须每个字节都算, 不能等落盘后再算(桶会被覆盖) */
	OTA_CRC32_Update(&r->crc, data, len);
	r->recv_size += len;

	/* 逐字节塞进 1024 桶, 满一桶刷一页 W25Q64 */
	for(uint32_t i = 0; i < len; i++){
		r->buffer[r->buf_len++] = data[i];
		if(r->buf_len == 1024){
			if(W25Q64_WriteBuffer(r->flash_addr, r->buffer, 1024) != W25Q64_OK){
				return W25Q64_ERROR;
			}
			r->flash_addr += 1024;
			r->buf_len = 0;
		}
	}
	return W25Q64_OK;
}

uint8_t OTA_RecvFinish(OTA_RecvTypeDef *r)
{
	/* 刷残余(最后不足1024的部分) */
	if(r->buf_len > 0){
		if(W25Q64_WriteBuffer(r->flash_addr, r->buffer, r->buf_len) != W25Q64_OK){
			return 0;
		}
		r->flash_addr += r->buf_len;
		r->buf_len = 0;
	}

	/* 长度对不上 -> 传输不完整 */
	if(r->recv_size != r->total_size){
		printf("size mismatch: recv %u, expect %u\r\n", r->recv_size, r->total_size);
		return 0;
	}

	/* CRC 比对 */
	uint32_t got = OTA_CRC32_Final(r->crc);
	if(got != r->expect_crc){
		printf("crc fail: got %08X, expect %08X\r\n", got, r->expect_crc);
		return 0;
	}
	printf("download ok, crc %08X\r\n", got);
	return 1;
}

