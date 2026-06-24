#include "FMC.h"

/*
 * @brief : 页擦除
 * @param : start 起始页号 (0 ~ 63)
 *          num   擦除页数
 * @retval: FMC_Erase_OK / FMC_Erase_ERROR
 */
FMC_Status FMC_FlashPageErase(uint16_t start, uint16_t num)
{
	uint32_t PageError = 0;
	FLASH_EraseInitTypeDef eraseInit;

	/* 边界校验：起始页不能落在 Bootloader 区，范围不能越界 */
	if (start < FMC_APP_START_PAGE) {
		return FMC_Erase_ERROR;
	}
	if (start + num > FMC_TOTAL_PAGES) {
		return FMC_Erase_ERROR;
	}

	eraseInit.TypeErase   = FLASH_TYPEERASE_PAGES;
	eraseInit.Banks       = FLASH_BANK_1;
	eraseInit.PageAddress = FMC_FLASH_BASE + start * FMC_PAGE_SIZE;
	eraseInit.NbPages     = num;

	HAL_FLASH_Unlock();
	if (HAL_FLASHEx_Erase(&eraseInit, &PageError) != HAL_OK) {
		HAL_FLASH_Lock();
		return FMC_Erase_ERROR;
	}
	HAL_FLASH_Lock();

	return FMC_Erase_OK;
}

/*
 * @brief : 写入字节流（内部以半字为单位编程）
 * @param : addr 目标地址（必须半字/2字节对齐）
 *          data 待写入字节流
 *          len  字节数
 * @retval: FMC_Write_OK / FMC_Write_ERROR
 * @note  : 写前需保证目标页已擦除；len 为奇数时末字节补 0xFF
 */
FMC_Status FMC_FlashWrite(uint32_t addr, uint8_t *data, uint32_t len)
{
	uint32_t i;

	/* 边界校验 */
	if (addr < FMC_APP_ADDR) {                 /* 不能写进 Bootloader 区 */
		return FMC_Write_ERROR;
	}
	if (addr + len > FMC_FLASH_END + 1) {      /* 不能越界 */
		return FMC_Write_ERROR;
	}
	if (addr & 0x1U) {                         /* 半字对齐 */
		return FMC_Write_ERROR;
	}

	HAL_FLASH_Unlock();
	for (i = 0; i < len; i += 2) {
		uint16_t lo = data[i];
		uint16_t hi = (i + 1 < len) ? data[i + 1] : 0xFF;  /* 奇数长度补 0xFF */
		uint16_t halfWord = (uint16_t)(lo | (hi << 8));

		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, halfWord) != HAL_OK) {
			HAL_FLASH_Lock();
			return FMC_Write_ERROR;
		}
	}
	HAL_FLASH_Lock();

	return FMC_Write_OK;
}

/*
 * @brief : 读取 Flash（直接地址映射拷贝）
 * @param : addr   源地址
 *          buffer 目标缓冲
 *          len    字节数
 */
void FMC_FlashRead(uint32_t addr, uint8_t *buffer, uint32_t len)
{
	uint32_t i;
	for (i = 0; i < len; i++) {
		buffer[i] = *(__IO uint8_t *)(addr + i);
	}
}
