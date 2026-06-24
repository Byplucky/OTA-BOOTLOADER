#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "ESP8266.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "AT24C02.h"
#include "Bootloader.h"
uint8_t ESP_Rx_Buf[ESP8266_UART_RX_BUF_SIZE];
uint8_t ESP_Tx_Buf[ESP8266_UART_TX_BUF_SIZE];
uint32_t Rx_CNT;
uint8_t esp_RxFlag;


void ESP8266_Clear(void)
{
	memset(ESP_Rx_Buf, 0, ESP8266_UART_RX_BUF_SIZE);
	esp_RxFlag = 0;
	Rx_CNT = 0;
	/* 重置DMA到buffer开头, 让下一条命令的响应从头累加 */
	HAL_UART_AbortReceive(&huart2);
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, ESP_Rx_Buf, ESP8266_UART_RX_BUF_SIZE);
}
/*发送AT指令 */


uint8_t ESP8266_SendCmd(char *cmd, char *expect,uint32_t timeOut){
	ESP8266_Clear();
	HAL_UART_Transmit(&huart2, (unsigned char *)cmd, strlen((const char *)cmd), 100);

	while(timeOut--) {
		if(esp_RxFlag) {						//如果收到数据
			if(strstr((const char *)ESP_Rx_Buf, expect) != NULL)		//如果检索到关键词
				return ESP8266_EOK;
		}
		HAL_Delay(10);
	}

	return ESP8266_ERROR;
}

/* 在已累积的接收缓冲里轮询等待某关键词出现(不清缓冲, 不发命令).
 * 用于被动模式下等 ESP 自发的异步通知, 如 "+IPD"(有数据到达).
 * 命中返回 EOK, 超时返回 ERROR. timeOut 单位 10ms. */
static uint8_t ESP8266_WaitRecv(char *expect, uint32_t timeOut)
{
	while(timeOut--){
		if(strstr((const char *)ESP_Rx_Buf, expect) != NULL)
			return ESP8266_EOK;
		HAL_Delay(10);
	}
	return ESP8266_ERROR;
}


uint8_t ESP8266_ATEconfig(uint8_t cfg)
{
    switch (cfg) {
        case 0:
            return ESP8266_SendCmd("ATE0\r\n", "OK",200);   /* 关闭回显 */       
        case 1:
            return ESP8266_SendCmd("ATE1\r\n", "OK",200);   /* 打开回显 */
        
        default:
            return 0;
    }
}
uint8_t ESP8266_Reset(void){

	return ESP8266_SendCmd("AT+RST\r\n","OK",800);

}
/*设置模式*/
uint8_t ESP8266_SetMode(uint8_t mode)
{
    switch (mode) {
        case ESP8266_STA_MODE:
            return ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK",200);    /* Station模式 */
        
        case ESP8266_AP_MODE:
            return ESP8266_SendCmd("AT+CWMODE=2\r\n", "OK",200);    /* AP模式 */
        
        case ESP8266_STA_AP_MODE:
            return ESP8266_SendCmd("AT+CWMODE=3\r\n", "OK",200);    /* AP+Station模式 */
        
        default:
            return ESP8266_EINVAL;
    }
}
/*设置单链接模式*/
uint8_t ESP8266_SingleConnection(void)
{
    return ESP8266_SendCmd("AT+CIPMUX=0\r\n", "OK",200);
}

/*链接wifi*/
uint8_t ESP8266_ConnectWifi(void)
{
    char cmd[64];
    
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID,WIFI_PWD);
    
    return ESP8266_SendCmd(cmd, "WIFI GOT IP",1500);
}
/*ESP8266连接TCP服务器
 * 先无条件 CIPCLOSE 清掉上一次残留的连接(STM32复位但ESP没复位时会残留),
 * 再 CIPSTART, 失败则重试几次. 单连接模式下不先关会回 ALREADY CONNECTED/busy. */
uint8_t ESP8266_ConnectTCPServer(void){
    char cmd[128];

    sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%s\r\n", TCP_SERVER_IP, TCP_SERVER_PORT);

        ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK", 100);
        HAL_Delay(100);
	    return ESP8266_SendCmd(cmd,"OK",200);

}
/*设置套接字接收模式*/
uint8_t ESP8266_SocketMode(void){
	
	return ESP8266_SendCmd("AT+CIPRECVMODE=1\r\n","OK",200);
}

/*ESP8266断开TCP服务器*/
uint8_t ESP8266_DisconnectTCPServer(void)
{
    return ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK",200);

}

/* 通过已建立的TCP连接发送一段原始数据(用CIPSEND).
 * data: 要发的字节, len: 字节数.
 * 先 AT+CIPSEND=len 等 ">", 再把 data 发出去, 等 "SEND OK". */
uint8_t ESP8266_CIPSend(uint8_t *data, uint16_t len)
{
	char cmd[32];

	/* 第一步: 告知长度, 等待 ">" 提示符 */
	sprintf(cmd, "AT+CIPSEND=%u\r\n", len);
	ESP8266_Clear();
	HAL_UART_Transmit(&huart2, (uint8_t *)cmd, strlen(cmd), 100);
	{
		uint32_t t = 100;
		uint8_t got = 0;
		while(t--){
			if(esp_RxFlag && strchr((const char *)ESP_Rx_Buf, '>')){ got = 1; break; }
			HAL_Delay(10);
		}
		if(!got) return ESP8266_ERROR;
	}

	/* 第二步: 发原始数据, 等 "SEND OK" */
	ESP8266_Clear();
	HAL_UART_Transmit(&huart2, data, len, 1000);
	{
		uint32_t t = 200;
		while(t--){
			if(esp_RxFlag && strstr((const char *)ESP_Rx_Buf, "SEND OK")) return ESP8266_EOK;
			HAL_Delay(10);
		}
	}
	return ESP8266_ERROR;
}

/* 发一个 HTTP GET 请求(用sprintf拼, strlen算长度, 不会数错字节) */
uint8_t ESP8266_HttpGet(char *path)
{
	char req[160];
	int len = sprintf(req,
		"GET %s HTTP/1.1\r\n"
		"Host: %s:%s\r\n"
		"Connection: close\r\n"
		"\r\n",
		path, TCP_SERVER_IP, TCP_SERVER_PORT);
	return ESP8266_CIPSend((uint8_t *)req, (uint16_t)len);
}
 
/*
* brief:初始化EPS8266,连接AP
* parem:NONE
* retval:
*/


uint8_t ESP8266_Init(void){
	if(ESP8266_ATEconfig(0)        != ESP8266_EOK) return ESP8266_ERROR;  /* 关回显 */
	if(ESP8266_SetMode(ESP8266_STA_MODE) != ESP8266_EOK) return ESP8266_ERROR;  /* STA模式 */
	if(ESP8266_SingleConnection()  != ESP8266_EOK) return ESP8266_ERROR;  /* 单连接 */
	if(ESP8266_ConnectWifi()       != ESP8266_EOK) return ESP8266_ERROR;  /* 连WiFi(慢) */
	return ESP8266_EOK;
}

/* ============ manifest.json 解析 ============ */
/* 纯字符串解析, 不依赖硬件, 可脱机喂测试串验证.
 * 思路: 找到 key -> 跳到冒号 -> strtoul 自动跳空格读值.
 *      crc32/url 是带引号字符串, 需先跳过开引号. */

/* 在 json 里找 key 对应的冒号位置, 返回冒号后第一个字符地址, 找不到返回 NULL */
static char *json_value_ptr(char *json, char *key)
{
	char *p = strstr(json, key);
	if(p == NULL) return NULL;
	p = strchr(p, ':');          /* 跳到冒号 */
	if(p == NULL) return NULL;
	return p + 1;                /* 冒号后(可能有空格, strtoul会自动跳) */
}

uint8_t ESP8266_ParseManifest(char *json, ManifestTypeDef *m)
{
	char *p;

	/* version_code: 十进制. 用带下划线的全名, 避免匹配到 "version" */
	p = json_value_ptr(json, "version_code");
	if(p == NULL) return ESP8266_EINVAL;
	m->version_code = strtoul(p, NULL, 10);
	
	/* size: 十进制 */
	p = json_value_ptr(json, "size");
	if(p == NULL) return ESP8266_EINVAL;
	m->size = strtoul(p, NULL, 10);

	/* crc32: 带引号的 "0x...." 十六进制. 先跳到开引号后 */
	p = json_value_ptr(json, "crc32");
	if(p == NULL) return ESP8266_EINVAL;
	p = strchr(p, '\"');                 /* 跳到开引号 */
	if(p == NULL) return ESP8266_EINVAL;
	m->crc32 = strtoul(p + 1, NULL, 16); /* 从引号后的 0x.. 起, base16自动识别0x */

	/* url: 带引号字符串, 拷贝引号之间的内容 */
	p = json_value_ptr(json, "url");
	if(p == NULL) return ESP8266_EINVAL;
	p = strchr(p, '\"');                 /* 开引号 */
	if(p == NULL) return ESP8266_EINVAL;
	p++;                                 /* 引号内首字符 */
	{
		char *q = strchr(p, '\"');       /* 闭引号 */
		uint32_t n;
		if(q == NULL) return ESP8266_EINVAL;
		n = q - p;
		if(n >= sizeof(m->url)) n = sizeof(m->url) - 1;
		memcpy(m->url, p, n);
		m->url[n] = '\0';
	}

	return ESP8266_EOK;
}

void ESP8266_WriteInfo(void){
	OTA_CtlStructure.HttpCRC =  ManifestHandle.crc32;
	OTA_CtlStructure.OTA_UpdataSize[0] = ManifestHandle.size;
	OTA_CtlStructure.Version = ManifestHandle.version_code;
	OTA_CtlStructure.OTA_Flag = OTA_FLAG_UPDATA;
	AT24C02_Write_OTAInf();
}
/* 获取 manifest + 解析 + 比较本地版本.
 * 走裸TCP(本固件不支持HTTPCGET/HTTPCLIENT).
 * 返回 1=需要更新, 0=不需要或失败 */
static uint8_t ESP8266_GetJson_Once(ManifestTypeDef *m)
{
	/* 1. 被动接收模式 */
	if(ESP8266_SocketMode() != ESP8266_EOK){
		printf("recvmode fail\r\n");
		return 0xFF;   /* 0xFF=本次失败可重试 */
	}
	/* 2. 连TCP */
	if(ESP8266_ConnectTCPServer() != ESP8266_EOK){
		printf("tcp connect fail\r\n");
		return 0xFF;
	}
	/* 3. 发HTTP GET请求 */
	if(ESP8266_HttpGet("/ota/manifest.json") != ESP8266_EOK){
		printf("http get fail\r\n");
		ESP8266_DisconnectTCPServer();
		return 0xFF;
	}
	/* 关键: 等服务器把响应数据送达ESP缓冲. 被动模式下数据到达时ESP会自发
	 * "+IPD,<len>" 通知, 等到它再取数据, 比盲等固定延时可靠.
	 * 等不到(网络慢/丢)就当本次失败可重试. */
	if(ESP8266_WaitRecv("+IPD", 200) != ESP8266_EOK){
		printf("no data notify\r\n");
		ESP8266_DisconnectTCPServer();
		return 0xFF;
	}
	/* 4. 取响应数据(manifest小, 一次512够). */
	if(ESP8266_SendCmd("AT+CIPRECVDATA=512\r\n", "+CIPRECVDATA", 300) != ESP8266_EOK){
		printf("recvdata fail\r\n");
		ESP8266_DisconnectTCPServer();
		return 0xFF;
	}
	/* 再等一下, 让CIPRECVDATA的应答(头+JSON)多帧累加完整再解析 */
	HAL_Delay(100);

	/* 5. 解析(ParseManifest用strstr找key, 自动跳过+CIPRECVDATA头和HTTP响应头) */
	if(ESP8266_ParseManifest((char *)ESP_Rx_Buf, m) != ESP8266_EOK){
		printf("parse manifest fail\r\n");
		return 0xFF;
	}

	printf("server ver:%u size:%u crc:%08X url:%s\r\n",
	       m->version_code, m->size, m->crc32, m->url);
   // ESP8266_DisconnectTCPServer();
	/* 6. 与本地版本比较: 服务器版本更高才更新 */
	if(m->version_code > OTA_CtlStructure.Version){
		printf("need update: %u -> %u\r\n", OTA_CtlStructure.Version, m->version_code);
		return 1;
	}
	printf("already latest\r\n");
	return 0;
}

/* 对外接口: 带重试地获取并解析manifest.
 * GetJson_Once 返回 1=需更新 / 0=已最新(最终态) / 0xFF=本次网络/解析失败可重试.
 * 这里把 0xFF 当作可重试, 0 和 1 都是最终结果直接返回. */
uint8_t ESP8266_GetJson(ManifestTypeDef *m)
{
	uint8_t r;
	uint8_t retry;
	for(retry = 0; retry < 3; retry++){
		r = ESP8266_GetJson_Once(m);
		if(r != 0xFF) return r;          /* 0或1是最终结果 */
		printf("get manifest retry %d\r\n", retry + 1);
		HAL_Delay(500);                  /* 退避一下再试, 给ESP/网络喘息 */
	}
	return 0;                            /* 重试耗尽, 当作不更新 */
}

uint8_t ESP8266_GetBinAndWrite(void){
	if(ESP8266_ConnectTCPServer() != ESP8266_EOK){
		printf("tcp connect fail\r\n");
		return 0;
	}
		
	if(ESP8266_HttpGet(ManifestHandle.url) != ESP8266_EOK){
		printf("http get fail\r\n");
		ESP8266_DisconnectTCPServer();
		return 0;
	}	
	OTA_RecvInit(&OTA_RecvHandle,0x00,ManifestHandle.size,ManifestHandle.crc32);

	/* 等首批数据到达ESP缓冲(+IPD通知)再取, 否则首个CIPRECVDATA可能取空 */
	if(ESP8266_WaitRecv("+IPD", 200) != ESP8266_EOK){
		printf("no data notify\r\n");
		ESP8266_DisconnectTCPServer();
		return 0;
	}
	if(ESP8266_SendCmd("AT+CIPRECVDATA=512\r\n", "+CIPRECVDATA", 200) != ESP8266_EOK){
		printf("recvdata fail\r\n");
		ESP8266_DisconnectTCPServer();
	//	ESP8266_Reset();
		return 0;
	}
	    char *p = strstr((char *)ESP_Rx_Buf, "+CIPRECVDATA:");
		uint32_t actual_len = strtoul(p + 13, NULL, 10);   // 13 = strlen("+CIPRECVDATA:")
		char *data = strchr(p, ',') + 1;                   // 逗号后是原始载荷起点
		char *data_end = data + actual_len;                // 载荷结束位置(用len算，不靠\0)
		
		// 2) 在 HTTP 头里找空行 \r\n\r\n
		char *body = strstr(data, "\r\n\r\n");
		body += 4;                                         // 跳过空行，body就是bin第一个字节
		
		// 3) 这一批里属于 bin 的字节数
		uint32_t first_chunk = data_end - body;
		OTA_RecvFeed(&OTA_RecvHandle,(uint8_t *)body,first_chunk);
		
		while(OTA_RecvHandle.recv_size < OTA_RecvHandle.total_size){
		if(ESP8266_SendCmd("AT+CIPRECVDATA=512\r\n", "+CIPRECVDATA", 200) != ESP8266_EOK){
			printf("recvdata fail\r\n");
			ESP8266_DisconnectTCPServer();
			return 0;
			}
		    p = strstr((char *)ESP_Rx_Buf, "+CIPRECVDATA:");
			actual_len = strtoul(p + 13, NULL, 10);
			data = strchr(p, ',') + 1;
			OTA_RecvFeed(&OTA_RecvHandle,(uint8_t *)data,actual_len);
		}
		return OTA_RecvFinish(&OTA_RecvHandle);
}
	

