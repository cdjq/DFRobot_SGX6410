# DFRobot_SGX6410
- [中文版](./README_CN.md)

DFRobot_SGX6410 is the Arduino library for the SGX6410 MEMS air quality sensor (SKU: SEN0771). The sensor estimates indoor air quality and reports IAQ, TVOC, ethanol equivalent (ETOH), estimated CO2 (eCO2), and relative IAQ (RelIAQ).

The host talks to the SGX6410 over I2C or UART using the same command frame, not a Gravity bridge register map. I2C uses a 7-bit slave address in bits 6:0 (default `0x38`); bit 7 of the programmable address byte is don't care. UART defaults to 9600 8N1.

Typical workflow: call `begin()` to read the current operation mode and initialize the warm-up timer, set an operation mode (IAQ / ULP / PBAQ / Suspend), wait until warm-up completes (`valid` is true), then call `update()` and read the cached gas data. Humidity can be written manually as 0–100 %RH.

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

## Summary

* Dual interface: I2C (7-bit slave address in bits 6:0, default `0x38`; bit 7 = don't care) or UART command frames, 9600 8N1
* `begin()` reads the current operation mode and initializes the warm-up timer
* Operation modes: Suspend; IAQ (~3 s sample, ~5 min warm-up); ULP (~90 s sample, ~15 min warm-up); PBAQ (~5 s sample, ~5 min warm-up)
* `update()` scales the measurement frame: IAQ/ULP use IAQ=raw/10, TVOC=raw/100 mg/m³, ETOH=raw/100 ppm, eCO2=raw ppm, RelIAQ=raw/10; PBAQ only reports TVOC/ETOH (raw/1000), other fields are NAN
* Check `isNew` for a fresh sample and `valid` for warm-up completion before using the readings
* Manual humidity compensation: write 0–100 %RH (stored as code 0–255)
* Flash Shadow I2C address and startup mode

## Installation

* Search `DFRobot_SGX6410` in the Arduino IDE Library Manager and install it.
* Or download this repository, unzip it into the Arduino `libraries` folder, then open an example from `examples`.

## Methods

```C++
  /**
   * @fn DFRobot_SGX6410_UART
   * @brief Constructor (UNO/ESP8266 uses SoftwareSerial)
   * @param sSerial SoftwareSerial object pointer
   * @param baud Baud rate, default 9600
   * @n Default line settings are 9600 8N1.
   */
  DFRobot_SGX6410_UART(SoftwareSerial *sSerial, uint32_t baud = SGX_UART_BAUDRATE);

  /**
   * @fn DFRobot_SGX6410_UART
   * @brief Constructor (HardwareSerial)
   * @param hSerial HardwareSerial object pointer, typically &Serial1
   * @param baud Baud rate, default 9600
   * @param rxPin RX pin, -1 keeps the current assignment or uses the core default
   * @param txPin TX pin, -1 keeps the current assignment or uses the core default
   * @n Default line settings are 9600 8N1.
   * @n ESP32: pass rxPin/txPin to remap Serial1, or omit them to keep the current pin assignments or use the core defaults.
   */
  DFRobot_SGX6410_UART(HardwareSerial *hSerial,
                       uint32_t baud = SGX_UART_BAUDRATE,
                       int8_t rxPin = -1,
                       int8_t txPin = -1);

  /**
   * @fn begin
   * @brief Initialize UART and then verify the sensor
   * @return bool
   * @retval true  Initialization successful
   * @retval false Initialization failed
   * @n Opens the injected serial port at the constructor baud rate, waits 20 ms, flushes RX, then runs the base begin() check.
   * @n On ESP32, custom RX/TX from the constructor are applied here.
   * @n If the first command fails, sends the 0xFF sync byte. Call begin() again to retry.
   */
  bool begin(void);

  /**
   * @fn DFRobot_SGX6410_I2C
   * @brief Constructor
   * @param pWire I2C object pointer, typically &Wire
   * @param address 7-bit I2C address (bits 6:0), default 0x38
   * @n The programmable I2C address byte uses bit 7 = don't care, bits 6:0 = slave address (0x00-0x7F).
   * @param sclPin SCL pin, default 0xFF (board default)
   * @param sdaPin SDA pin, default 0xFF (board default)
   * @n ESP32/ESP8266: pass sclPin/sdaPin to remap I2C, or omit them to use the board default pins.
   * @n UNO and other boards with fixed Wire pins: omit sclPin/sdaPin; they are ignored in begin().
   */
  DFRobot_SGX6410_I2C(TwoWire *pWire, uint8_t address = SGX_I2C_ADDR, uint8_t sclPin = 0xFF, uint8_t sdaPin = 0xFF);

  /**
   * @fn begin
   * @brief Initialize I2C and then verify the sensor
   * @return bool
   * @retval true  Initialization successful
   * @retval false Initialization failed
   * @n Calls TwoWire::begin() on the injected bus, then the base begin() operation-mode read.
   * @n On ESP32/ESP8266, custom SCL/SDA from the constructor are applied here. UNO always uses the default Wire pins.
   */
  bool begin(void);

  /**
   * @fn begin
   * @brief Initialize the communication interface and verify the sensor
   * @return bool
   * @retval true  Initialization successful
   * @retval false Initialization failed
   * @n Reads the current operation mode and starts the host-side warm-up timer.
   */
  bool begin(void);

  /**
   * @fn getProductID
   * @brief Read product ID and firmware version
   * @param productId Product identifier returned by the sensor, expected 0x2310
   * @param firmwareVer Three firmware version bytes: major, minor, revision
   * @return bool
   * @retval true  Read successful
   * @retval false Invalid pointer or read failed
   * @n On failure, productId and firmwareVer keep the values they had before the call.
   */
  bool getProductID(uint16_t &productId, uint8_t firmwareVer[3]);

  /**
   * @fn setOperationMode
   * @brief Set SGX6410 operation mode
   * @param mode Operation mode (see eSGXMode_t)
   * @n eSgxModeSuspend: Suspend mode
   * @n eSgxModeIaq: IAQ 2nd generation, about 3 s sample period, 5 min warm-up
   * @n eSgxModeUlp: Ultra-low power, about 90 s sample period, 15 min warm-up
   * @n eSgxModePbaq: PBAQ, about 5 s sample period, 5 min warm-up
   * @n Mode change is a maintenance action. The host restarts warm-up and clears isNew/valid.
   * @param save Save this mode as the startup mode and commit all Flash Shadow settings, default false
   * @return SGX_FDL_OK on success, SGX_FDL_PARAMETER_ERROR for an invalid mode,
   * @n SGX_COMM_ERROR on communication failure, or the FDL status from the Flash commit.
   * @n The current mode changes immediately. No automatic reset.
   * @n With save=false, success only means the mode command was sent.
   * @n If saving fails, the current mode has already changed.
   */
  uint8_t setOperationMode(eSGXMode_t mode, bool save = false);

  /**
   * @fn setHumidityCompensate
   * @brief Write ambient relative humidity for algorithm compensation
   * @param rh Relative humidity, range 0.0 to 100.0, unit %RH
   * @n Host converts rh to code = (rh / 100) * 255 and writes the 0-255 code.
   * @n The default compensation value is 50.0 %RH when this method is not called.
   * @return bool
   * @retval true  Write successful
   * @retval false rh out of range or write failed
   */
  bool setHumidityCompensate(float rh);

  /**
   * @fn getOperationMode
   * @brief Read the current SGX6410 operation mode
   * @return eSGXMode_t Current operation mode, or eSgxModeNone (0xFF) if the read fails
   */
  eSGXMode_t getOperationMode(void);

  /**
   * @fn update
   * @brief Read the measurement frame and scale it to engineering units
   * @return bool
   * @retval true  Frame was read (a repeated sample still returns true with isNew = false)
   * @retval false Bus error, parse error, or current mode has no measurement payload
   * @n Call this before getAllGasData() / getSingleGasData().
   * @n IAQ/ULP: IAQ = raw/10, TVOC = raw/100 mg/m3, ETOH = raw/100 ppm, ECO2 = raw ppm, RELIAQ = raw/10
   * @n PBAQ: TVOC = raw/1000 mg/m3, ETOH = raw/1000 ppm; IAQ, ECO2 and RELIAQ are NAN
   * @n Suspend mode has no measurement payload and return false.
   * @n isNew follows the sample-counter byte. valid is true only after mode warm-up.
   */
  bool update(void);

  /**
   * @fn getAllGasData
   * @brief Return the cached gas measurement structure
   * @return const sSGXData_t & Latest cached measurement
   * @n Must call update() first. Check isNew for a new sample and valid for warm-up completion.
   */
  const sSGXData_t &getAllGasData(void) const;

  /**
   * @fn getSingleGasData
   * @brief Return one value from the cached measurement
   * @param dataType Gas value selected by eSGXDataType_t
   * @n eSgxDataIaq: IAQ
   * @n eSgxDataTvoc: TVOC
   * @n eSgxDataEtoh: ETOH
   * @n eSgxDataEcO2: estimated CO2
   * @n eSgxDataReliaq: relative IAQ
   * @return float Cached value, or NAN if unsupported in the current mode
   * @n Must call update() first.
   */
  float getSingleGasData(eSGXDataType_t dataType);

  /**
   * @fn getTrackingNumber
   * @brief Read the six-byte SGX6410 tracking number
   * @param number Caller-provided six-byte output buffer
   * @return bool
   * @retval true  Read successful
   * @retval false Invalid pointer or read failed
   */
  bool getTrackingNumber(uint8_t *number);

  /**
   * @fn setI2CAddress
   * @brief Set the I2C slave address in Flash Shadow and optionally save to Flash
   * @param address 7-bit slave address, 0x00-0x7F; other values send no command
   * @param save Whether to commit all Flash Shadow settings to Flash, default false
   * @return SGX_FDL_OK on success, SGX_FDL_PARAMETER_ERROR for an invalid address,
   * @n SGX_COMM_ERROR on communication failure, or the FDL status from the Flash commit.
   * @n With save=false, success only means the Shadow write command was sent.
   * @n The address takes effect after saving and resetting the sensor. No automatic reset.
   * @n After reset, recreate the I2C object with the new address.
   */
  uint8_t setI2CAddress(uint8_t address, bool save = false);

  /**
   * @fn readDebugData
   * @brief Read raw debug bytes
   * @param data Caller-provided output buffer
   * @param size Number of payload bytes to read, up to 100
   * @return bool
   * @retval true  Read successful
   * @retval false Invalid size/pointer or read failed
   */
  bool readDebugData(uint8_t *data, size_t size);

  /**
   * @fn reset
   * @brief Reset the sensor
   * @return bool
   * @retval true  Reset command sent successfully
   * @retval false Write failed
   * @n Command 0x8F has no response. After reset, wait for start-up and call begin() again.
   */
  bool reset();

  /**
   * @fn isDataNew
   * @brief Check whether the latest update contains a new sample
   * @return bool Cached isNew flag
   * @n Must call update() first.
   */
  bool isDataNew(void);

  /**
   * @fn isDataValid
   * @brief Check whether cached measurement data is valid
   * @return bool Cached valid flag
   * @n Must call update() first.
   * @n valid is true only for a new sample acquired after warm-up: 5 min IAQ, 15 min ULP, 5 min PBAQ.
   */
  bool isDataValid(void);
```

## Compatibility

| Board        | Work Well | Work Wrong | Untested | Remarks |
| ------------ | :-------: | :--------: | :------: | ------- |
| Arduino uno  |      √      |            |        |         |
| Mega2560     |      √      |            |     |         |
| Leonardo     |      √      |            |      |         |
| ESP32        |      √      |            |     |         |
| ESP8266      |      √      |            |      |         |
| FireBeetle M0 |      √      |            |        |         |
| micro:bit     |      √      |            |        |         |

## History

- 2026/09/03 - V1.0.0 version

## Credits

Written by PLELES(li.jia@dfrobot.com), 2026. (Welcome to our [website](https://www.dfrobot.com/))
