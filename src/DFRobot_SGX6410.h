/*!
 * @file DFRobot_SGX6410.h
 * @brief Define basic struct of DFRobot_SGX6410 class
 * @details Header for DFRobot_SGX6410: class declaration, protocol constants, enums and structs.
 * @n The host talks to the SGX6410 over I2C or UART using the same command frame.
 * @copyright Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license The MIT License (MIT)
 * @author PLELES(li.jia@dfrobot.com)
 * @version V1.0.0
 * @date 2026-09-03
 * @url https://github.com/cdjq/DFRobot_SGX6410
 */

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

#define SGX_CMD_GET_PRODUCT_ID       0x00    ///< Get product ID
#define SGX_CMD_GET_TRACKING_NUMBER  0x01    ///< Get tracking number
#define SGX_CMD_GET_OPERATION_MODE   0x10    ///< Get operation mode
#define SGX_CMD_SET_OPERATION_MODE   0x11    ///< Set operation mode
#define SGX_CMD_SET_HUMIDITY         0x12    ///< Set humidity compensation
#define SGX_CMD_GET_MEASUREMENT      0x18    ///< Get measurement results
#define SGX_CMD_CONFIG_GPIO          0x20    ///< Configure GPIO direction
#define SGX_CMD_GET_GPIO             0x21    ///< Get GPIO pin values
#define SGX_CMD_SET_GPIO             0x22    ///< Set GPIO1
#define SGX_CMD_CLEAR_GPIO           0x23    ///< Clear GPIO1
#define SGX_CMD_READ_FLASH           0x40    ///< Read Flash Shadow byte
#define SGX_CMD_WRITE_FLASH          0x41    ///< Write Flash Shadow byte
#define SGX_CMD_SAVE_FLASH           0x42    ///< Write Flash and return FDL status
#define SGX_CMD_READ_DEBUG           0x31    ///< Read debug data
#define SGX_CMD_RESET                0x8F    ///< Reset module; no response

#define CLEAN_OK                     0       ///< Cleaning completed
#define CLEAN_DONE                   1       ///< Cleaning was already performed
#define CLEAN_TIMEOUT                2       ///< Cleaning timed out
#define CLEAN_FAIL                   3       ///< Cleaning communication failure

#define SGX_FDL_IDLE                0x30    ///< Idle; ready to receive request
#define SGX_FDL_OK                  0x00    ///< Command finished without problems
#define SGX_FDL_BUSY                0xFF    ///< Command is being processed
#define SGX_FDL_PROTECTION_ERROR    0x10    ///< Access right conflict
#define SGX_FDL_ERASE_ERROR         0x1A    ///< Erase error
#define SGX_FDL_BLANK_CHECK_ERROR   0x1B    ///< Blank check or verify margin violated
#define SGX_FDL_WRITE_ERROR         0x1C    ///< Write error
#define SGX_FDL_PARAMETER_ERROR     0x05    ///< Parameter error

class DFRobot_SGX6410 {
public:
  /**
   * @enum sSGXMode_t
   * @brief SGX6410 operation mode
   * @n Line values match the sensor command bytes.
   */
  typedef enum {
    SGX_MODE_SUSPEND      = 0x00,    /**< Suspend mode */
    SGX_MODE_IAQ          = 0x01,    /**< IAQ 2nd generation mode, poll about every 3 s, warm-up 5 min */
    SGX_MODE_ULP          = 0x02,    /**< Ultra-low power mode, poll about every 90 s, warm-up 15 min */
    SGX_MODE_PBAQ         = 0x05,    /**< PBAQ mode, poll about every 5 s, warm-up 5 min */
    SGX_MODE_SENSOR_CLEAN = 0x80     /**< Thermal clean, once in lifetime */
  } sSGXMode_t;

  /**
   * @enum sSGXDataType_t
   * @brief Select one cached gas value
   */
  typedef enum {
    SGX_DATA_IAQ    = 0,    /**< IAQ, UBA level */
    SGX_DATA_TVOC   = 1,    /**< TVOC */
    SGX_DATA_ETOH   = 2,    /**< Ethanol equivalent */
    SGX_DATA_ECO2   = 3,    /**< Estimated CO2 */
    SGX_DATA_RELIAQ = 4     /**< Relative IAQ */
  } sSGXDataType_t;

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

  /**
   * @fn DFRobot_SGX6410
   * @brief Constructor
   */
  DFRobot_SGX6410();
  virtual ~DFRobot_SGX6410() {}

  /**
   * @fn begin
   * @brief Initialize the communication interface and verify the sensor
   * @return bool
   * @retval true  Initialization successful
   * @retval false Initialization failed
   * @n Reads product ID (expected 0x2310) and the six-byte tracking number.
   * @n Both reads must succeed before the sensor is treated as online.
   * @n Then reads the current operation mode and starts the host-side warm-up timer.
   */
  virtual bool begin(void);

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
   * @param mode Operation mode (see sSGXMode_t)
   * @n SGX_MODE_SUSPEND: Suspend mode
   * @n SGX_MODE_IAQ: IAQ 2nd generation, about 3 s sample period, 5 min warm-up
   * @n SGX_MODE_ULP: Ultra-low power, about 90 s sample period, 15 min warm-up
   * @n SGX_MODE_PBAQ: PBAQ, about 5 s sample period, 5 min warm-up
   * @n Mode change is a maintenance action. The host restarts warm-up and clears isNew/valid.
   * @n Sensor cleaning 0x80 should be started with runSensorClean(), not called repeatedly.
   * @return bool
   * @retval true  Setting successful
   * @retval false Setting failed
   */
  bool setOperationMode(sSGXMode_t mode);

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
   * @fn runSensorClean
   * @brief Run the sensor thermal-clean sequence: start clean, then poll until done
   * @param timeoutMs Timeout in ms, recommend >= 90000 (clean takes about 60 s)
   * @return uint8_t CLEAN_OK=0 finished / CLEAN_DONE=1 already cleaned / CLEAN_TIMEOUT=2 timeout / CLEAN_FAIL=3 comm error
   * @warning Interrupting clean can permanently damage the sensing material. Keep power stable.
   * @n The sensor should be cleaned only once in its lifetime.
   * @n Writes SGX_MODE_SENSOR_CLEAN (0x80) and polls the mode register: 0x80 still cleaning, 0x00 finished.
   * @n If the mode is already Suspend without observing clean, returns CLEAN_DONE.
   */
  uint8_t runSensorClean(uint32_t timeoutMs);

  /**
   * @fn getOperationMode
   * @brief Read the current SGX6410 operation mode
   * @return uint8_t Current mode command byte
   * @n Returns SGX_MODE_SUSPEND (0x00) when the read fails.
   * @n A returned 0x00 always means Suspend; it is not used to distinguish a communication error.
   */
  uint8_t getOperationMode(void);

  /**
   * @fn update
   * @brief Read the measurement frame and scale it to engineering units
   * @return bool
   * @retval true  Frame was read (a repeated sample still returns true with isNew = false)
   * @retval false Bus error, parse error, or current mode has no measurement payload
   * @n Call this before getAllGasData() / getSingleGasData().
   * @n IAQ/ULP: IAQ = raw/10, TVOC = raw/100 mg/m3, ETOH = raw/100 ppm, ECO2 = raw ppm, RELIAQ = raw/10
   * @n PBAQ: TVOC = raw/1000 mg/m3, ETOH = raw/1000 ppm; IAQ, ECO2 and RELIAQ are NAN
   * @n Suspend and sensor-clean modes have no measurement payload and return false.
   * @n isNew follows the sample-counter byte. valid is true only after mode warm-up.
   */
  bool update(void);

  /**
   * @fn getAllGasData
   * @brief Return the cached gas measurement structure
   * @return const sSGXData_t & Latest cached measurement
   * @n Must call update() first. Check isNew for a new sample and valid for warm-up completion.
   */
  const sSGXData_t &getAllGasData(void) const { return _gasData; }

  /**
   * @fn getSingleGasData
   * @brief Return one value from the cached measurement
   * @param dataType Gas value selected by sSGXDataType_t
   * @n SGX_DATA_IAQ: IAQ
   * @n SGX_DATA_TVOC: TVOC
   * @n SGX_DATA_ETOH: ETOH
   * @n SGX_DATA_ECO2: estimated CO2
   * @n SGX_DATA_RELIAQ: relative IAQ
   * @return float Cached value, or NAN if unsupported in the current mode
   * @n Must call update() first.
   */
  float getSingleGasData(sSGXDataType_t dataType);

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
   * @fn configGPIO
   * @brief Configure GPIO1 direction
   * @param config 0x00 for input or 0x01 for output
   * @n This implementation only configures GPIO1. Other values return false and send no command.
   * @n Generic bit map: bit0=GPIO1, bit1=GPIO2, bit2=GPIO3, bit3=GPIO4; 0=input, 1=output; bits 4-7 ignored.
   * @return bool
   * @retval true  Command accepted
   * @retval false Invalid config or write failed
   */
  bool configGPIO(uint8_t config);

  /**
   * @fn getGPIO
   * @brief Read GPIO pin values
   * @param value Output bit field
   * @n bit0=GPIO1, bit1=GPIO2, bit2=GPIO3, bit3=GPIO4; bits 4-7 are always 0.
   * @return bool
   * @retval true  Read successful
   * @retval false Read failed
   */
  bool getGPIO(uint8_t &value);

  /**
   * @fn setGPIO
   * @brief Set GPIO1 high
   * @return bool
   * @retval true  Command accepted
   * @retval false Write failed
   * @n Payload is fixed 0x01 and operates GPIO1 only.
   */
  bool setGPIO();

  /**
   * @fn clearGPIO
   * @brief Set GPIO1 low
   * @return bool
   * @retval true  Command accepted
   * @retval false Write failed
   * @n Payload is fixed 0x01 and operates GPIO1 only.
   */
  bool clearGPIO();

  /**
   * @fn readFlash
   * @brief Read one byte from the Flash Shadow area
   * @param address One-byte Flash Shadow address
   * @n SGX_I2C_ADDRESS_SHADOW_ADDR (0x00): I2C address byte
   * @n   bit 7 = don't care; bits 6:0 = 7-bit slave address (0x00-0x7F), default 0x38
   * @n SGX_STARTUP_MODE_SHADOW_ADDR (0x03): startup operation mode
   * @param data Output byte read from the requested address
   * @return bool
   * @retval true  Read successful
   * @retval false Unsupported address or read failed
   * @n Other addresses are rejected on the host and send no command.
   */
  bool readFlash(uint8_t address, uint8_t &data);

  /**
   * @fn writeFlash
   * @brief Write one byte to the Flash Shadow area
   * @param address One-byte Flash Shadow address
   * @n SGX_I2C_ADDRESS_SHADOW_ADDR (0x00): I2C address byte
   * @n   bit 7 = don't care; bits 6:0 = 7-bit slave address (0x00-0x7F), default 0x38
   * @n SGX_STARTUP_MODE_SHADOW_ADDR (0x03): startup operation mode
   * @param data Byte to write
   * @n For I2C address (shadow 0x00): bit 7 = don't care; bits 6:0 = slave address 0x00-0x7F
   * @return bool
   * @retval true  Command sent successfully
   * @retval false Unsupported address or write failed
   * @n The new value stays in Shadow until saveFlash() commits it.
   * @n Changing the I2C address takes effect after saveFlash() and reset().
   */
  bool writeFlash(uint8_t address, uint8_t data);

  /**
   * @fn saveFlash
   * @brief Commit the Flash Shadow area to Flash
   * @param status Output FDL status byte returned by the sensor
   * @return bool
   * @retval true  Status byte received
   * @retval false Write or read failed
   * @n The request has no payload. The response is one raw FDL status byte.
   * @n Typical codes: SGX_FDL_OK (0x00) success, SGX_FDL_BUSY (0xFF) still processing.
   */
  bool saveFlash(uint8_t &status);

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

protected:
  virtual bool interfaceWrite(const uint8_t *data, size_t len) = 0;
  virtual int16_t interfaceRead(uint8_t *data, size_t len) = 0;
  bool writeCmd(uint8_t cmd, const uint8_t *data, size_t size);
  bool readCmd(uint8_t cmd, uint8_t *data, size_t size);
  bool parseMeasurement(const uint8_t *values, size_t size, sSGXData_t &data);
  sSGXMode_t _mode;
  sSGXData_t _gasData;
  // Last sample counter returned by Get Measurement Results. It is not part
  // of sSGXData_t, but is used to reject repeated measurement frames.
  uint8_t _sampleCounter;
  bool _sampleCounterNew;
  uint32_t _modeStartTime;
  bool _modeTimerValid;
};

class DFRobot_SGX6410_I2C : public DFRobot_SGX6410 {
public:
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
   * @n Calls TwoWire::begin() on the injected bus, then the base begin() product-ID check.
   * @n On ESP32/ESP8266, custom SCL/SDA from the constructor are applied here. UNO always uses the default Wire pins.
   */
  bool begin(void) override;
protected:
  bool interfaceWrite(const uint8_t *data, size_t len) override;
  int16_t interfaceRead(uint8_t *data, size_t len) override;
private:
  TwoWire * _pWire;
  uint8_t   _address;
  uint8_t _sclPin;
  uint8_t _sdaPin;
};

class DFRobot_SGX6410_UART : public DFRobot_SGX6410 {
public:
#if defined(ARDUINO_AVR_UNO) || defined(ESP8266)
  /**
   * @fn DFRobot_SGX6410_UART
   * @brief Constructor (UNO/ESP8266 uses SoftwareSerial)
   * @param sSerial SoftwareSerial object pointer
   * @param baud Baud rate, default 9600
   * @n Default line settings are 9600 8N1.
   */
  DFRobot_SGX6410_UART(SoftwareSerial *sSerial, uint32_t baud = SGX_UART_BAUDRATE);
#else
  /**
   * @fn DFRobot_SGX6410_UART
   * @brief Constructor (HardwareSerial)
   * @param hSerial HardwareSerial object pointer, typically &Serial1
   * @param baud Baud rate, default 9600
   * @param rxPin RX pin, default 38
   * @param txPin TX pin, default 37
   * @n Default line settings are 9600 8N1.
   * @n ESP32: pass rxPin/txPin to remap Serial1, or omit them to use the constructor defaults.
   */
  DFRobot_SGX6410_UART(HardwareSerial *hSerial,
                       uint32_t baud = SGX_UART_BAUDRATE,
                       uint8_t rxPin = 38,
                       uint8_t txPin = 37);
#endif

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
  bool begin(void) override;
protected:
  bool interfaceWrite(const uint8_t *data, size_t len) override;
  int16_t interfaceRead(uint8_t *data, size_t len) override;
private:
#if defined(ARDUINO_AVR_UNO) || defined(ESP8266)
  SoftwareSerial *_serial;
#else
  HardwareSerial *_serial;
#endif
  uint32_t _baud;
  uint8_t _rxPin;
  uint8_t _txPin;
};

#endif
