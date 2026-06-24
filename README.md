# STM32F103 OTA Bootloader (ESP8266 + 本地服务器)

基于 STM32F103C8T6 的固件 OTA 升级方案。MCU 通过 ESP8266（AT 固件）连接 WiFi，从本地 Nginx 服务器拉取固件，校验后写入并跳转运行。同时保留串口 Xmodem 升级作为备用通道。

## 特性

- **WiFi OTA**：ESP8266 走裸 TCP + 手动拼 HTTP GET，从服务器下载 `manifest.json` 和固件 `.bin`
- **版本管理**：服务端 manifest 描述版本号 / 大小 / CRC32 / 下载路径，本地版本号存于 AT24C02，仅当服务器版本更高才升级
- **完整性校验**：下载过程中流式计算 IEEE 802.3 CRC32（多项式 `0xEDB88320`），与 manifest 比对，校验失败拒绝升级
- **掉电保护**：升级标志（`0x11223344`）写入 AT24C02，升级途中掉电复位后可恢复继续
- **双升级通道**：除 WiFi OTA 外，还支持串口 Xmodem-CRC 升级（CLI 模式）
- **外部 Flash 暂存**：固件先下载到 W25Q64（SPI Flash）暂存，校验通过后再搬运到内部 Flash，避免下载失败损坏运行区

## 硬件

| 模块 | 说明 |
|------|------|
| MCU | STM32F103C8T6（64KB Flash / 1KB 每页 / 20KB RAM） |
| WiFi | ESP8266（AT 固件，STA 模式，被动接收 `CIPRECVMODE=1`） |
| 外部 Flash | W25Q64（SPI，8MB，固件暂存区） |
| EEPROM | AT24C02（软件 I2C，存升级标志/版本/CRC/大小） |
| 调试 | USART1 printf 输出；USART2 接 ESP8266 |

## Flash 分区

```
0x08000000 ┌──────────────────────┐
           │  Bootloader (页 0~23) │  24KB  下载 + 校验 + 搬运 + 跳转
0x08006000 ├──────────────────────┤
           │  App (页 24~63)       │  40KB  应用固件
0x0800FFFF └──────────────────────┘
```

> 本项目为**单镜像 single-image 设计**：下载、校验、安装全部逻辑都在 Bootloader 中完成，App 区只放纯应用。

## 升级流程

```
上电
 │
 ├─ 3s 内串口收到 'w' ──► 进入 CLI 模式（手动擦除 / Xmodem 升级 / 查版本 / 重启）
 │
 └─ 未按键
     │
     ├─ ESP8266 联网 ──► GET manifest.json ──► 解析版本
     │       │
     │       └─ 服务器版本更高？
     │             │
     │             ├─ 是 ──► GET app.bin ──► 流式写 W25Q64 + 算 CRC32
     │             │              │
     │             │              └─ CRC 校验通过？──► 写升级标志到 AT24C02
     │             │
     │             └─ 否 ──► 跳过
     │
     └─ 检查 AT24C02 升级标志
            │
            ├─ 有标志 ──► W25Q64 固件搬运到内部 Flash App 区 ──► 清标志 ──► 复位
            │
            └─ 无标志 ──► 校验 App 栈顶有效 ──► 跳转运行 App
```

## 服务器 manifest 格式

服务器（Nginx）放一个 `manifest.json`：

```json
{
  "version_code": 10003,
  "size": 3312,
  "crc32": "0xD9D4BE58",
  "url": "/ota/APP/app.bin"
}
```

> `crc32` 须与设备端算法一致（等价于 Python `zlib.crc32` / `binascii.crc32`）。

## 目录结构

```
Drivers/Hardware/
├── ESP8266.c/h     WiFi AT 通信、HTTP GET、manifest 解析、固件下载
├── Bootloader.c/h  分区跳转、Xmodem、CRC32、固件累积器
├── W25Q64.c/h      SPI Flash 驱动（读/写/擦除，自动跨页）
├── AT24C02.c/h     EEPROM 驱动（软件 I2C），升级信息持久化
├── FMC.c/h         内部 Flash 读写擦除
└── SW_I2C.c/h      软件模拟 I2C
Core/Src/main.c     主流程、DMA 串口接收（多帧累加）
```

## 开发环境

- STM32CubeMX 生成外设初始化
- Keil MDK-ARM 编译（工程在 `MDK-ARM/`）
- HAL 库

## 关键实现要点

- **DMA + Idle 多帧累加**：ESP8266 的 AT 响应常分多个 IDLE 帧到达，接收回调按 `Rx_CNT` 累加追加而非覆盖，命令处理完再重置
- **被动接收模式**：用 `CIPRECVMODE=1` + `CIPRECVDATA`，靠 `+IPD` 通知判断数据到达再取，比盲等固定延时可靠
- **二进制安全**：固件含 `0x00` 字节，提取载荷用 `+CIPRECVDATA:<len>` 里的实际长度做指针运算，不依赖 `strlen`
- **流式 CRC**：边收边算 CRC32，不依赖完整缓存，适配小 RAM
