# DFRobot_SGX6410
- [中文版](./README_CN.md)

DFRobot_SGX6410 is the Raspberry Pi Python library for the SGX6410 MEMS air quality sensor (SKU: SEN0771). The sensor estimates indoor air quality and reports IAQ, TVOC, ethanol equivalent (ETOH), estimated CO2 (eCO2), and relative IAQ (RelIAQ).

The host talks to the SGX6410 over I2C or UART using the same command frame. I2C uses 7-bit address `0x38` on the Pi I2C bus (typically bus 1). UART defaults to 9600 8N1 on `/dev/ttyAMA0`.

Typical workflow: call `begin()` to verify product ID `0x2310` and the 6-byte tracking number, set an operation mode (IAQ / ULP / PBAQ / Suspend), wait until warm-up completes (`valid` is true), then call `update()` and read the cached gas data. Humidity can be written manually as 0–100 %RH. In the examples, `ctype = 0` selects I2C and any other value selects UART.

## Product Link

```
SKU: SEN0771
```

## Table of Contents

  * [Summary](#summary)
  * [Installation](#installation)
  * [Methods](#methods)
  * [Compatibility](#compatibility)
  * [History](#history)
  * [Credits](#credits)

## Summary

* Dual interface: I2C (`smbus2` raw I2C, default address `0x38`) or UART command frames (`/dev/ttyAMA0`, 9600 8N1)
* `begin()` verifies product ID `0x2310` and reads the 6-byte tracking number
* Operation modes: Suspend; IAQ (~3 s sample, ~5 min warm-up); ULP (~90 s sample, ~15 min warm-up); PBAQ (~5 s sample, ~5 min warm-up)
* `update()` scales the measurement frame: IAQ/ULP use IAQ=raw/10, TVOC=raw/100 mg/m³, ETOH=raw/100 ppm, eCO2=raw ppm, RelIAQ=raw/10; PBAQ only reports TVOC/ETOH (raw/1000), other fields are NAN
* Check `is_new` for a fresh sample and `valid` for warm-up completion before using the readings
* Manual humidity compensation: write 0–100 %RH (stored as code 0–255)
* Flash Shadow I2C address and startup mode

## Installation

1. Enable I2C and/or UART on the Raspberry Pi (`raspi-config`).
2. Install dependencies:

```
sudo apt-get update
sudo apt-get install python3-smbus2 python3-serial
```

3. Copy this `python/raspberrypi/` folder onto the Pi, then run an example:

```
python3 examples/polling_read_data/polling_read_data.py
```

Set `ctype = 0` for I2C or a non-zero value for UART.

## Methods

```python
  def __init__(self, bus, addr)
    '''!
      @brief Constructor
      @param bus I2C bus number, typically 1 on Raspberry Pi
      @param addr 7-bit I2C address, default 0x38
    '''

  def __init__(self, baud, addr, port)
    '''!
      @brief Constructor
      @param baud Baud rate, default 9600
      @param addr Kept for API symmetry; UART frames do not use a slave address
      @param port Linux serial device, default /dev/ttyAMA0
      @n Default line settings are 9600 8N1.
    '''

  def begin(self)
    '''!
      @brief Initialize UART and then verify the sensor
      @return True on success, False on failure
      @n Flushes RX, waits 20 ms, then runs the base begin() check.
      @n If the first command fails, sends the 0xFF sync byte. Call begin() again to retry.
    '''

  def begin(self)
    '''!
      @brief Initialize the communication interface and verify the sensor
      @return True on success, False on failure
      @n Reads product ID (expected 0x2310) and the six-byte tracking number.
      @n Both reads must succeed before the sensor is treated as online.
      @n Then reads the current operation mode and starts the host-side warm-up timer.
    '''

  def get_product_id(self)
    '''!
      @brief Read product ID and firmware version
      @return (product_id, (major, minor, revision)) on success, None on failure
      @n product_id is expected to be 0x2310.
    '''

  def set_operation_mode(self, mode)
    '''!
      @brief Set SGX6410 operation mode
      @param mode Operation mode
      @n SGX_MODE_SUSPEND: Suspend mode
      @n SGX_MODE_IAQ: IAQ 2nd generation, about 3 s sample period, 5 min warm-up
      @n SGX_MODE_ULP: Ultra-low power, about 90 s sample period, 15 min warm-up
      @n SGX_MODE_PBAQ: PBAQ, about 5 s sample period, 5 min warm-up
      @n Mode change is a maintenance action. The host restarts warm-up and clears is_new/valid.
      @return True on success, False on failure
    '''

  def set_humidity_compensate(self, rh)
    '''!
      @brief Write ambient relative humidity for algorithm compensation
      @param rh Relative humidity, range 0.0 to 100.0, unit %RH
      @n Host converts rh to code = (rh / 100) * 255 and writes the 0-255 code.
      @n The default compensation value is 50.0 %RH when this method is not called.
      @return True on success, False on failure
    '''

  def get_operation_mode(self)
    '''!
      @brief Read the current SGX6410 operation mode
      @return Current mode command byte
      @n Returns SGX_MODE_NONE (0xFF) when the read fails.
    '''

  def update(self)
    '''!
      @brief Read the measurement frame and scale it to engineering units
      @return True if the frame was read (a repeated sample still returns True with is_new = False)
      @n Call this before get_all_gas_data() / get_single_gas_data().
      @n IAQ/ULP: IAQ = raw/10, TVOC = raw/100 mg/m3, ETOH = raw/100 ppm, ECO2 = raw ppm, RELIAQ = raw/10
      @n PBAQ: TVOC = raw/1000 mg/m3, ETOH = raw/1000 ppm; IAQ, ECO2 and RELIAQ are NAN
      @n Suspend mode has no measurement payload and return False.
      @n is_new follows the sample-counter byte. valid is true only after mode warm-up.
    '''

  def get_all_gas_data(self)
    '''!
      @brief Return the cached gas measurement structure
      @return DFRobot_SGX6410_Data Latest cached measurement
      @n Must call update() first. Check is_new for a new sample and valid for warm-up completion.
    '''

  def get_single_gas_data(self, data_type)
    '''!
      @brief Return one value from the cached measurement
      @param data_type SGX_DATA_IAQ / SGX_DATA_TVOC / SGX_DATA_ETOH / SGX_DATA_ECO2 / SGX_DATA_RELIAQ
      @return Cached value, or NAN if unsupported in the current mode
      @n Must call update() first.
    '''

  def get_tracking_number(self)
    '''!
      @brief Read the six-byte SGX6410 tracking number
      @return Six-byte bytes on success, None on failure
    '''

  def read_flash(self, address)
    '''!
      @brief Read one byte from the Flash Shadow area
      @param address Flash Shadow address: 0x00 I2C address, 0x03 startup mode
      @return Byte value on success, None on failure
      @n Other addresses are rejected on the host and send no command.
    '''

  def write_flash(self, address, data)
    '''!
      @brief Write one byte to the Flash Shadow area
      @param address Flash Shadow address: 0x00 I2C address, 0x03 startup mode
      @param data Byte to write
      @return True on success, False on failure
      @n The new value stays in Shadow until save_flash() commits it.
      @n Changing the I2C address takes effect after save_flash() and reset().
    '''

  def save_flash(self)
    '''!
      @brief Commit the Flash Shadow area to Flash
      @return FDL status byte on success, None on failure
      @n Typical codes: sgx_fdl_ok (0x00) success, sgx_fdl_busy (0xFF) still processing.
    '''

  def read_debug_data(self, size)
    '''!
      @brief Read raw debug bytes
      @param size Number of payload bytes to read, up to 100
      @return bytes on success, None on failure
    '''

  def reset(self)
    '''!
      @brief Reset the sensor
      @return True on success, False on failure
      @n Command 0x8F has no response. After reset, wait for start-up and call begin() again.
    '''

  def is_data_new(self)
    '''!
      @brief Check whether the latest update contains a new sample
      @return Cached is_new flag
      @n Must call update() first.
    '''

  def is_data_valid(self)
    '''!
      @brief Check whether cached measurement data is valid
      @return Cached valid flag
      @n Must call update() first.
      @n valid is true only for a new sample acquired after warm-up: 5 min IAQ, 15 min ULP, 5 min PBAQ.
    '''
```

## Compatibility

| Board        | Work Well | Work Wrong | Untested | Remarks |
| ------------ | :-------: | :--------: | :------: | ------- |
| RaspberryPi2 |           |            |    √     |         |
| RaspberryPi3 |           |            |    √     |         |
| RaspberryPi4 |           |            |    √     |         |

* Python Version

| Python  | Work Well | Work Wrong | Untested | Remarks |
| ------- | :-------: | :--------: | :------: | ------- |
| Python2 |           |            |    √     |         |
| Python3 |           |            |    √     |         |

## History

- 2026/09/04 - V1.0.0 version

## Credits

Written by PLELES(li.jia@dfrobot.com), 2026. (Welcome to our [website](https://www.dfrobot.com/))
