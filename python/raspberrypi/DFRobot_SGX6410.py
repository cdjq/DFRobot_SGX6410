# -*- coding:utf-8 -*-
'''!
  @file DFRobot_SGX6410.py
  @brief SGX6410 MEMS air quality sensor Raspberry Pi library
  @copyright Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
  @license The MIT License (MIT)
  @author PLELES(li.jia@dfrobot.com)
  @version V1.0.0
  @date 2026-09-04
  @url https://github.com/cdjq/DFRobot_SGX6410
'''

import math
import struct
import time

try:
  from smbus2 import SMBus, i2c_msg
except ImportError:
  SMBus = None
  i2c_msg = None

try:
  import serial
except ImportError:
  serial = None


SGX_TIMEOUT_MS = 100
SGX_I2C_ADDR = 0x38
SGX_UART_BAUDRATE = 9600
SGX_UART_SYNC_BYTE = 0xFF
SGX_WARMUP_IAQ_MS = 300000
SGX_WARMUP_ULP_MS = 900000
SGX_WARMUP_PBAQ_MS = 300000
SGX_I2C_ADDRESS_SHADOW_ADDR = 0x00
SGX_STARTUP_MODE_SHADOW_ADDR = 0x03

SGX_CMD_GET_PRODUCT_ID = 0x00
SGX_CMD_GET_TRACKING_NUMBER = 0x01
SGX_CMD_GET_OPERATION_MODE = 0x10
SGX_CMD_SET_OPERATION_MODE = 0x11
SGX_CMD_SET_HUMIDITY = 0x12
SGX_CMD_GET_MEASUREMENT = 0x18
SGX_CMD_READ_DEBUG = 0x31
SGX_CMD_READ_FLASH = 0x40
SGX_CMD_WRITE_FLASH = 0x41
SGX_CMD_SAVE_FLASH = 0x42
SGX_CMD_RESET = 0x8F


SGX_FDL_IDLE = 0x30
SGX_FDL_OK = 0x00
SGX_FDL_BUSY = 0xFF
SGX_FDL_PROTECTION_ERROR = 0x10
SGX_FDL_ERASE_ERROR = 0x1A
SGX_FDL_BLANK_CHECK_ERROR = 0x1B
SGX_FDL_WRITE_ERROR = 0x1C
SGX_FDL_PARAMETER_ERROR = 0x05

I2C_MODE = 0x01
UART_MODE = 0x02


class SGXData(object):
  '''!
    @brief Parsed and cached gas measurement data
  '''

  def __init__(self):
    self.iaq = float('nan')
    self.tvoc = float('nan')
    self.etoh = float('nan')
    self.eco2 = float('nan')
    self.reliaq = float('nan')
    self.is_new = False
    self.valid = False


class DFRobot_SGX6410(object):
  SGX_MODE_SUSPEND = 0x00
  SGX_MODE_IAQ = 0x01
  SGX_MODE_ULP = 0x02
  SGX_MODE_PBAQ = 0x05
  SGX_MODE_NONE = 0xFF
  SGX_DATA_IAQ = 0
  SGX_DATA_TVOC = 1
  SGX_DATA_ETOH = 2
  SGX_DATA_ECO2 = 3
  SGX_DATA_RELIAQ = 4

  def __init__(self):
    self._mode = self.SGX_MODE_SUSPEND
    self._gas_data = SGXData()
    self._sample_counter = 0
    self._sample_counter_new = False
    self._mode_start_time = 0
    self._mode_timer_valid = False

  @staticmethod
  def _checksum(data):
    return (~sum(data)) & 0xFF

  def _write_transport(self, data):
    raise NotImplementedError

  def _read_transport(self, length):
    raise NotImplementedError

  def _write_cmd(self, cmd, data=None):
    payload = [cmd] + list(data or [])
    payload.append(self._checksum(payload))
    return self._write_transport(payload)

  def _read_cmd(self, cmd, size):
    request = [cmd, self._checksum([cmd])]
    if not self._write_transport(request):
      return None
    response = self._read_transport(size + 2)
    if response is None or len(response) != size + 2:
      return None
    if response[0] != 0 or self._checksum(response[:-1]) != response[-1]:
      return None
    return response[1:-1]

  def begin(self):
    '''!
      @brief Initialize the communication interface and verify the sensor
      @return True on success, False on failure
      @n Reads product ID (expected 0x2310) and the six-byte tracking number.
      @n Both reads must succeed before the sensor is treated as online.
      @n Then reads the current operation mode and starts the host-side warm-up timer.
    '''
    product = self.get_product_id()
    if product is None or product[0] != 0x2310:
      return False
    if self.get_tracking_number() is None:
      return False
    mode = self._read_cmd(SGX_CMD_GET_OPERATION_MODE, 1)
    if mode is None:
      return False
    self._mode = mode[0]
    self._mode_timer_valid = self._mode in (self.SGX_MODE_IAQ, self.SGX_MODE_ULP, self.SGX_MODE_PBAQ)
    self._mode_start_time = self._millis()
    self._sample_counter_new = False
    self._gas_data.is_new = False
    self._gas_data.valid = False
    return True

  def get_product_id(self):
    '''!
      @brief Read product ID and firmware version
      @return (product_id, (major, minor, revision)) on success, None on failure
      @n product_id is expected to be 0x2310.
    '''
    response = self._read_cmd(SGX_CMD_GET_PRODUCT_ID, 5)
    if response is None:
      return None
    product_id = response[0] | (response[1] << 8)
    return product_id, tuple(response[2:5])

  def set_operation_mode(self, mode):
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
    if mode not in (self.SGX_MODE_SUSPEND, self.SGX_MODE_IAQ, self.SGX_MODE_ULP,
                    self.SGX_MODE_PBAQ):
      return False
    if not self._write_cmd(SGX_CMD_SET_OPERATION_MODE, [mode]):
      return False
    self._mode = mode
    self._sample_counter_new = False
    self._mode_timer_valid = mode in (self.SGX_MODE_IAQ, self.SGX_MODE_ULP, self.SGX_MODE_PBAQ)
    self._mode_start_time = self._millis()
    self._gas_data.is_new = False
    self._gas_data.valid = False
    return True

  def set_humidity_compensate(self, rh):
    '''!
      @brief Write ambient relative humidity for algorithm compensation
      @param rh Relative humidity, range 0.0 to 100.0, unit %RH
      @n Host converts rh to code = (rh / 100) * 255 and writes the 0-255 code.
      @n The default compensation value is 50.0 %RH when this method is not called.
      @return True on success, False on failure
    '''
    if math.isnan(rh) or rh < 0.0 or rh > 100.0:
      return False
    return self._write_cmd(SGX_CMD_SET_HUMIDITY, [int((rh / 100.0) * 255.0) & 0xFF])

  def get_operation_mode(self):
    '''!
      @brief Read the current SGX6410 operation mode
      @return Current mode command byte
      @n Returns SGX_MODE_NONE (0xFF) when the read fails.
    '''
    response = self._read_cmd(SGX_CMD_GET_OPERATION_MODE, 1)
    if response is None:
      return self.SGX_MODE_NONE
    self._mode = response[0]
    return response[0]

  def update(self):
    '''!
      @brief Read the measurement frame and scale it to engineering units
      @return True if the frame was read (a repeated sample still returns True with is_new = False)
      @n Call this before get_all_gas_data() / get_single_gas_data().
      @n IAQ/ULP: IAQ = raw/10, TVOC = raw/100 mg/m3, ETOH = raw/100 ppm, ECO2 = raw ppm, RELIAQ = raw/10
      @n PBAQ: TVOC = raw/1000 mg/m3, ETOH = raw/1000 ppm; IAQ, ECO2 and RELIAQ are NAN
      @n Suspend mode has no measurement payload and return False.
      @n is_new follows the sample-counter byte. valid is true only after mode warm-up.
    '''
    self._gas_data.is_new = False
    if self._mode not in (self.SGX_MODE_IAQ, self.SGX_MODE_ULP, self.SGX_MODE_PBAQ):
      return False
    payload_size = 5 if self._mode == self.SGX_MODE_PBAQ else 9
    values = self._read_cmd(SGX_CMD_GET_MEASUREMENT, payload_size)
    if values is None:
      return False
    sample_counter = values[0]
    if self._sample_counter_new and sample_counter == self._sample_counter:
      return True
    if self._mode in (self.SGX_MODE_IAQ, self.SGX_MODE_ULP):
      self._gas_data.iaq = values[1] / 10.0
      self._gas_data.tvoc = (values[2] | (values[3] << 8)) / 100.0
      self._gas_data.etoh = (values[4] | (values[5] << 8)) / 100.0
      self._gas_data.eco2 = values[6] | (values[7] << 8)
      self._gas_data.reliaq = values[8] / 10.0
    else:
      self._gas_data.iaq = float('nan')
      self._gas_data.eco2 = float('nan')
      self._gas_data.reliaq = float('nan')
      self._gas_data.tvoc = (values[1] | (values[2] << 8)) / 1000.0
      self._gas_data.etoh = (values[3] | (values[4] << 8)) / 1000.0
    warmup = SGX_WARMUP_IAQ_MS if self._mode != self.SGX_MODE_ULP else SGX_WARMUP_ULP_MS
    self._gas_data.is_new = True
    self._gas_data.valid = self._mode_timer_valid and self._millis() - self._mode_start_time >= warmup
    self._sample_counter = sample_counter
    self._sample_counter_new = True
    return True

  def get_all_gas_data(self):
    '''!
      @brief Return the cached gas measurement structure
      @return SGXData Latest cached measurement
      @n Must call update() first. Check is_new for a new sample and valid for warm-up completion.
    '''
    return self._gas_data

  def get_single_gas_data(self, data_type):
    '''!
      @brief Return one value from the cached measurement
      @param data_type SGX_DATA_IAQ / SGX_DATA_TVOC / SGX_DATA_ETOH / SGX_DATA_ECO2 / SGX_DATA_RELIAQ
      @return Cached value, or NAN if unsupported in the current mode
      @n Must call update() first.
    '''
    values = (self._gas_data.iaq, self._gas_data.tvoc, self._gas_data.etoh,
              self._gas_data.eco2, self._gas_data.reliaq)
    return values[data_type] if 0 <= data_type < len(values) else float('nan')

  def get_tracking_number(self):
    '''!
      @brief Read the six-byte SGX6410 tracking number
      @return Six-byte bytes on success, None on failure
    '''
    response = self._read_cmd(SGX_CMD_GET_TRACKING_NUMBER, 6)
    return bytes(response) if response is not None else None

  def read_flash(self, address):
    '''!
      @brief Read one byte from the Flash Shadow area
      @param address Flash Shadow address: 0x00 I2C address, 0x03 startup mode
      @return Byte value on success, None on failure
      @n Other addresses are rejected on the host and send no command.
    '''
    if address not in (SGX_I2C_ADDRESS_SHADOW_ADDR, SGX_STARTUP_MODE_SHADOW_ADDR):
      return None
    if not self._write_transport([SGX_CMD_READ_FLASH, address, self._checksum([SGX_CMD_READ_FLASH, address])]):
      return None
    response = self._read_transport(3)
    if response is None or response[0] != 0 or self._checksum(response[:-1]) != response[-1]:
      return None
    return response[1]

  def write_flash(self, address, data):
    '''!
      @brief Write one byte to the Flash Shadow area
      @param address Flash Shadow address: 0x00 I2C address, 0x03 startup mode
      @param data Byte to write
      @return True on success, False on failure
      @n The new value stays in Shadow until save_flash() commits it.
      @n Changing the I2C address takes effect after save_flash() and reset().
    '''
    if address not in (SGX_I2C_ADDRESS_SHADOW_ADDR, SGX_STARTUP_MODE_SHADOW_ADDR):
      return False
    return self._write_cmd(SGX_CMD_WRITE_FLASH, [address, data & 0xFF])

  def save_flash(self):
    '''!
      @brief Commit the Flash Shadow area to Flash
      @return FDL status byte on success, None on failure
      @n Typical codes: SGX_FDL_OK (0x00) success, SGX_FDL_BUSY (0xFF) still processing.
    '''
    if not self._write_cmd(SGX_CMD_SAVE_FLASH):
      return None
    time.sleep(1.0)
    response = self._read_transport(1)
    return response[0] if response is not None and len(response) == 1 else None

  def read_debug_data(self, size):
    '''!
      @brief Read raw debug bytes
      @param size Number of payload bytes to read, up to 100
      @return bytes on success, None on failure
    '''
    response = self._read_cmd(SGX_CMD_READ_DEBUG, size)
    return bytes(response) if response is not None else None

  def reset(self):
    '''!
      @brief Reset the sensor
      @return True on success, False on failure
      @n Command 0x8F has no response. After reset, wait for start-up and call begin() again.
    '''
    result = self._write_cmd(SGX_CMD_RESET)
    if result:
      self._sample_counter_new = False
      self._mode_timer_valid = False
      self._gas_data.is_new = False
      self._gas_data.valid = False
    return result

  def is_data_new(self):
    '''!
      @brief Check whether the latest update contains a new sample
      @return Cached is_new flag
      @n Must call update() first.
    '''
    return self._gas_data.is_new

  def is_data_valid(self):
    '''!
      @brief Check whether cached measurement data is valid
      @return Cached valid flag
      @n Must call update() first.
      @n valid is true only for a new sample acquired after warm-up: 5 min IAQ, 15 min ULP, 5 min PBAQ.
    '''
    return self._gas_data.valid

  @staticmethod
  def _millis():
    return int(time.monotonic() * 1000)


class DFRobot_SGX6410_I2C(DFRobot_SGX6410):
  def __init__(self, bus=1, addr=SGX_I2C_ADDR):
    '''!
      @brief Constructor
      @param bus I2C bus number, typically 1 on Raspberry Pi
      @param addr 7-bit I2C address, default 0x38
    '''
    DFRobot_SGX6410.__init__(self)
    if SMBus is None:
      raise ImportError('Install python3-smbus2 to use I2C mode')
    self._addr = addr
    self.i2cbus = SMBus(bus)

  def _write_transport(self, data):
    try:
      # Send the complete command frame without an SMBus register or count byte.
      message = i2c_msg.write(self._addr, data)
      self.i2cbus.i2c_rdwr(message)
      return True
    except Exception:
      return False

  def _read_transport(self, length):
    try:
      # Read the response directly; do not send an extra register byte.
      message = i2c_msg.read(self._addr, length)
      self.i2cbus.i2c_rdwr(message)
      return list(message)
    except Exception:
      return None


class DFRobot_SGX6410_UART(DFRobot_SGX6410):
  def __init__(self, baud=SGX_UART_BAUDRATE, addr=SGX_I2C_ADDR,
               port='/dev/ttyAMA0'):
    '''!
      @brief Constructor
      @param baud Baud rate, default 9600
      @param addr Kept for API symmetry; UART frames do not use a slave address
      @param port Linux serial device, default /dev/ttyAMA0
      @n Default line settings are 9600 8N1.
    '''
    DFRobot_SGX6410.__init__(self)
    if serial is None:
      raise ImportError('Install pyserial to use UART mode')
    self._addr = addr
    self._baud = baud
    self.master = serial.Serial(port=port, baudrate=baud, bytesize=8,
                                parity='N', stopbits=1, timeout=SGX_TIMEOUT_MS / 1000.0)

  def begin(self):
    '''!
      @brief Initialize UART and then verify the sensor
      @return True on success, False on failure
      @n Flushes RX, waits 20 ms, then runs the base begin() check.
      @n If the first command fails, sends the 0xFF sync byte. Call begin() again to retry.
    '''
    self.master.reset_input_buffer()
    time.sleep(0.02)
    if DFRobot_SGX6410.begin(self):
      return True
    self.master.write(bytes([SGX_UART_SYNC_BYTE]))
    return False

  def _write_transport(self, data):
    try:
      self.master.reset_input_buffer()
      return self.master.write(bytes(data)) == len(data)
    except Exception:
      return False

  def _read_transport(self, length):
    try:
      data = self.master.read(length)
      return list(data) if len(data) == length else None
    except Exception:
      return None
