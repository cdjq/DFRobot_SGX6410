# -*- coding: utf-8 -*-
'''!
@file set_humidity_compensate.py
@brief Set relative humidity compensation and print the result.
@details Set humidity_rh to the actual ambient relative humidity (0.0-100.0 %RH).
@n Apply the setting every 5 seconds. Use 101.0 to demonstrate an invalid humidity value.
@n Success means the command was written; this interface does not read back the setting.
'''

import os
import sys
import time

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))))
from DFRobot_SGX6410 import DFRobot_SGX6410_I2C, DFRobot_SGX6410_UART, sgx_i2c_addr, sgx_uart_baudrate

# Ambient relative humidity for compensation, in %RH. Valid range: 0.0-100.0.
humidity_rh = 50.0

# Choose one communication method: uncomment only one line below.
# sgx6410_comm_uart = True
sgx6410_comm_i2c = True

if ('sgx6410_comm_uart' in globals()) == ('sgx6410_comm_i2c' in globals()):
  raise ValueError('Select exactly one of sgx6410_comm_uart or sgx6410_comm_i2c')
i2c_bus = 1
address = sgx_i2c_addr


def setup(sensor):
  print('=== Set Humidity Compensation Demo ===')
  while not sensor.begin():
    print('SGX6410 initialization failed. Check the connection.')
    time.sleep(1)
  print('SGX6410 initialization successful.')


def loop(sensor):
  print('Compensation humidity: %.1f %%RH' % humidity_rh)
  if sensor.set_humidity_compensate(humidity_rh):
    print('Humidity compensation setting successful.')
  else:
    print('Humidity compensation setting failed: invalid humidity or command write failed.')
  print('--------------------')
  time.sleep(5)


if __name__ == '__main__':
  if 'sgx6410_comm_uart' in globals():
    sgx6410 = DFRobot_SGX6410_UART(sgx_uart_baudrate, address)
  else:
    sgx6410 = DFRobot_SGX6410_I2C(i2c_bus, address)
  setup(sgx6410)
  while True:
    loop(sgx6410)
