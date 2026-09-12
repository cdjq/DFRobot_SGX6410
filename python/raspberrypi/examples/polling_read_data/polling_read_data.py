'''! Read SGX6410 measurements by polling.'''
import os
import sys
import time

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))))
from DFRobot_SGX6410 import *

ctype = 0
I2C_BUS = 1
ADDRESS = SGX_I2C_ADDR
sensor = DFRobot_SGX6410_I2C(I2C_BUS, ADDRESS) if ctype == 0 else DFRobot_SGX6410_UART(SGX_UART_BAUDRATE, ADDRESS)


def setup():
  # Initialize the sensor and verify product ID 0x2310
  while sensor.begin() == False:
    print('Sensor initialize failed!!')
  print('Sensor initialize success!!')
  # IAQ: about 3 s sample period, 5 min warm-up
  sensor.set_operation_mode(DFRobot_SGX6410.SGX_MODE_IAQ)


def loop():
  # update() reads the measurement frame and scales it
  if sensor.update() == True:
    data = sensor.get_all_gas_data()
    # Print only a new sample. valid stays false during datasheet warm-up
    if data.is_new == True:
      print('IAQ: %.2f' % data.iaq)
      print('TVOC: %.2f mg/m3' % data.tvoc)
      print('ETOH: %.2f ppm' % data.etoh)
      print('ECO2: %d ppm' % data.eco2)
      print('RELIAQ: %.2f' % data.reliaq)
      print('Data valid: %s' % ('YES' if data.valid else 'NO (warming up)'))
  time.sleep(1)


if __name__ == '__main__':
  setup()
  while True:
    loop()
