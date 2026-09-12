'''! Persist a new SGX6410 I2C address.'''
import os
import sys

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))))
from DFRobot_SGX6410 import *

ctype = 0
OLD_ADDRESS = 0x38
NEW_ADDRESS = 0x39
sensor = DFRobot_SGX6410_I2C(1, OLD_ADDRESS) if ctype == 0 else DFRobot_SGX6410_UART(SGX_UART_BAUDRATE, OLD_ADDRESS)


def setup():
  # Initialize with the current (old) address
  while sensor.begin() == False:
    print('Sensor initialize failed!!')
  # Write the new address to Flash Shadow, then commit Flash
  if sensor.write_flash(SGX_I2C_ADDRESS_SHADOW_ADDR, NEW_ADDRESS) == False:
    print('Failed to write I2C address shadow')
    return
  status = sensor.save_flash()
  print('Flash status: 0x%02X' % status if status is not None else 'No Flash status')
  if status == SGX_FDL_OK:
    # Reset so the new I2C address takes effect
    sensor.reset()
    print('Address saved. Recreate the driver with 0x%02X after reset.' % NEW_ADDRESS)


if __name__ == '__main__':
  setup()
