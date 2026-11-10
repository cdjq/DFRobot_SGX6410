'''! Persist a new SGX6410 I2C address.'''

import os
import sys
import time

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))))
from DFRobot_SGX6410 import *

# Choose one communication method: uncomment only one line below.
# SGX6410_COMM_UART = True
SGX6410_COMM_I2C = True

if ('SGX6410_COMM_UART' in globals()) == ('SGX6410_COMM_I2C' in globals()):
  raise ValueError('Select exactly one of SGX6410_COMM_UART or SGX6410_COMM_I2C')
OLD_ADDRESS = 0x38
NEW_ADDRESS = 0x39
if 'SGX6410_COMM_UART' in globals():
  sgx6410 = DFRobot_SGX6410_UART(SGX_UART_BAUDRATE, OLD_ADDRESS)
else:
  sgx6410 = DFRobot_SGX6410_I2C(1, OLD_ADDRESS)


def setup():
  # Initialize with the current (old) address
  while not sgx6410.begin():
    print('SGX6410 init fail!')
    time.sleep(1)
  print('SGX6410 init successfully!')
  print('Changing I2C address from 0x%X to 0x%X' % (OLD_ADDRESS, NEW_ADDRESS))
  print('Keep power stable during this process.')

  # Write the new address to Flash Shadow, then commit Flash
  if not sgx6410.write_flash(SGX_I2C_ADDRESS_SHADOW_ADDR, NEW_ADDRESS):
    print('Failed to write I2C address to Flash Shadow')
    return
  status = sgx6410.save_flash()
  if status is None:
    print('Failed to receive Flash write status')
    return
  if status != SGX_FDL_OK:
    print('Flash write failed with status: 0x%X' % status)
    return
  print('Flash write: OK')
  print('I2C address saved to Flash.')

  # Reset so the new I2C address takes effect
  print('Now resetting sensor...')
  if not sgx6410.reset():
    print('Failed to reset sensor')
    return
  print('Sensor will restart with new address (0x%X).' % NEW_ADDRESS)
  print('Please reconnect using the new address!')


if __name__ == '__main__':
  setup()
