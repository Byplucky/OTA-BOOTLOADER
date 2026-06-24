#ifndef __ESP8266_H__
#define __ESP8266_H__
/* UART收发缓冲大小 */
#define ESP8266_UART_RX_BUF_SIZE            1024
#define ESP8266_UART_TX_BUF_SIZE            64

/* 错误代码 */
#define ESP8266_EOK                         0   /* 没有错误 */
#define ESP8266_ERROR                       1   /* 通用错误 */
#define ESP8266_ETIMEOUT                    2   /* 超时错误 */
#define ESP8266_EINVAL                      3   /* 参数错误 */

/* 工作模式 */
#define ESP8266_STA_MODE                    1
#define ESP8266_AP_MODE                     2
#define ESP8266_STA_AP_MODE                 3

#define WIFI_SSID                           "信号不好"
#define WIFI_PWD                            "xxxxxxxxxx"

#define TCP_SERVER_IP                       "192.168.31.193"
#define TCP_SERVER_PORT                     "8080"

/* manifest.json 解析结果 */
typedef struct {
	uint32_t version_code;   /* 版本号, 用于比较是否更新 */
	uint32_t size;           /* bin 文件字节数 */
	uint32_t crc32;          /* bin 文件 CRC32 */
	char     url[64];        /* bin 下载路径, 如 /ota/APP/app.bin */
}ManifestTypeDef;

uint8_t ESP8266_Init(void);
/* 纯解析: 从 json 文本抽出字段, 脱机可测. 成功 EOK, 字段缺失 EINVAL */
uint8_t ESP8266_ParseManifest(char *json, ManifestTypeDef *m);
void ESP8266_WriteInfo(void);
uint8_t ESP8266_GetBinAndWrite(void);
/* 裸TCP发送原始数据 / 发HTTP GET请求 */
uint8_t ESP8266_CIPSend(uint8_t *data, uint16_t len);
uint8_t ESP8266_HttpGet(char *path);
/* 获取 manifest + 解析 + 与本地版本比较. 返回 1=需要更新, 0=不需要/失败 */
uint8_t ESP8266_GetJson(ManifestTypeDef *m);
uint8_t ESP8266_Reset(void);
#endif

