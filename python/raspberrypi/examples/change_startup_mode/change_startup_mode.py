'''! Persist the SGX6410 startup operation mode.'''
import os
import sys

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))))
from DFRobot_SGX6410 import *

ctype = 0
STARTUP_MODE = DFRobot_SGX6410.SGX_MODE_IAQ
sensor = DFRobot_SGX6410_I2C(1, SGX_I2C_ADDR) if ctype == 0 else DFRobot_SGX6410_UART(SGX_UART_BAUDRATE, SGX_I2C_ADDR)


def setup():
  # Initialize the sensor and verify product ID 0x2310
  while sensor.begin() == False:
    print('Sensor initialize failed!!')
  # Write the startup mode to Flash Shadow, then commit Flash
  if sensor.write_flash(SGX_STARTUP_MODE_SHADOW_ADDR, STARTUP_MODE) == False:
    print('Failed to write startup mode shadow')
    return
  status = sensor.save_flash()
  print('Flash status: 0x%02X' % status if status is not None else 'No Flash status')
  if status == SGX_FDL_OK:
    # Reset so the new startup mode takes effect
    sensor.reset()
    print('Startup mode saved. It becomes active after reset.')


if __name__ == '__main__':
  setup()
