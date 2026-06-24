#include "gpio.h"
#include "spi.h"
#include "W25Q64_INF.h"

void W25Q64_W_SS(uint8_t BitValue){
	HAL_GPIO_WritePin(SPI_SS_GPIO_Port,SPI_SS_Pin,(GPIO_PinState)BitValue);
}

W25Q64_Status W25Q64_WriteEnable(void)
{
        uint8_t cmd[] = {W25Q64_WRITE_ENABLE};
        /*Select the FLASH: Chip Select low */
        W25Q64_W_SS(W_ENABLE);
        /* Send the read ID command */
        if(HAL_SPI_Transmit(&hspi1, cmd, 1, HAL_MAX_DELAY) != HAL_OK){
			return W25Q64_ERROR;
		}        
        /*Deselect the FLASH: Chip Select high */
        W25Q64_W_SS(W_DISABLE);
		return W25Q64_OK;
}
/*
*brief:写操作完成等待
*/

W25Q64_Status W25Q64_WaitBusy(void){
	uint8_t cmd[] = {W25Q64_READ_STATUS_REGISTER_1};
	uint8_t status = 0;
	W25Q64_W_SS(W_ENABLE);
	if(HAL_SPI_Transmit(&hspi1,cmd,1,HAL_MAX_DELAY) != HAL_OK){
		return W25Q64_ERROR;
	}
	do{
	if(HAL_SPI_Receive(&hspi1,&status,1,HAL_MAX_DELAY) != HAL_OK){
		break;
	}
	}
	while((status & 0x01) == SET);
	W25Q64_W_SS(W_DISABLE);
	return W25Q64_OK;
}

W25Q64_Status W25Q64_ReadDeviceID(uint32_t *ID){
	uint8_t  Tx_Buff[4] = {0};
	uint8_t  Rx_Buff[4] = {0};
	Tx_Buff[0] = W25Q64_JEDEC_ID;	
	Tx_Buff[1] = W25Q64_DUMMY_BYTE;
	Tx_Buff[2] = W25Q64_DUMMY_BYTE;
	Tx_Buff[3] = W25Q64_DUMMY_BYTE;
	W25Q64_W_SS(W_ENABLE);
	if(HAL_SPI_TransmitReceive(&hspi1,Tx_Buff,Rx_Buff,4,1000) != HAL_OK){
		W25Q64_W_SS(W_DISABLE);
		return W25Q64_ERROR;
	}
	W25Q64_W_SS(W_DISABLE);
	*ID = ((uint32_t)Rx_Buff[1]<<16)|((uint32_t)Rx_Buff[2]<<8) |((uint32_t)Rx_Buff[3]); 
	return W25Q64_OK;
}
/*
	brief: 掉电模式
*/
W25Q64_Status W25Q64_PowerDown(void){
	
        uint8_t cmd[] = {W25Q64_POWER_DOWN};
        W25Q64_W_SS(W_ENABLE);
        if(HAL_SPI_Transmit(&hspi1, cmd, 1, HAL_MAX_DELAY) != HAL_OK){
			return W25Q64_ERROR;
		}        
        W25Q64_W_SS(W_DISABLE);
		return W25Q64_OK;

}
/*
	brief:释放掉电模式
*/
W25Q64_Status W25Q64_WakeUp(void){
	
        uint8_t cmd[] = {W25Q64_RELEASE_POWER_DOWN_HPM_DEVICE_ID};
        W25Q64_W_SS(W_ENABLE);
        if(HAL_SPI_Transmit(&hspi1, cmd, 1, HAL_MAX_DELAY) != HAL_OK){
			return W25Q64_ERROR;
		}        
        W25Q64_W_SS(W_DISABLE);
		return W25Q64_OK;

}
/*
	brief:整片擦除
*/
W25Q64_Status W25Q64_ChipErase(void){
	
        uint8_t cmd[] = {W25Q64_CHIP_ERASE};
		W25Q64_WriteEnable();
        W25Q64_W_SS(W_ENABLE);
        if(HAL_SPI_Transmit(&hspi1, cmd, 1, HAL_MAX_DELAY) != HAL_OK){
			return W25Q64_ERROR;
		}        
        W25Q64_W_SS(W_DISABLE);
		W25Q64_WaitBusy();
		return W25Q64_OK;

}

/*
 * 	@brief: BLOCK ERASE 64KB
 *	@arg:
 *	@retval:
 */
W25Q64_Status W25Q64_BLOCKERASE(uint32_t PageNum){
	
        uint8_t cmd[4] = {0};
		cmd[0] = W25Q64_BLOCK_ERASE_64KB;
		cmd[1] = (PageNum*256)>> 16;
		cmd[2] = (PageNum*256)>> 8;
		cmd[3] = (PageNum*256);
		W25Q64_WriteEnable();
        W25Q64_W_SS(W_ENABLE);
        if(HAL_SPI_Transmit(&hspi1, cmd, 4, HAL_MAX_DELAY) != HAL_OK){
			return W25Q64_ERROR;
		}        
        W25Q64_W_SS(W_DISABLE);
		W25Q64_WaitBusy();
		return W25Q64_OK;
}
/*
 * 	@brief: Sector Erase (4KB)
 *	@arg:
 *	@retval:
 */
W25Q64_Status W25Q64_SectorErase(uint32_t ADDR){

        uint8_t cmd[4] = {0};
		cmd[0] = W25Q64_SECTOR_ERASE_4KB;
		cmd[1] = (ADDR)>> 16;
		cmd[2] = (ADDR)>> 8;
		cmd[3] = (ADDR);
		W25Q64_WriteEnable();
        W25Q64_W_SS(W_ENABLE);
        if(HAL_SPI_Transmit(&hspi1, cmd, 4, HAL_MAX_DELAY) != HAL_OK){
			return W25Q64_ERROR;
		}        
        W25Q64_W_SS(W_DISABLE);
		W25Q64_WaitBusy();
		return W25Q64_OK;
}
/*
 * 	@brief: Page Program 
 *	@arg:
 *	@retval:
 */
W25Q64_Status W25Q64_WritePage(uint32_t ADDR , uint8_t *Buffer ,uint16_t Size){
		if(Size + (ADDR % W25Q64_PAGE_SIZE) > W25Q64_PAGE_SIZE) return W25Q64_ERROR;
        uint8_t cmd[4] = {0};
		cmd[0] = W25Q64_PAGE_PROGRAM;
		cmd[1] = (ADDR) >> 16;
		cmd[2] = (ADDR) >> 8;
		cmd[3] = (ADDR);
		W25Q64_WriteEnable();
        W25Q64_W_SS(W_ENABLE);
        if(HAL_SPI_Transmit(&hspi1, cmd, 4, HAL_MAX_DELAY) != HAL_OK){
			return W25Q64_ERROR;
		}  
        if(HAL_SPI_Transmit(&hspi1, Buffer, Size, HAL_MAX_DELAY) != HAL_OK){
			return W25Q64_ERROR;
		}    		
        W25Q64_W_SS(W_DISABLE);
		W25Q64_WaitBusy();
		return W25Q64_OK;
}
W25Q64_Status W25Q64_WritePageNew(uint32_t PageNum , uint8_t *Buffer ,uint16_t Size){
	//	if(Size + (ADDR % W25Q64_PAGE_SIZE) > W25Q64_PAGE_SIZE) return W25Q64_ERROR;
        uint8_t cmd[4] = {0};
		cmd[0] = W25Q64_PAGE_PROGRAM;
		cmd[1] = (PageNum*256) >> 16;
		cmd[2] = (PageNum*256) >> 8;
		cmd[3] = (PageNum*256);
		W25Q64_WriteEnable();
        W25Q64_W_SS(W_ENABLE);
        if(HAL_SPI_Transmit(&hspi1, cmd, 4, HAL_MAX_DELAY) != HAL_OK){
			return W25Q64_ERROR;
		}  
        if(HAL_SPI_Transmit(&hspi1, Buffer, Size, HAL_MAX_DELAY) != HAL_OK){
			return W25Q64_ERROR;
		}    		
        W25Q64_W_SS(W_DISABLE);
		W25Q64_WaitBusy();
		return W25Q64_OK;
}
/*
 * 	@brief: 写入任意长度，自动处理跨页数据
 *	@arg:
 *	@retval:
 */
W25Q64_Status W25Q64_WriteBuffer(uint32_t ADDR , uint8_t *Buffer ,uint32_t Size){
		uint32_t write_num = Size;
		uint32_t current_addr = ADDR;
		uint32_t offset = ADDR % W25Q64_PAGE_SIZE;

	/* 起始地址不在页边界: 先把当前页剩余部分写满 */
	if(offset != 0){
		uint32_t page_remain = W25Q64_PAGE_SIZE - offset;
		if(write_num > page_remain){
			W25Q64_WritePage(current_addr,Buffer, page_remain);
			current_addr += page_remain;
			Buffer += page_remain;
			write_num  -= page_remain;
		}
		else{
			W25Q64_WritePage(ADDR, Buffer, Size);
			return W25Q64_OK;
		}
	}

	/* 按整页(256)逐页写, 最后不足一页写剩余 */
	while(write_num > 0){
		 uint16_t write_size = (write_num > W25Q64_PAGE_SIZE) ? W25Q64_PAGE_SIZE : write_num;
		 W25Q64_WritePage(current_addr,Buffer, write_size);
		 current_addr += write_size;
		 Buffer += write_size;
		 write_num  -= write_size;
	}

  	 return W25Q64_OK;
}
/*
* 	@brief: 读取任意长度数据
 *	@arg:
 *	@retval:
 */
W25Q64_Status W25Q64_ReadBuffer(uint32_t ADDR , uint8_t *Buffer ,uint16_t Size){
        uint8_t cmd[4] = {0};
		cmd[0] = W25Q64_READ_DATA;
		cmd[1] = ((ADDR) >> 16) & 0xFF;
		cmd[2] = ((ADDR)>> 8) & 0xFF;
		cmd[3] =  (ADDR) & 0xFF;
        W25Q64_W_SS(W_ENABLE);
        if(HAL_SPI_Transmit(&hspi1, cmd, 4, HAL_MAX_DELAY) != HAL_OK){
			return W25Q64_ERROR;
		}  
        if(HAL_SPI_Receive(&hspi1, Buffer, Size, HAL_MAX_DELAY) != HAL_OK){
			return W25Q64_ERROR;
		}    		
        W25Q64_W_SS(W_DISABLE);
		W25Q64_WaitBusy();
		return W25Q64_OK;
}
