#ifndef __FMC_H__
#define __FMC_H__

#include "main.h"

/* ---------- STM32F103C8T6 内部 Flash 参数 ---------- */
#define FMC_FLASH_BASE        0x08000000U   /* Flash 起始地址 */
#define FMC_PAGE_SIZE         1024U         /* C8T6 每页 1KB */
#define FMC_TOTAL_PAGES       64U           /* 共 64 页 (64KB) */
#define FMC_FLASH_END         0x0800FFFFU   /* Flash 末地址 */

/* ---------- Bootloader / App 分区 ---------- */
/* Bootloader: 页 0~23 (24KB)，App: 页 24~63 (40KB) */
#define FMC_APP_START_PAGE        24U            /*Bootloader 区大小*/
#define FMC_APP_PAGE_NUM         (FMC_TOTAL_PAGES - FMC_APP_START_PAGE)
#define FMC_APP_ADDR          (FMC_FLASH_BASE + FMC_APP_START_PAGE * FMC_PAGE_SIZE) /* APP起始地址0x08006000*/
#define FMC_APP_MAX_SIZE      ((FMC_TOTAL_PAGES - FMC_BAPP_START_PAGE) * FMC_PAGE_SIZE) /*APP区总字节数*/

typedef enum {
	FMC_Erase_OK = 0,
	FMC_Erase_ERROR,
	FMC_Write_OK,
	FMC_Write_ERROR
}FMC_Status;

/* 页擦除：start=起始页号(0~63)，num=页数 */
FMC_Status FMC_FlashPageErase(uint16_t start, uint16_t num);

/* 字节流写入：addr=目标地址(半字对齐)，data=字节流，len=字节数 */
FMC_Status FMC_FlashWrite(uint32_t addr, uint8_t *data, uint32_t len);

/* 读取：内部 Flash 直接映射，直接拷贝 */
void FMC_FlashRead(uint32_t addr, uint8_t *buffer, uint32_t len);

#endif
