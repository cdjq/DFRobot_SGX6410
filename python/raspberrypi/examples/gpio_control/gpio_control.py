'''! Configure and control SGX6410 GPIO1.'''
import os
import sys
import time

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))))
from DFRobot_SGX6410 import *

ctype = 0
sensor = DFRobot_SGX6410_I2C(1, SGX_I2C_ADDR) if ctype == 0 else DFRobot_SGX6410_UART(SGX_UART_BAUDRATE, SGX_I2C_ADDR)


def setup():
  # Initialize the sensor and verify product ID 0x2310
  while sensor.begin() == False:
    print('Sensor initialize failed!!')
  print('Sensor initialize success!!')
  # config_gpio(1) configures GPIO1 as an output
  print('GPIO1 configured: %s' % sensor.config_gpio(1))


def loop():
  # Toggle GPIO1 high, then low, and print bit0
  sensor.set_gpio()
  print('GPIO1: %s' % ('HIGH' if sensor.get_gpio() & 0x01 else 'LOW'))
  time.sleep(1)
  sensor.clear_gpio()
  print('GPIO1: %s' % ('HIGH' if sensor.get_gpio() & 0x01 else 'LOW'))
  time.sleep(1)


if __name__ == '__main__':
  setup()
  while True:
    loop()
