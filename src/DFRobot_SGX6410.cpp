/*!
 * @file DFRobot_SGX6410.cpp
 * @brief Implement the DFRobot SGX6410 air quality sensor API.
 * @details Implements command framing, measurement parsing, and I2C/UART transports.
 * @copyright Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license The MIT License (MIT)
 * @author [PLELES](li.jia@dfrobot.com)
 * @version V1.0.0
 * @date 2026-09-03
 * @url https://github.com/cdjq/DFRobot_SGX6410
 */

#include "DFRobot_SGX6410.h"

static uint8_t sgxChecksum(const uint8_t *data, size_t len) {
  uint8_t sum = 0;
  for (size_t i = 0; i < len; ++i) sum += data[i];
  return (uint8_t)~sum;
}

DFRobot_SGX6410::DFRobot_SGX6410()
    : _mode(SGX_MODE_SUSPEND), _sampleCounter(0), _sampleCounterNew(false), _modeStartTime(0), _modeTimerValid(false) {
  _gasData.IAQ = NAN;
  _gasData.TVOC = NAN;
  _gasData.ETOH = NAN;
  _gasData.ECO2 = NAN;
  _gasData.RELIAQ = NAN;
  _gasData.isNew = false;
  _gasData.valid = false;
}

bool DFRobot_SGX6410::writeCmd(uint8_t cmd, const uint8_t *data, size_t size) {
  if (size > 100 || (size && data == nullptr)) return false;
  uint8_t frame[102];
  frame[0] = cmd;
  for (size_t i = 0; i < size; ++i) frame[i + 1] = data[i];
  frame[size + 1] = sgxChecksum(frame, size + 1);
  return interfaceWrite(frame, size + 2);
}

bool DFRobot_SGX6410::readCmd(uint8_t cmd, uint8_t *data, size_t size) {
  if (size > 100 || (size && data == nullptr)) return false;
  uint8_t request[2];
  request[0] = cmd;
  request[1] = sgxChecksum(request, 1);
  if (!interfaceWrite(request, sizeof(request))) return false;
  uint8_t response[102];
  int16_t count = interfaceRead(response, size + 2);
  if (count != (int16_t)(size + 2)) return false;
  if (response[0] != 0 || sgxChecksum(response, size + 1) != response[size + 1]) return false;
  for (size_t i = 0; i < size; ++i) data[i] = response[i + 1];
  return true;
}

bool DFRobot_SGX6410::begin(void) {
  uint16_t id = 0;
  uint8_t version[3] = {0, 0, 0};
  uint8_t tracking[6] = {0, 0, 0, 0, 0, 0};
  if (!getProductID(id, version)) return false;
  if (id != 0x2310) return false;
  if (!getTrackingNumber(tracking)) return false;
  uint8_t mode = SGX_MODE_SUSPEND;
  if (!readCmd(SGX_CMD_GET_OPERATION_MODE, &mode, 1)) return false;   // Get the current operation mode.
  _mode = (eSGXMode_t)mode;
  // Only these three modes will have a preheating timer.
  _modeTimerValid = mode == SGX_MODE_IAQ || mode == SGX_MODE_ULP || mode == SGX_MODE_PBAQ;
  // Start the preheating timer.
  _modeStartTime = millis();
  _sampleCounterNew = false;
  _gasData.isNew = false;
  _gasData.valid = false;
  return true;
}

bool DFRobot_SGX6410::getProductID(uint16_t &productId, uint8_t firmwareVer[3]) {
  if (firmwareVer == nullptr) return false;
  uint8_t response[5];
  if (!readCmd(SGX_CMD_GET_PRODUCT_ID, response, sizeof(response))) return false;
  uint16_t id = (uint16_t)response[0] | ((uint16_t)response[1] << 8);
  productId = id;
  firmwareVer[0] = response[2]; firmwareVer[1] = response[3]; firmwareVer[2] = response[4];
  return true;
}

bool DFRobot_SGX6410::setOperationMode(eSGXMode_t mode) {
  if (mode != SGX_MODE_SUSPEND && mode != SGX_MODE_IAQ &&
      mode != SGX_MODE_ULP && mode != SGX_MODE_PBAQ) return false;
  uint8_t value = (uint8_t)mode;
  if (!writeCmd(SGX_CMD_SET_OPERATION_MODE, &value, 1)) return false;
  _mode = mode;
  _sampleCounterNew = false;
  _modeTimerValid = mode == SGX_MODE_IAQ || mode == SGX_MODE_ULP || mode == SGX_MODE_PBAQ;
  _modeStartTime = millis();
  _gasData.isNew = false;
  _gasData.valid = false;
  return true;
}

bool DFRobot_SGX6410::setHumidityCompensate(float rh) {
  if (isnan(rh) || rh < 0.0f || rh > 100.0f) return false;
  // The protocol represents relative humidity as an unsigned 8-bit code.
  uint8_t code = (uint8_t)((rh / 100.0f) * 255.0f);
  return writeCmd(SGX_CMD_SET_HUMIDITY, &code, 1);
}

eSGXMode_t DFRobot_SGX6410::getOperationMode(void) {
  uint8_t value = SGX_MODE_NONE;
  if (!readCmd(SGX_CMD_GET_OPERATION_MODE, &value, 1)) return SGX_MODE_NONE;
  _mode = (eSGXMode_t)value;
  return _mode;
}

bool DFRobot_SGX6410::parseMeasurement(const uint8_t *values, size_t size, sSGXData_t &data) {
  if (values == nullptr) return false;
  if (_mode == SGX_MODE_IAQ || _mode == SGX_MODE_ULP) {
    // The first payload byte is the sample counter, followed by 8 data bytes.
    if (size != 9) return false;
    data.IAQ = values[1] / 10.0f;
    data.TVOC = ((uint16_t)values[2] | ((uint16_t)values[3] << 8)) / 100.0f;
    data.ETOH = ((uint16_t)values[4] | ((uint16_t)values[5] << 8)) / 100.0f;
    data.ECO2 = (uint16_t)values[6] | ((uint16_t)values[7] << 8);
    data.RELIAQ = values[8] / 10.0f;
    return true;
  }
  if (_mode == SGX_MODE_PBAQ) {
    // The first payload byte is the sample counter, followed by 4 data bytes.
    if (size != 5) return false;
    data.IAQ = NAN; 
    data.ECO2 = NAN; 
    data.RELIAQ = NAN;
    data.TVOC = ((uint16_t)values[1] | ((uint16_t)values[2] << 8)) / 1000.0f;
    data.ETOH = ((uint16_t)values[3] | ((uint16_t)values[4] << 8)) / 1000.0f;
    return true;
  }
  return false;
}

bool DFRobot_SGX6410::update(void) {
  _gasData.isNew = false;
  if (_mode != SGX_MODE_IAQ && _mode != SGX_MODE_ULP && _mode != SGX_MODE_PBAQ) return false;
  // readCmd() receives payload length only. The complete response is
  // status + payload + checksum: 7 bytes for PBAQ and 11 bytes for IAQ/ULP.
  size_t payloadSize = (_mode == SGX_MODE_PBAQ) ? 5 : 9;
  uint8_t values[9];
  if (!readCmd(SGX_CMD_GET_MEASUREMENT, values, payloadSize)) return false;

  const uint8_t sampleCounter = values[0];
  const bool isNewSample = !_sampleCounterNew || sampleCounter != _sampleCounter;  // Check if the sample counter has changed.
  if (!isNewSample) return true;

  sSGXData_t parsedData = _gasData;
  if (!parseMeasurement(values, payloadSize, parsedData)) return false;

  uint32_t warmupTime = SGX_WARMUP_IAQ_MS;
  if (_mode == SGX_MODE_ULP) warmupTime = SGX_WARMUP_ULP_MS;
  if (_mode == SGX_MODE_PBAQ) warmupTime = SGX_WARMUP_PBAQ_MS;
  parsedData.isNew = true;
  parsedData.valid = _modeTimerValid && (uint32_t)(millis() - _modeStartTime) >= warmupTime;
  _gasData = parsedData;
  _sampleCounter = sampleCounter;
  _sampleCounterNew = true;
  return true;
}

float DFRobot_SGX6410::getSingleGasData(eSGXDataType_t dataType) {
  switch (dataType) {
    case SGX_DATA_IAQ: return _gasData.IAQ;
    case SGX_DATA_TVOC: return _gasData.TVOC;
    case SGX_DATA_ETOH: return _gasData.ETOH;
    case SGX_DATA_ECO2: return _gasData.ECO2;
    case SGX_DATA_RELIAQ: return _gasData.RELIAQ;
    default: return NAN;
  }
}

bool DFRobot_SGX6410::getTrackingNumber(uint8_t *number) {
  return readCmd(SGX_CMD_GET_TRACKING_NUMBER, number, 6);
}

bool DFRobot_SGX6410::readFlash(uint8_t address, uint8_t &data) {
  if (address != SGX_I2C_ADDRESS_SHADOW_ADDR && address != SGX_STARTUP_MODE_SHADOW_ADDR) return false;
  uint8_t request[3] = {SGX_CMD_READ_FLASH, address, 0};
  request[2] = sgxChecksum(request, 2);
  if (!interfaceWrite(request, sizeof(request))) return false;
  uint8_t response[3];
  if (interfaceRead(response, sizeof(response)) != (int16_t)sizeof(response)) return false;
  if (response[0] != 0 || sgxChecksum(response, 2) != response[2]) return false;
  data = response[1];
  return true;
}

bool DFRobot_SGX6410::writeFlash(uint8_t address, uint8_t data) {
  if (address != SGX_I2C_ADDRESS_SHADOW_ADDR && address != SGX_STARTUP_MODE_SHADOW_ADDR) return false;
  uint8_t payload[2] = {address, data};
  return writeCmd(SGX_CMD_WRITE_FLASH, payload, sizeof(payload));
}

bool DFRobot_SGX6410::saveFlash(uint8_t &status) {
  // Write Flash has no request payload and returns one raw FDL status byte.
  uint8_t request[2] = {SGX_CMD_SAVE_FLASH, 0};
  request[1] = sgxChecksum(request, 1);
  if (!interfaceWrite(request, sizeof(request))) return false;
  delay(1000); // Wait for the FDL status to be updated.
  return interfaceRead(&status, 1) == 1;
}

bool DFRobot_SGX6410::readDebugData(uint8_t *data, size_t size) {
  return readCmd(SGX_CMD_READ_DEBUG, data, size);
}

bool DFRobot_SGX6410::reset() {
  bool result = writeCmd(SGX_CMD_RESET, nullptr, 0);
  if (result) {
    _sampleCounterNew = false;
    _modeTimerValid = false;
    _gasData.isNew = false;
    _gasData.valid = false;
  }
  return result;
}


bool DFRobot_SGX6410::isDataNew(void) {
  return _gasData.isNew;
}
bool DFRobot_SGX6410::isDataValid(void) {
  return _gasData.valid;
}

DFRobot_SGX6410_I2C::DFRobot_SGX6410_I2C(TwoWire *pWire, uint8_t address, uint8_t sclPin, uint8_t sdaPin) : 
_pWire(pWire), _address(address) ,_sclPin(sclPin), _sdaPin(sdaPin){
}

bool DFRobot_SGX6410_I2C::begin(void) {
  if (!_pWire) return false;
#if defined(ESP32) || defined(ESP8266)
  if (_sclPin != 0xFF || _sdaPin != 0xFF) {
    _pWire->begin(_sdaPin, _sclPin);
  } else {
    _pWire->begin();
  }
#else
  _pWire->begin();
#endif
  delay(20);
  return DFRobot_SGX6410::begin();
}

bool DFRobot_SGX6410_I2C::interfaceWrite(const uint8_t *data, size_t len) {
  if (!_pWire) return false;
  _pWire->beginTransmission(_address);
  _pWire->write(data, len);
  return _pWire->endTransmission() == 0;
}

int16_t DFRobot_SGX6410_I2C::interfaceRead(uint8_t *data, size_t len) {
  if (!_pWire) return -1;
  uint8_t got = _pWire->requestFrom((int)_address, (int)len);
  size_t i = 0;
  while (_pWire->available() && i < len) data[i++] = _pWire->read();
  return got == len ? (int16_t)i : -1;
}

#if defined(ARDUINO_AVR_UNO) || defined(ESP8266)
DFRobot_SGX6410_UART::DFRobot_SGX6410_UART(SoftwareSerial *sSerial,
                                           uint32_t baud)
    : _serial(sSerial), _baud(baud), _rxPin(0), _txPin(0) {
}
#else
DFRobot_SGX6410_UART::DFRobot_SGX6410_UART(HardwareSerial *hSerial,
                                           uint32_t baud,
                                           uint8_t rxPin,
                                           uint8_t txPin)
    : _serial(hSerial), _baud(baud), _rxPin(rxPin), _txPin(txPin) {
}
#endif

bool DFRobot_SGX6410_UART::begin(void) {
  if (!_serial) return false;
#if defined(ESP32)
  _serial->begin(_baud, SERIAL_8N1, _rxPin, _txPin);
  delay(100);
#elif defined(ARDUINO_AVR_UNO) || defined(ESP8266)
  _serial->begin(_baud);
  delay(100);
#else
  _serial->begin(_baud);
#endif

  // UART has no hardware handshake or frame delimiter. Follow the sensor's
  // synchronization procedure before sending the first command frame.
  delay(20);
  while (_serial->available()) _serial->read();
  if (DFRobot_SGX6410::begin()) return true;

  // A checksum error is the sensor's only way to discard a partial command.
  // The caller can invoke begin() again to repeat the synchronization loop.
  _serial->write((uint8_t)SGX_UART_SYNC_BYTE);
  return false;
}

bool DFRobot_SGX6410_UART::interfaceWrite(const uint8_t *data, size_t len) {
  if (!_serial) return false;
  // Write commands are intentionally not read by writeCmd(). Discard any
  // delayed write response before issuing the next request so it cannot be
  // mistaken for the next read response.
  while (_serial->available()) _serial->read();
  return _serial->write(data, len) == len;
}

int16_t DFRobot_SGX6410_UART::interfaceRead(uint8_t *data, size_t len) {
  if (!_serial) return -1;
  unsigned long start = millis();
  size_t i = 0;
  while (i < len && millis() - start < SGX_TIMEOUT_MS) {
    if (_serial->available()) data[i++] = _serial->read();
  }
  return i == len ? (int16_t)i : -1;
}
