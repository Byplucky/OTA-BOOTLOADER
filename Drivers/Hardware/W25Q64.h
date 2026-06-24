#ifndef __W25Q64_H__
#define __W25Q64_H__
typedef enum{
	W25Q64_OK = 0,
	W25Q64_ERROR ,
	W25Q64_TIMEOUT
}W25Q64_Status;

typedef enum {
	W_ENABLE = 0,
	W_DISABLE
}WriteStatus;

void W25Q64_W_SS(uint8_t BitValue);

W25Q64_Status W25Q64_WriteEnable(void);
/*
*brief:写操作完成等待
*/

W25Q64_Status W25Q64_WaitBusy(void);

W25Q64_Status W25Q64_ReadDeviceID(uint32_t *ID);
/*
	brief: 掉电模式
*/
W25Q64_Status W25Q64_PowerDown(void);
/*
	brief:释放掉电模式
*/
W25Q64_Status W25Q64_WakeUp(void);
/*
	brief:整片擦除
*/
W25Q64_Status W25Q64_ChipErase(void);

/*
 * 	@brief: Sector Erase (4KB)
 *	@arg:
 *	@retval:
 */
W25Q64_Status W25Q64_SectorErase(uint32_t ADDR);
/*
 * 	@brief: Page Program 
 *	@arg:
 *	@retval:
 */
W25Q64_Status W25Q64_WritePage(uint32_t ADDR , uint8_t *Buffer ,uint16_t Size);
/*
 * 	@brief: 写入任意长度，自动处理跨页数据
 *	@arg:
 *	@retval:
 */
W25Q64_Status W25Q64_WriteBuffer(uint32_t ADDR , uint8_t *Buffer ,uint32_t Size);
/*
* 	@brief: 读取任意长度数据
 *	@arg:
 *	@retval:
 */
W25Q64_Status W25Q64_ReadBuffer(uint32_t ADDR , uint8_t *Buffer ,uint16_t Size);
W25Q64_Status W25Q64_BLOCKERASE(uint32_t ADDR);
W25Q64_Status W25Q64_WritePageNew(uint32_t PageNum , uint8_t *Buffer ,uint16_t Size);
#endif
