'''! Persist a new SGX6410 I2C address.'''

import os
import sys
import time

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))))
from DFRobot_SGX6410 import *

# Choose one communication method: uncomment only one line below.
# sgx6410_comm_uart = True
sgx6410_comm_i2c = True

if ('sgx6410_comm_uart' in globals()) == ('sgx6410_comm_i2c' in globals()):
  raise ValueError('Select exactly one of sgx6410_comm_uart or sgx6410_comm_i2c')
old_address = 0x38
new_address = 0x39
if 'sgx6410_comm_uart' in globals():
  sgx6410 = DFRobot_SGX6410_UART(sgx_uart_baudrate, old_address)
else:
  sgx6410 = DFRobot_SGX6410_I2C(1, old_address)


def setup():
  # Initialize with the current (old) address
  while not sgx6410.begin():
    print('SGX6410 init fail!')
    time.sleep(1)
  print('SGX6410 init successfully!')
  print('Changing I2C address from 0x%X to 0x%X' % (old_address, new_address))
  print('Keep power stable during this process.')

  # Write the new address to Flash Shadow, then commit Flash
  if not sgx6410.write_flash(sgx_i2c_address_shadow_addr, new_address):
    print('Failed to write I2C address to Flash Shadow')
    return
  status = sgx6410.save_flash()
  if status is None:
    print('Failed to receive Flash write status')
    return
  if status != sgx_fdl_ok:
    print('Flash write failed with status: 0x%X' % status)
    return
  print('Flash write: OK')
  print('I2C address saved to Flash.')

  # Reset so the new I2C address takes effect
  print('Now resetting sensor...')
  if not sgx6410.reset():
    print('Failed to reset sensor')
    return
  print('Sensor will restart with new address (0x%X).' % new_address)
  print('Please reconnect using the new address!')


if __name__ == '__main__':
  setup()
