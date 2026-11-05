# Arduino 库审核报告

库目录：/workspace/DFRobot_SGX6410

先写规范：名字、注释和库的写法。再写体验：示例是否合理、用户能不能按用途直接用上。示例是并列的，不排先烧哪一份。

审核提交：`09bb407`（删除所有sensorClean事务）

## 规范

### `enum-name`

**结论：** 不满足

**reason：** 枚举类型须为 `eXxx_t`。当前 `sSGXMode_t`、`sSGXDataType_t` 误用结构体前缀 `s`；结构体 `sSGXData_t` 正确。嵌套在类内时示例以 `DFRobot_SGX6410::sSGXMode_t` 引用，更名后注释 / README 方法块 / `keywords.txt` 的 KEYWORD1 须只用新名。

**suggestion：**

```cpp
/**
 * @enum eSGXMode_t
 * @brief SGX6410 operation mode
 * @n Line values match the sensor command bytes.
 */
typedef enum {
  SGX_MODE_SUSPEND = 0x00, /**< Suspend mode */
  SGX_MODE_IAQ     = 0x01, /**< IAQ 2nd generation mode, poll about every 3 s, warm-up 5 min */
  SGX_MODE_ULP     = 0x02, /**< Ultra-low power mode, poll about every 90 s, warm-up 15 min */
  SGX_MODE_PBAQ    = 0x05, /**< PBAQ mode, poll about every 5 s, warm-up 5 min */
} eSGXMode_t;

/**
 * @enum eSGXDataType_t
 * @brief Select one cached gas value
 */
typedef enum {
  SGX_DATA_IAQ    = 0, /**< IAQ, UBA level */
  SGX_DATA_TVOC   = 1, /**< TVOC */
  SGX_DATA_ETOH   = 2, /**< Ethanol equivalent */
  SGX_DATA_ECO2   = 3, /**< Estimated CO2 */
  SGX_DATA_RELIAQ = 4  /**< Relative IAQ */
} eSGXDataType_t;
```

方法签名与注释中同步：

```cpp
bool setOperationMode(eSGXMode_t mode);
float getSingleGasData(eSGXDataType_t dataType);
```

`keywords.txt` KEYWORD1：

```text
eSGXMode_t            KEYWORD1
eSGXDataType_t        KEYWORD1
```

README / README_CN Methods 块内凡 `sSGXMode_t` / `sSGXDataType_t` 均改为 `eSGXMode_t` / `eSGXDataType_t`。示例中 `DFRobot_SGX6410::sSGXData_t` 在结构体仍属类内时保持，若结构体随 `header-layout` 外置则改为 `sSGXData_t`；模式常量在枚举外置后写 `SGX_MODE_IAQ`（或保留类作用域时用新类型名）。

### `file-header`

**结论：** 不满足

**reason：** 头文件 `@details` 使用禁词套话 “Header for …”。

**suggestion：**

```cpp
/*!
 * @file DFRobot_SGX6410.h
 * @brief Arduino driver for the SGX6410 MEMS air quality sensor (SEN0771)
 * @details Declares I2C/UART transports, command helpers, operation-mode enums,
 * @n cached gas measurement struct, and public APIs for mode, humidity, GPIO and Flash Shadow.
 * @copyright Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license The MIT License (MIT)
 * @author PLELES(li.jia@dfrobot.com)
 * @version V1.0.0
 * @date 2026-09-03
 * @url https://github.com/cdjq/DFRobot_SGX6410
 */
```

### `header-layout`

**结论：** 不满足

**reason：** 文件级顺序应为 guard → Arduino.h → 平台头 → 宏 → 枚举 → 结构体 → 类。现状宏已在类外，但 `sSGXMode_t` / `sSGXDataType_t` / `sSGXData_t` 仍嵌在 `DFRobot_SGX6410` 的 `public:` 内。

**suggestion：**

```cpp
#ifndef __DFROBOT_SGX6410_H__
#define __DFROBOT_SGX6410_H__

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#if defined(ARDUINO_AVR_UNO) || defined(ESP8266)
#include <SoftwareSerial.h>
#endif

#define SGX_TIMEOUT_MS               100     ///< Command response timeout in ms
#define SGX_I2C_ADDR                 0x38    ///< Default 7-bit I2C slave address (bits 6:0)
#define SGX_UART_BAUDRATE            9600    ///< Default UART baud rate
#define SGX_UART_SYNC_BYTE           0xFF    ///< UART command-frame synchronization byte
#define SGX_WARMUP_IAQ_MS            300000UL  ///< IAQ 2nd Gen warm-up time in ms
#define SGX_WARMUP_ULP_MS            900000UL  ///< ULP warm-up time in ms
#define SGX_WARMUP_PBAQ_MS           300000UL  ///< PBAQ warm-up time in ms
#define SGX_I2C_ADDRESS_SHADOW_ADDR  0x00    ///< Flash Shadow: I2C address byte (bit7 don't care, bits 6:0 slave address)
#define SGX_STARTUP_MODE_SHADOW_ADDR 0x03    ///< Flash Shadow address for startup mode
/* …其余 SGX_CMD_* / SGX_FDL_* 宏保持原样… */

/**
 * @enum eSGXMode_t
 * @brief SGX6410 operation mode
 * @n Line values match the sensor command bytes.
 */
typedef enum {
  SGX_MODE_SUSPEND = 0x00, /**< Suspend mode */
  SGX_MODE_IAQ     = 0x01, /**< IAQ 2nd generation mode, poll about every 3 s, warm-up 5 min */
  SGX_MODE_ULP     = 0x02, /**< Ultra-low power mode, poll about every 90 s, warm-up 15 min */
  SGX_MODE_PBAQ    = 0x05, /**< PBAQ mode, poll about every 5 s, warm-up 5 min */
} eSGXMode_t;

/**
 * @enum eSGXDataType_t
 * @brief Select one cached gas value
 */
typedef enum {
  SGX_DATA_IAQ    = 0, /**< IAQ, UBA level */
  SGX_DATA_TVOC   = 1, /**< TVOC */
  SGX_DATA_ETOH   = 2, /**< Ethanol equivalent */
  SGX_DATA_ECO2   = 3, /**< Estimated CO2 */
  SGX_DATA_RELIAQ = 4  /**< Relative IAQ */
} eSGXDataType_t;

/**
 * @struct sSGXData_t
 * @brief Parsed and cached gas measurement data
 */
typedef struct {
  float IAQ;     /**< IAQ, UBA level; NAN in PBAQ or Suspend */
  float TVOC;    /**< TVOC, mg/m3 */
  float ETOH;    /**< Ethanol equivalent, ppm */
  float ECO2;    /**< Estimated CO2, ppm; NAN in PBAQ or Suspend */
  float RELIAQ;  /**< Relative IAQ; NAN in PBAQ or Suspend */
  bool  isNew;   /**< True when the latest frame has a new sample counter */
  bool  valid;   /**< True when the sample was acquired after mode warm-up */
} sSGXData_t;

class DFRobot_SGX6410 {
public:
  DFRobot_SGX6410();
  virtual ~DFRobot_SGX6410() {}
  virtual bool begin(void);
  bool getProductID(uint16_t &productId, uint8_t firmwareVer[3]);
  bool setOperationMode(eSGXMode_t mode);
  bool setHumidityCompensate(float rh);
  uint8_t getOperationMode(void);
  bool update(void);
  const sSGXData_t &getAllGasData(void) const { return _gasData; }
  float getSingleGasData(eSGXDataType_t dataType);
  bool getTrackingNumber(uint8_t *number);
  bool configGPIO(uint8_t config);
  bool getGPIO(uint8_t &value);
  bool setGPIO();
  bool clearGPIO();
  bool readFlash(uint8_t address, uint8_t &data);
  bool writeFlash(uint8_t address, uint8_t data);
  bool saveFlash(uint8_t &status);
  bool readDebugData(uint8_t *data, size_t size);
  bool reset();
  bool isDataNew(void);
  bool isDataValid(void);
protected:
  virtual bool interfaceWrite(const uint8_t *data, size_t len) = 0;
  virtual int16_t interfaceRead(uint8_t *data, size_t len) = 0;
  bool writeCmd(uint8_t cmd, const uint8_t *data, size_t size);
  bool readCmd(uint8_t cmd, uint8_t *data, size_t size);
  bool parseMeasurement(const uint8_t *values, size_t size, sSGXData_t &data);
private:
  eSGXMode_t _mode;
  sSGXData_t _gasData;
  uint8_t _sampleCounter;
  bool _sampleCounterNew;
  uint32_t _modeStartTime;
  bool _modeTimerValid;
};
```

（`DFRobot_SGX6410_I2C` / `DFRobot_SGX6410_UART` 仍按 public → protected → private；类外勿再嵌套枚举/结构体。）

### `class-sections`

**结论：** 不满足

**reason：** 规范要求协议/内部读写在 `protected`，总线指针/缓存/配置在 `private`。基类中 `_mode`、`_gasData`、`_sampleCounter`、`_sampleCounterNew`、`_modeStartTime`、`_modeTimerValid` 现为 `protected`。子类总线指针已在 `private`，符合。

**suggestion：**

```cpp
protected:
  virtual bool interfaceWrite(const uint8_t *data, size_t len) = 0;
  virtual int16_t interfaceRead(uint8_t *data, size_t len) = 0;
  bool writeCmd(uint8_t cmd, const uint8_t *data, size_t size);
  bool readCmd(uint8_t cmd, uint8_t *data, size_t size);
  bool parseMeasurement(const uint8_t *values, size_t size, sSGXData_t &data);
private:
  eSGXMode_t _mode;
  sSGXData_t _gasData;
  uint8_t _sampleCounter;
  bool _sampleCounterNew;
  uint32_t _modeStartTime;
  bool _modeTimerValid;
```

### `readme-usable`

**结论：** 不满足

**reason：**  
1. 已有 `## Product Link` / `## 产品链接`（含 SKU `SEN0771`），但 Table of Contents / 目录未列出该章，目录与标题未互指齐全。  
2. 章节顺序、库安装两条、兼容性主板表（√ / 未测试、无总线列）、History / Credits 格式具备；无多余「接线」「示例」顶层章。

**suggestion：**

```markdown
## Product Link ([https://www.dfrobot.com](https://www.dfrobot.com))

```
SKU: SEN0771
```

## Table of Contents

  * [Product Link](#product-link-httpswwwdfrobotcom)
  * [Summary](#summary)
  * [Installation](#installation)
  * [Methods](#methods)
  * [Compatibility](#compatibility)
  * [History](#history)
  * [Credits](#credits)
```

```markdown
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
```

### `example-wiring`

**结论：** 不满足

**reason：**  
1. `polling_readData` 默认 `SGX6410_COMM_I2C`，I2C 分支无接线表；UART 表仅在 UART 分支。  
2. `interrupt_readData` 使用 `interruptPin`（ESP32=5、ESP8266=12，其它板中断号 0→UNO D2），接线表只有 UART 电源与收发脚，未列 INT，与代码用到的中断脚不一致。  
3. 其余示例 UART 表与 Soft/Hard 构造引脚一致。

**suggestion：**

`polling_readData.ino`（及同样双接口的示例）在 I2C 分支补表：

```cpp
#elif defined(SGX6410_COMM_I2C)
/* ---------------------------------------------------------------------------------------------------------
 *   SGX6410  |      MCU       | Leonardo/Mega2560/M0 | UNO | ESP8266 | ESP32 | Other Wire boards
 *     VCC    |      3.3V      |        3.3V           | 3.3V|   3.3V  | 3.3V  |        3.3V
 *     GND    |      GND       |        GND            | GND |   GND   | GND   |        GND
 *     SCL    |      SCL       |        SCL            | SCL |  GPIO5  | SCL   |        SCL
 *     SDA    |      SDA       |        SDA            | SDA |  GPIO4  | SDA   |        SDA
 * --------------------------------------------------------------------------------------------------------- */
  DFRobot_SGX6410_I2C sgx6410(&Wire, SGX_I2C_ADDR);
```

`interrupt_readData.ino` 接线表增加 INT，并与 `interruptPin` 一致：

```cpp
/* ---------------------------------------------------------------------------------------------------------
 *   SGX6410  |      MCU       | Leonardo/Mega2560/M0 | UNO | ESP8266 | ESP32 | Other Serial1 boards
 *     VCC    |      3.3V      |        3.3V           | 3.3V|   3.3V  | 3.3V  |        3.3V
 *     GND    |      GND       |        GND            | GND |   GND   | GND   |        GND
 *      RX    |      TX        |     Serial1 TX1       |  7  |  GPIO5  |  37   |     Serial1 TX
 *      TX    |      RX        |     Serial1 RX1       |  6  |  GPIO4  |  38   |     Serial1 RX
 *      INT   |   interrupt    |          2            |  2  | GPIO12  |   5   |   board INT pin
 * --------------------------------------------------------------------------------------------------------- */
#if defined(ESP32)
const uint8_t interruptPin = 5;   // D5 / GPIO5
#elif defined(ESP8266)
const uint8_t interruptPin = 12;  // D6 / GPIO12
#else
const uint8_t interruptPin = 2;   // UNO / Mega interrupt 0 → digital pin 2
#endif
```

（非 ESP 分支建议改为 `attachInterrupt(digitalPinToInterrupt(interruptPin), sgxInterrupt, RISING);`，与表中 INT=2 一致。）

### `keywords-literal`

**结论：** 不满足

**reason：** LITERAL1 未列枚举成员 `SGX_MODE_*`、`SGX_DATA_*`。公开 Flash 地址宏 `SGX_I2C_ADDRESS_SHADOW_ADDR`、`SGX_STARTUP_MODE_SHADOW_ADDR` 亦未收录（示例与 API 均使用）。

**suggestion：**

```text
SGX_MODE_SUSPEND            LITERAL1
SGX_MODE_IAQ                LITERAL1
SGX_MODE_ULP                LITERAL1
SGX_MODE_PBAQ               LITERAL1
SGX_DATA_IAQ                LITERAL1
SGX_DATA_TVOC               LITERAL1
SGX_DATA_ETOH               LITERAL1
SGX_DATA_ECO2               LITERAL1
SGX_DATA_RELIAQ             LITERAL1
SGX_I2C_ADDRESS_SHADOW_ADDR LITERAL1
SGX_STARTUP_MODE_SHADOW_ADDR LITERAL1
```

## 体验

### `example-dirname`

**结论：** 不满足

**reason：** 示例目录须 camelCase 且能看出用途。`polling_readData`、`interrupt_readData` 含下划线；`GPIOControl`、`changeI2CAddress`、`changeStartupMode` 已符合。功能不同，保持并列，勿合并。

**suggestion：**

```text
examples/pollingReadData/pollingReadData.ino
examples/interruptReadData/interruptReadData.ino
```

（同步修改 `.ino` 内 `@file` 文件名；`GPIOControl` / `changeI2CAddress` / `changeStartupMode` 目录名可保持。）

### `demo-experience`

**结论：** 不满足

**reason：** `changeI2CAddress.ino` 中 `OLD_I2C_ADDRESS = 0x39`、`NEW_I2C_ADDRESS = 0x38`。出厂默认地址为 `0x38`，按当前常量在 I2C 下 `begin()` 会对不上模块，示例无法按目录用途跑通；且把地址改回默认也不符合「改地址」演示习惯。其余示例串口带名称/单位（IAQ/RelIAQ 无量纲除外），未在 loop 中反复写 Flash。

**suggestion：**

```cpp
/*!
 * @file changeI2CAddress.ino
 * @brief Change and persist the SGX6410 I2C slave address.
 * @details Writes the address to Flash Shadow, saves it to data Flash, then resets the SGX6410.
 * @n Do not reset or remove power while saveFlash() is writing data Flash.
 * @copyright Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license The MIT License (MIT)
 * @author [PLELES](li.jia@dfrobot.com)
 * @version V1.0.0
 * @date 2026-09-03
 * @url https://github.com/cdjq/DFRobot_SGX6410
 */
#include "DFRobot_SGX6410.h"

// Default on the module is 0x38. Set OLD to the address currently answering on the bus.
// Set NEW to the address you want to persist (bits 6:0, 0x00-0x7F).
const uint8_t OLD_I2C_ADDRESS = 0x38;  // default: factory address
const uint8_t NEW_I2C_ADDRESS = 0x39;  // default: example target address
```

（其后 `writeFlash` / `saveFlash` / `reset` 流程可保持；I2C 构造继续使用 `OLD_I2C_ADDRESS`。）

## 计数

- 要改：9
- 已符合：22
- 不适用：1
