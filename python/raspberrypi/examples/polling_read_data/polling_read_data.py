'''! Read SGX6410 measurements by polling.'''

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
i2c_bus = 1
address = sgx_i2c_addr
if 'sgx6410_comm_uart' in globals():
  sgx6410 = DFRobot_SGX6410_UART(sgx_uart_baudrate, address)
else:
  sgx6410 = DFRobot_SGX6410_I2C(i2c_bus, address)


def setup():
  print('=== Polling Read Data Demo ===')
  # Initialize the sensor and verify product ID 0x2310
  while sgx6410.begin() == False:
    print('SGX6410 init fail!')
    time.sleep(1)
  print('SGX6410 init success!')
  # IAQ: about 3 s sample period, 5 min warm-up
  if not sgx6410.set_operation_mode(DFRobot_SGX6410.SGX_MODE_IAQ):
    print('Failed to set IAQ mode!')
  else:
    print('IAQ mode set success!')


def loop():
  # update() reads the measurement frame and scales it
  if sgx6410.update() == True:
    data = sgx6410.get_all_gas_data()
    # Print only a new sample. valid stays false during datasheet warm-up
    if data.is_new == True:
      print('IAQ:      %.2f' % data.iaq)
      print('TVOC:     %.2f mg/m3' % data.tvoc)
      print('ETOH:     %.2f ppm' % data.etoh)
      print('ECO2:     %.0f ppm' % data.eco2)
      print('RELIAQ:   %.2f' % data.reliaq)
      print('Data valid: %s' % ('YES' if data.valid else 'NO (warming up)'))
      print('--------------------')
  else:
    print('Failed to read gas data!')
  time.sleep(1)


if __name__ == '__main__':
  setup()
  while True:
    loop()
