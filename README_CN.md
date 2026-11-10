# DFRobot_SGX6410
- [English Version](./README.md)

DFRobot_SGX6410 是 SGX6410 MEMS 空气质量传感器的 Arduino 库。传感器用于评估室内空气质量，可输出 IAQ、TVOC、乙醇当量（ETOH）、估计 CO2（eCO2）和相对 IAQ（RelIAQ）。

主机通过 I2C 或 UART 与 SGX6410 通信，两者使用同一套命令帧。I2C 从机地址在 bit6:0（7 位，默认 `0x38`）；可编程地址字节的 bit7 忽略。UART 默认 9600 8N1。

典型流程：调用 `begin()` 读取当前工作模式并初始化预热计时，设置工作模式（IAQ / ULP / PBAQ / Suspend），等待预热完成（`valid` 为 true），再调用 `update()` 读取缓存的气体数据。湿度可按 0–100 %RH 手动写入。

## 产品链接 ([https://www.dfrobot.com.cn](https://www.dfrobot.com.cn))

```
SKU: SEN0771
```

## 目录

  * [产品链接](#产品链接-httpswwwdfrobotcomcn)
  * [概述](#概述)
  * [库安装](#库安装)
  * [方法](#方法)
  * [兼容性](#兼容性)
  * [历史](#历史)
  * [创作者](#创作者)

## 概述

* 双接口：I2C（bit6:0 为 7 位从机地址，默认 `0x38`，bit7 忽略）或 UART 命令帧，9600 8N1
* `begin()` 读取当前工作模式并初始化预热计时
* 工作模式：Suspend；IAQ（约 3 s 采样，预热约 5 min）；ULP（约 90 s 采样，预热约 15 min）；PBAQ（约 5 s 采样，预热约 5 min）
* `update()` 按模式换算：IAQ/ULP 为 IAQ=raw/10、TVOC=raw/100 mg/m³、ETOH=raw/100 ppm、eCO2=raw ppm、RelIAQ=raw/10；PBAQ 仅输出 TVOC/ETOH（raw/1000），其余为 NAN
* 使用读数前用 `isNew` 判断是否新样本，用 `valid` 判断预热是否完成
* 手动湿度补偿：写入 0–100 %RH（内部存为 0–255 码）
* 支持 Flash Shadow 中的 I2C 地址与启动模式

## 库安装

* 在 Arduino IDE 库管理器中搜索 `DFRobot_SGX6410` 并安装。
* 或下载本仓库，解压到 Arduino 的 `libraries` 目录，再打开 `examples` 中的示例。

## 方法

```C++
  /**
   * @fn DFRobot_SGX6410_UART
   * @brief 构造函数（UNO/ESP8266 使用 SoftwareSerial）
   * @param sSerial SoftwareSerial 对象指针
   * @param baud 波特率，默认 9600
   * @n 默认串口参数为 9600 8N1。
   */
  DFRobot_SGX6410_UART(SoftwareSerial *sSerial, uint32_t baud = SGX_UART_BAUDRATE);

  /**
   * @fn DFRobot_SGX6410_UART
   * @brief 构造函数（HardwareSerial）
   * @param hSerial HardwareSerial 对象指针，通常为 &Serial1
   * @param baud 波特率，默认 9600
   * @param rxPin RX 引脚，默认 -1，保留已有配置或使用串口核心默认引脚
   * @param txPin TX 引脚，默认 -1，保留已有配置或使用串口核心默认引脚
   * @n 默认串口参数为 9600 8N1。
   * @n ESP32：传入 rxPin/txPin 可改 Serial1 引脚，省略则保留已有引脚配置或使用串口核心默认引脚。
   */
  DFRobot_SGX6410_UART(HardwareSerial *hSerial,
                       uint32_t baud = SGX_UART_BAUDRATE,
                       int8_t rxPin = -1,
                       int8_t txPin = -1);

  /**
   * @fn begin
   * @brief 初始化 UART 并校验传感器
   * @return bool
   * @retval true  初始化成功
   * @retval false 初始化失败
   * @n 按构造函数波特率打开注入的串口，等待 20 ms，清空 RX，再执行基类 begin() 检查。
   * @n ESP32 会在此处应用构造函数里的自定义 RX/TX。
   * @n 若第一条命令失败，发送 0xFF 同步字节。再次调用 begin() 可重试。
   */
  bool begin(void);

  /**
   * @fn DFRobot_SGX6410_I2C
   * @brief 构造函数
   * @param pWire I2C 对象指针，通常为 &Wire
   * @param address 7 位 I2C 地址（bit6:0），默认 0x38
   * @n 可编程 I2C 地址字节：bit7 忽略，bit6:0 为从机地址（0x00-0x7F）。
   * @param sclPin SCL 引脚，默认 0xFF（板级默认）
   * @param sdaPin SDA 引脚，默认 0xFF（板级默认）
   * @n ESP32/ESP8266：传入 sclPin/sdaPin 可改 I2C 引脚，省略则用板级默认脚。
   * @n UNO 等固定 Wire 引脚的主板：不要传 sclPin/sdaPin，begin() 会忽略这两个参数。
   */
  DFRobot_SGX6410_I2C(TwoWire *pWire, uint8_t address = SGX_I2C_ADDR, uint8_t sclPin = 0xFF, uint8_t sdaPin = 0xFF);

  /**
   * @fn begin
   * @brief 初始化 I2C 并校验传感器
   * @return bool
   * @retval true  初始化成功
   * @retval false 初始化失败
   * @n 对注入的总线调用 TwoWire::begin()，再执行基类 begin() 的工作模式读取。
   * @n ESP32/ESP8266 会在此处应用构造函数里的自定义 SCL/SDA。UNO 始终使用默认 Wire 引脚。
   */
  bool begin(void);

  /**
   * @fn begin
   * @brief 初始化通信接口并校验传感器
   * @return bool
   * @retval true  初始化成功
   * @retval false 初始化失败
   * @n 读取当前工作模式，并启动主机侧预热计时。
   */
  bool begin(void);

  /**
   * @fn getProductID
   * @brief 读取产品 ID 和固件版本
   * @param productId 传感器返回的产品标识，期望为 0x2310
   * @param firmwareVer 3 字节固件版本：主版本、次版本、修订版本
   * @return bool
   * @retval true  读取成功
   * @retval false 空指针或读取失败
   * @n 失败时 productId 和 firmwareVer 保持调用前的原值。
   */
  bool getProductID(uint16_t &productId, uint8_t firmwareVer[3]);

  /**
   * @fn setOperationMode
   * @brief 设置 SGX6410 工作模式
   * @param mode 工作模式（见 eSGXMode_t）
   * @n eSgxModeSuspend: 休眠模式
   * @n eSgxModeIaq: IAQ 第二代，采样周期约 3 s，预热 5 min
   * @n eSgxModeUlp: 超低功耗，采样周期约 90 s，预热 15 min
   * @n eSgxModePbaq: PBAQ，采样周期约 5 s，预热 5 min
   * @n 改模式属于维护操作。主机侧会重新预热并清除 isNew/valid。
   * @param save 是否将该模式设为启动模式并保存全部 Flash Shadow 设置，默认 false
   * @return 成功返回 SGX_FDL_OK，模式无效返回 SGX_FDL_PARAMETER_ERROR，
   * @n 通信失败返回 SGX_COMM_ERROR，否则返回 saveFlash() 收到的 FDL 状态。
   * @n 当前工作模式立即切换，本接口不自动复位。
   * @n save=false 时，成功仅表示模式设置命令已发送。
   * @n 保存失败时，当前工作模式已经切换。
   */
  uint8_t setOperationMode(eSGXMode_t mode, bool save = false);

  /**
   * @fn setHumidityCompensate
   * @brief 写入环境相对湿度，供算法补偿
   * @param rh 相对湿度，范围 0.0 到 100.0，单位 %RH
   * @n 主机将 rh 转为 code = (rh / 100) * 255，写入 0-255 湿度码。
   * @n 未调用本函数时，默认补偿值为 50.0 %RH。
   * @return bool
   * @retval true  写入成功
   * @retval false rh 超范围或写入失败
   */
  bool setHumidityCompensate(float rh);

  /**
   * @fn getOperationMode
   * @brief 读取当前 SGX6410 工作模式
   * @return eSGXMode_t 当前工作模式，读取失败时返回 eSgxModeNone（0xFF）
   */
  eSGXMode_t getOperationMode(void);

  /**
   * @fn update
   * @brief 读取测量帧并换算为工程单位
   * @return bool
   * @retval true  帧已读到（重复样本仍返回 true，isNew = false）
   * @retval false 总线错误、解析失败，或当前模式没有测量数据
   * @n 调用 getAllGasData() / getSingleGasData() 前必须先调用本函数。
   * @n IAQ/ULP：IAQ = raw/10，TVOC = raw/100 mg/m3，ETOH = raw/100 ppm，ECO2 = raw ppm，RELIAQ = raw/10
   * @n PBAQ：TVOC = raw/1000 mg/m3，ETOH = raw/1000 ppm；IAQ、ECO2、RELIAQ 为 NAN
   * @n Suspend 模式没有测量数据，返回 false。
   * @n isNew 由样本计数器字节决定。valid 仅在预热完成后为 true。
   */
  bool update(void);

  /**
   * @fn getAllGasData
   * @brief 返回缓存的气体测量结构体
   * @return const sSGXData_t & 最近一次缓存的测量
   * @n 须先调用 update()。用 isNew 判断是否新样本，用 valid 判断预热是否完成。
   */
  const sSGXData_t &getAllGasData(void) const;

  /**
   * @fn getSingleGasData
   * @brief 从缓存中返回单个气体值
   * @param dataType 由 eSGXDataType_t 选择的气体类型
   * @n eSgxDataIaq: IAQ
   * @n eSgxDataTvoc: TVOC
   * @n eSgxDataEtoh: ETOH
   * @n eSgxDataEcO2: 估计 CO2
   * @n eSgxDataReliaq: 相对 IAQ
   * @return float 缓存值；当前模式不支持时为 NAN
   * @n 须先调用 update()。
   */
  float getSingleGasData(eSGXDataType_t dataType);

  /**
   * @fn getTrackingNumber
   * @brief 读取 6 字节 SGX6410 序列号
   * @param number 调用方提供的 6 字节输出缓冲
   * @return bool
   * @retval true  读取成功
   * @retval false 空指针或读取失败
   */
  bool getTrackingNumber(uint8_t *number);

  /**
   * @fn readFlash
   * @brief 从 Flash Shadow 区读取 1 字节
   * @param address 1 字节 Flash Shadow 地址
   * @n SGX_I2C_ADDRESS_SHADOW_ADDR（0x00）：I2C 地址字节
   * @n   bit7 忽略；bit6:0 为 7 位从机地址（0x00-0x7F），默认 0x38
   * @n SGX_STARTUP_MODE_SHADOW_ADDR（0x03）：启动工作模式
   * @param data 读出的字节
   * @return bool
   * @retval true  读取成功
   * @retval false 不支持的地址或读取失败
   * @n 其它地址在主机侧拒绝，不发送命令。
   */
  bool readFlash(uint8_t address, uint8_t &data);

  /**
   * @fn setI2CAddress
   * @brief 设置 I2C 地址到 Flash Shadow，可选保存到 Flash
   * @param address 7 位从机地址，范围 0x00-0x7F；超出范围不发送命令
   * @param save 是否将全部 Flash Shadow 设置保存到 Flash，默认 false
   * @return 成功返回 SGX_FDL_OK，地址无效返回 SGX_FDL_PARAMETER_ERROR，
   * @n 通信失败返回 SGX_COMM_ERROR，否则返回 saveFlash() 收到的 FDL 状态。
   * @n save=false 时，成功仅表示 Shadow 写入命令已发送。
   * @n 新地址保存并复位后生效，本接口不自动复位。
   * @n 复位后需使用新地址重新创建 I2C 对象。
   */
  uint8_t setI2CAddress(uint8_t address, bool save = false);

  /**
   * @fn writeFlash
   * @brief 向 Flash Shadow 区写入 1 字节
   * @param address 1 字节 Flash Shadow 地址
   * @n SGX_I2C_ADDRESS_SHADOW_ADDR（0x00）：I2C 地址字节
   * @n   bit7 忽略；bit6:0 为 7 位从机地址（0x00-0x7F），默认 0x38
   * @n SGX_STARTUP_MODE_SHADOW_ADDR（0x03）：启动工作模式
   * @param data 要写入的字节
   * @n 写 I2C 地址（Shadow 0x00）时：bit7 忽略，bit6:0 为从机地址 0x00-0x7F
   * @return bool
   * @retval true  命令发送成功
   * @retval false 不支持的地址或写入失败
   * @n 新值会停留在 Shadow，直到 saveFlash() 提交。
   * @n 修改 I2C 地址需 saveFlash() 并 reset() 后生效。
   */
  bool writeFlash(uint8_t address, uint8_t data);

  /**
   * @fn saveFlash
   * @brief 将 Flash Shadow 区提交到 Flash
   * @param status 传感器返回的 FDL 状态字节
   * @return bool
   * @retval true  已收到状态字节
   * @retval false 写入或读取失败
   * @n 请求无 payload。响应为 1 个原始 FDL 状态字节。
   * @n 常见码：SGX_FDL_OK（0x00）成功，SGX_FDL_BUSY（0xFF）仍在处理。
   */
  bool saveFlash(uint8_t &status);

  /**
   * @fn readDebugData
   * @brief 读取原始调试字节
   * @param data 调用方提供的输出缓冲
   * @param size 要读取的 payload 字节数，最大 100
   * @return bool
   * @retval true  读取成功
   * @retval false 非法长度/指针或读取失败
   */
  bool readDebugData(uint8_t *data, size_t size);

  /**
   * @fn reset
   * @brief 复位传感器
   * @return bool
   * @retval true  复位命令发送成功
   * @retval false 写入失败
   * @n 命令 0x8F 无响应。复位后需等待启动，再调用 begin()。
   */
  bool reset();

  /**
   * @fn isDataNew
   * @brief 检查最近一次 update 是否包含新样本
   * @return bool 缓存的 isNew 标志
   * @n 须先调用 update()。
   */
  bool isDataNew(void);

  /**
   * @fn isDataValid
   * @brief 检查缓存的测量数据是否有效
   * @return bool 缓存的 valid 标志
   * @n 须先调用 update()。
   * @n valid 仅在预热完成后采集的新样本为 true：IAQ 5 min、ULP 15 min、PBAQ 5 min。
   */
  bool isDataValid(void);
```

## 兼容性

| 主板         | 通过 | 未通过 | 未测试 | 备注 |
| ------------ | :--: | :----: | :----: | ---- |
| Arduino uno  |      √ |        |      |      |
| Mega2560     |      √  |        |     |      |
| Leonardo     |      √  |        |      |      |
| ESP32        |      √  |        |     |      |
| ESP8266      |      √  |        |      |      |

## 历史

- 2026/09/03 - V1.0.0 版本

## 创作者

Written by PLELES(li.jia@dfrobot.com), 2026. (欢迎访问我们的[网站](https://www.dfrobot.com/))
