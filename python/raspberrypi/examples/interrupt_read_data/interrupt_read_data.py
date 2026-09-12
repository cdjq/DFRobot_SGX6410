'''! Read SGX6410 measurements after an INT edge.'''
import os
import sys
import time
import RPi.GPIO as GPIO

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))))
from DFRobot_SGX6410 import *

ctype = 0
I2C_BUS = 1
ADDRESS = SGX_I2C_ADDR
sensor = DFRobot_SGX6410_I2C(I2C_BUS, ADDRESS) if ctype == 0 else DFRobot_SGX6410_UART(SGX_UART_BAUDRATE, ADDRESS)
INT_GPIO = 17
interrupt_flag = False


def on_data_ready(channel):
  global interrupt_flag
  interrupt_flag = True


def setup():
  # Initialize the sensor and verify product ID 0x2310
  while sensor.begin() == False:
    print('Sensor initialize failed!!')
  print('Sensor initialize success!!')
  # IAQ: about 3 s sample period, 5 min warm-up
  sensor.set_operation_mode(DFRobot_SGX6410.SGX_MODE_IAQ)
  # INT goes high when a sample is ready; update() still reads the data
  GPIO.setmode(GPIO.BCM)
  GPIO.setup(INT_GPIO, GPIO.IN)
  GPIO.add_event_detect(INT_GPIO, GPIO.RISING, callback=on_data_ready)


def loop():
  global interrupt_flag
  # Wait until INT schedules a read
  if interrupt_flag == False:
    time.sleep(0.01)
    return
  interrupt_flag = False
  # update() reads the measurement frame and scales it
  if sensor.update() == True:
    data = sensor.get_all_gas_data()
    if data.is_new == True:
      print('IAQ: %.2f, TVOC: %.2f mg/m3, ETOH: %.2f ppm, ECO2: %d ppm, RELIAQ: %.2f, valid: %s' %
            (data.iaq, data.tvoc, data.etoh, data.eco2, data.reliaq, data.valid))
  time.sleep(1)


if __name__ == '__main__':
  setup()
  while True:
    loop()
