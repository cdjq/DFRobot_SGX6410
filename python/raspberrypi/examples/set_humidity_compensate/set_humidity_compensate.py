# -*- coding: utf-8 -*-
'''!
  @file set_humidity_compensate.py
  @brief Set relative humidity compensation and print the result.
  @details Set HUMIDITY_RH to the actual ambient relative humidity (0.0-100.0 %RH).
  @n Apply the setting every 5 seconds. Use 101.0 to demonstrate an invalid humidity value.
  @n Success means the command was written; this interface does not read back the setting.
'''
import os
import sys
import time

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))))
from DFRobot_SGX6410 import (
  DFRobot_SGX6410_I2C, DFRobot_SGX6410_UART, SGX_I2C_ADDR, SGX_UART_BAUDRATE
)

# Ambient relative humidity for compensation, in %RH. Valid range: 0.0-100.0.
HUMIDITY_RH = 50.0

# Choose one communication method: uncomment only one line below.
#SGX6410_COMM_UART = True
SGX6410_COMM_I2C = True

if ('SGX6410_COMM_UART' in globals()) == ('SGX6410_COMM_I2C' in globals()):
  raise ValueError('Select exactly one of SGX6410_COMM_UART or SGX6410_COMM_I2C')
I2C_BUS = 1
ADDRESS = SGX_I2C_ADDR


def setup(sgx6410):
  print('=== Set Humidity Compensation Demo ===')
  while not sgx6410.begin():
    print('SGX6410 initialization failed. Check the connection.')
    time.sleep(1)
  print('SGX6410 initialization successful.')


def loop(sgx6410):
  print('Compensation humidity: %.1f %%RH' % HUMIDITY_RH)
  if sgx6410.set_humidity_compensate(HUMIDITY_RH):
    print('Humidity compensation setting successful.')
  else:
    print('Humidity compensation setting failed: invalid humidity or command write failed.')
  print('--------------------')
  time.sleep(5)


if __name__ == '__main__':
  if 'SGX6410_COMM_UART' in globals():
    sgx6410 = DFRobot_SGX6410_UART(SGX_UART_BAUDRATE, ADDRESS)
  else:
    sgx6410 = DFRobot_SGX6410_I2C(I2C_BUS, ADDRESS)
  setup(sgx6410)
  while True:
    loop(sgx6410)
