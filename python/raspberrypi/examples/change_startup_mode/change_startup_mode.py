'''! Persist the SGX6410 startup operation mode.'''

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
STARTUP_MODE = DFRobot_SGX6410.SGX_MODE_IAQ
if 'SGX6410_COMM_UART' in globals():
  sgx6410 = DFRobot_SGX6410_UART(SGX_UART_BAUDRATE, SGX_I2C_ADDR)
else:
  sgx6410 = DFRobot_SGX6410_I2C(1, SGX_I2C_ADDR)


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
  print('Changing startup mode to %s...' % get_mode_name(STARTUP_MODE))
  print('Keep power stable during this process.')

  # Write the startup mode to Flash Shadow, then commit Flash
  if not sgx6410.write_flash(SGX_STARTUP_MODE_SHADOW_ADDR, STARTUP_MODE):
    print('Failed to write startup mode to Flash Shadow')
    return
  status = sgx6410.save_flash()
  if status is None:
    print('Failed to receive Flash write status')
    return
  if status != SGX_FDL_OK:
    print('Flash write failed with status: 0x%X' % status)
    return
  print('Flash write: OK')
  print('Startup mode saved to Flash.')

  # Reset so the new startup mode takes effect
  print('Now resetting sensor...')
  if not sgx6410.reset():
    print('Failed to reset sensor')
    return
  print('Sensor will restart with %s mode.' % get_mode_name(STARTUP_MODE))


if __name__ == '__main__':
  setup()
