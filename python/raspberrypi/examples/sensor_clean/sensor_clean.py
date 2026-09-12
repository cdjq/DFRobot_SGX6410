'''! Run the one-time SGX6410 sensor cleaning sequence.'''
import os
import sys

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))))
from DFRobot_SGX6410 import *

ctype = 0
sensor = DFRobot_SGX6410_I2C(1, SGX_I2C_ADDR) if ctype == 0 else DFRobot_SGX6410_UART(SGX_UART_BAUDRATE, SGX_I2C_ADDR)


def setup():
  # Initialize the sensor. Run cleaning once here, then stop using this demo
  while sensor.begin() == False:
    print('Sensor initialize failed!!')
  print('Sensor initialize success!!')
  print('Cleaning must not be interrupted and should be run only once.')
  # About 60 s. Do not reset or remove power while cleaning
  result = sensor.run_sensor_clean(90000)
  print('Cleaning result: %d' % result)


if __name__ == '__main__':
  setup()
