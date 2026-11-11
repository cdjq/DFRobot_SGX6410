'''! Persist the SGX6410 startup operation mode.'''

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
startup_mode = DFRobot_SGX6410.SGX_MODE_IAQ
if 'sgx6410_comm_uart' in globals():
  sgx6410 = DFRobot_SGX6410_UART(sgx_uart_baudrate, sgx_i2c_addr)
else:
  sgx6410 = DFRobot_SGX6410_I2C(1, sgx_i2c_addr)


def get_mode_name(mode):
  return {
    DFRobot_SGX6410.SGX_MODE_SUSPEND: 'SUSPEND',
    DFRobot_SGX6410.SGX_MODE_IAQ: 'IAQ',
    DFRobot_SGX6410.SGX_MODE_ULP: 'ULP',
    DFRobot_SGX6410.SGX_MODE_PBAQ: 'PBAQ',
  }.get(mode, 'UNKNOWN')


def setup():
  print('=== Change Startup Mode Demo ===')
  # Initialize the sensor and verify product ID 0x2310
  while not sgx6410.begin():
    print('SGX6410 init fail!')
    time.sleep(1)
  print('SGX6410 initialized successfully!')
  print('Changing startup mode to %s...' % get_mode_name(startup_mode))
  print('Keep power stable during this process.')

  # Update the setting and commit it to Flash
  status = sgx6410.set_operation_mode(startup_mode, save=True)
  if status is None:
    print('Failed to receive Flash write status')
    return
  if status != sgx_fdl_ok:
    print('Flash write failed with status: 0x%X' % status)
    return
  print('Flash write: OK')
  print('Startup mode saved to Flash.')

  # Reset so the new startup mode takes effect
  print('Now resetting sensor...')
  if not sgx6410.reset():
    print('Failed to reset sensor')
    return
  print('Sensor will restart with %s mode.' % get_mode_name(startup_mode))


if __name__ == '__main__':
  setup()
