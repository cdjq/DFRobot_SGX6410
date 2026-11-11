# DFRobot_SGX6410
- [English Version](./README.md)

DFRobot_SGX6410 是 SGX6410 MEMS 空气质量传感器（SKU: SEN0771）的树莓派 Python 库。传感器用于评估室内空气质量，可输出 IAQ、TVOC、乙醇当量（ETOH）、估计 CO2（eCO2）和相对 IAQ（RelIAQ）。

主机通过 I2C 或 UART 与 SGX6410 通信，两者使用同一套命令帧。I2C 7 位地址为 `0x38`（树莓派通常为总线 1）；UART 默认 9600 8N1，串口为 `/dev/ttyAMA0`。

典型流程：调用 `begin()` 校验产品 ID `0x2310` 并读取 6 字节序列号，设置工作模式（IAQ / ULP / PBAQ / Suspend），等待预热完成（`valid` 为 true），再调用 `update()` 读取缓存的气体数据。湿度可按 0–100 %RH 手动写入。示例里 `ctype = 0` 为 I2C，非 0 为 UART。

## 产品链接

```
SKU: SEN0771
```

## 目录

  * [概述](#概述)
  * [库安装](#库安装)
  * [方法](#方法)
  * [兼容性](#兼容性)
  * [历史](#历史)
  * [创作者](#创作者)

## 概述

* 双接口：I2C（`smbus2` 原始 I2C 事务，默认地址 `0x38`）或 UART 命令帧（`/dev/ttyAMA0`，9600 8N1）
* `begin()` 校验产品 ID `0x2310` 并读取 6 字节序列号
* 工作模式：Suspend；IAQ（约 3 s 采样，预热约 5 min）；ULP（约 90 s 采样，预热约 15 min）；PBAQ（约 5 s 采样，预热约 5 min）
* `update()` 按模式换算：IAQ/ULP 为 IAQ=raw/10、TVOC=raw/100 mg/m³、ETOH=raw/100 ppm、eCO2=raw ppm、RelIAQ=raw/10；PBAQ 仅输出 TVOC/ETOH（raw/1000），其余为 NAN
* 使用数据前请检查 `is_new`（是否新样本）和 `valid`（预热是否完成）
* 手动湿度补偿：写入 0–100 %RH（内部存为 0–255 码）
* 支持 Flash Shadow 中的 I2C 地址与启动模式

## 库安装

1. 在树莓派上打开 I2C 和/或 UART（`raspi-config`）。
2. 安装依赖：

```
sudo apt-get update
sudo apt-get install python3-smbus2 python3-serial
```

3. 把本目录 `python/raspberrypi/` 拷到树莓派，然后运行示例：

```
python3 examples/polling_read_data/polling_read_data.py
```

`ctype = 0` 为 I2C，非 0 为 UART。

## 方法

```python
  def __init__(self, bus, addr)
    '''!
      @brief 构造函数
      @param bus I2C 总线号，树莓派通常为 1
      @param addr 7 位 I2C 地址，默认 0x38
    '''

  def __init__(self, baud, addr, port)
    '''!
      @brief 构造函数
      @param baud 波特率，默认 9600
      @param addr 为 API 对称保留；UART 帧不使用从机地址
      @param port Linux 串口设备，默认 /dev/ttyAMA0
      @n 默认串口参数为 9600 8N1。
    '''

  def begin(self)
    '''!
      @brief 初始化 UART 并校验传感器
      @return 成功 True，失败 False
      @n 清空 RX，等待 20 ms，再执行基类 begin() 检查。
      @n 若第一条命令失败，发送 0xFF 同步字节。再次调用 begin() 可重试。
    '''

  def begin(self)
    '''!
      @brief 初始化通信接口并校验传感器
      @return 成功 True，失败 False
      @n 读取产品 ID（期望为 0x2310）和 6 字节序列号。
      @n 两项读取均成功才认为传感器在线。
      @n 然后读取当前工作模式，并启动主机侧预热计时。
    '''

  def get_product_id(self)
    '''!
      @brief 读取产品 ID 和固件版本
      @return 成功返回 (product_id, (major, minor, revision))，失败返回 None
      @n product_id 期望为 0x2310。
    '''

  def set_operation_mode(self, mode, save=False)
    '''!
      @brief 设置 SGX6410 工作模式
      @param mode 工作模式
      @param save True 时保存启动模式到 Flash，默认 False
      @n SGX_MODE_SUSPEND: 休眠模式
      @n SGX_MODE_IAQ: IAQ 第二代，采样周期约 3 s，预热 5 min
      @n SGX_MODE_ULP: 超低功耗，采样周期约 90 s，预热 15 min
      @n SGX_MODE_PBAQ: PBAQ，采样周期约 5 s，预热 5 min
      @n 改模式属于维护操作。主机侧会重新预热并清除 is_new/valid。
      @return save=False：True/False；save=True：FDL 状态码，通信失败为 None
    '''

  def set_humidity_compensate(self, rh)
    '''!
      @brief 写入环境相对湿度，供算法补偿
      @param rh 相对湿度，范围 0.0 到 100.0，单位 %RH
      @n 主机将 rh 转为 code = (rh / 100) * 255，写入 0-255 湿度码。
      @n 未调用本函数时，默认补偿值为 50.0 %RH。
      @return 成功 True，失败 False
    '''

  def get_operation_mode(self)
    '''!
      @brief 读取当前 SGX6410 工作模式
      @return 当前模式命令字节
      @n 读取失败时返回 SGX_MODE_NONE（0xFF）。
    '''

  def update(self)
    '''!
      @brief 读取测量帧并换算为工程单位
      @return 帧已读到则 True（重复样本仍返回 True，is_new = False）
      @n 调用 get_all_gas_data() / get_single_gas_data() 前必须先调用本函数。
      @n IAQ/ULP：IAQ = raw/10，TVOC = raw/100 mg/m3，ETOH = raw/100 ppm，ECO2 = raw ppm，RELIAQ = raw/10
      @n PBAQ：TVOC = raw/1000 mg/m3，ETOH = raw/1000 ppm；IAQ、ECO2、RELIAQ 为 NAN
      @n Suspend 模式没有测量数据，返回 False。
      @n is_new 由样本计数器字节决定。valid 仅在预热完成后为 true。
    '''

  def get_all_gas_data(self)
    '''!
      @brief 返回缓存的气体测量结构
      @return DFRobot_SGX6410_Data 最近一次缓存的测量
      @n 须先调用 update()。用 is_new 判断是否新样本，用 valid 判断预热是否完成。
    '''

  def get_single_gas_data(self, data_type)
    '''!
      @brief 从缓存中返回单个气体值
      @param data_type SGX_DATA_IAQ / SGX_DATA_TVOC / SGX_DATA_ETOH / SGX_DATA_ECO2 / SGX_DATA_RELIAQ
      @return 缓存值；当前模式不支持时为 NAN
      @n 须先调用 update()。
    '''

  def get_tracking_number(self)
    '''!
      @brief 读取 6 字节 SGX6410 序列号
      @return 成功返回 6 字节 bytes，失败返回 None
    '''

  def set_i2c_address(self, address, save=False)
    '''!
      @brief 设置 I2C 地址，可选保存到 Flash
      @param address 7 位从机地址（0x00-0x7F）
      @param save 是否保存到 Flash，默认 False
      @return FDL 状态码，通信失败返回 None
      @n 保存并复位传感器后，使用新地址重新连接。
    '''

  def read_debug_data(self, size)
    '''!
      @brief 读取原始调试字节
      @param size 要读取的 payload 字节数，最大 100
      @return 成功返回 bytes，失败返回 None
    '''

  def reset(self)
    '''!
      @brief 复位传感器
      @return 成功 True，失败 False
      @n 命令 0x8F 无响应。复位后需等待启动，再调用 begin()。
    '''

  def is_data_new(self)
    '''!
      @brief 检查最近一次 update 是否包含新样本
      @return 缓存的 is_new 标志
      @n 须先调用 update()。
    '''

  def is_data_valid(self)
    '''!
      @brief 检查缓存的测量数据是否有效
      @return 缓存的 valid 标志
      @n 须先调用 update()。
      @n valid 仅在预热完成后采集的新样本为 true：IAQ 5 min、ULP 15 min、PBAQ 5 min。
    '''
```

## 兼容性

| Board        | Work Well | Work Wrong | Untested | Remarks |
| ------------ | :-------: | :--------: | :------: | ------- |
| RaspberryPi4 |     √     |            |          |         |

* Python Version

| Python  | Work Well | Work Wrong | Untested | Remarks |
| ------- | :-------: | :--------: | :------: | ------- |
| Python3 |           |            |    √     |         |

## 历史

- 2026/09/04 - V1.0.0 版本

## 创作者

Written by PLELES(li.jia@dfrobot.com), 2026. (Welcome to our [website](https://www.dfrobot.com/))
