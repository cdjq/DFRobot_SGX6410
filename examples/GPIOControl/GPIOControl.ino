/**
 * @file  GPIOControl.ino
 * @brief Demonstrate configuring and controlling SGX6410 GPIO1.
 * @copyright Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license The MIT License (MIT)
 * @author [PLELES](li.jia@dfrobot.com)
 * @version V1.0.0
 * @date 2026-09-03
 * @url https://github.com/cdjq/DFRobot_SGX6410
 */
#include "DFRobot_SGX6410.h"

/* >> 1. Choose one communication method below. */
#define SGX6410_COMM_UART
// #define SGX6410_COMM_I2C

#if defined(SGX6410_COMM_UART)
/* ---------------------------------------------------------------------------------------------------------
 *   SGX6410  |      MCU       | Leonardo/Mega2560/M0 | UNO | ESP8266 | ESP32 | Other Serial1 boards
 *     VCC    |      3.3V      |        3.3V           | 3.3V|   3.3V  | 3.3V  |        3.3V
 *     GND    |      GND       |        GND            | GND |   GND   | GND   |        GND
 *      RX    |      TX        |     Serial1 TX1       |  7  |  GPIO5  |  37   |     Serial1 TX
 *      TX    |      RX        |     Serial1 RX1       |  6  |  GPIO4  |  38   |     Serial1 RX
 * --------------------------------------------------------------------------------------------------------- */
  #if defined(ARDUINO_AVR_UNO) || defined(ESP8266)
    #include <SoftwareSerial.h>
  #endif
  #if defined(ARDUINO_AVR_UNO)
    SoftwareSerial sgxSerial(/*rx =*/6, /*tx =*/7);
    DFRobot_SGX6410_UART sgx6410(&sgxSerial, SGX_UART_BAUDRATE);
  #elif defined(ESP8266)
    SoftwareSerial sgxSerial(/*rx =*/4, /*tx =*/5);
    DFRobot_SGX6410_UART sgx6410(&sgxSerial, SGX_UART_BAUDRATE);
  #elif defined(ESP32)
    DFRobot_SGX6410_UART sgx6410(&Serial1, SGX_UART_BAUDRATE, /*rx =*/38, /*tx =*/37);
  #elif defined(ARDUINO_BBC_MICROBIT) && !defined(ARDUINO_BBC_MICROBIT_V2)
    #error "BBC micro:bit has no usable UART. Select SGX6410_COMM_I2C."
  #else
    DFRobot_SGX6410_UART sgx6410(&Serial1, SGX_UART_BAUDRATE);
  #endif
#elif defined(SGX6410_COMM_I2C)
  DFRobot_SGX6410_I2C sgx6410(&Wire, SGX_I2C_ADDR);
#else
  #error "Select SGX6410_COMM_UART or SGX6410_COMM_I2C"
#endif

static void printGPIOValue() {
  uint8_t value = 0;
  if (!sgx6410.getGPIO(value)) {
    Serial.println("Failed to read GPIO value");
    return;
  }

  // GPIO1 is bit 0 of the returned pin-value byte.
  Serial.print("GPIO1: ");
  Serial.println((value & 0x01) ? "HIGH" : "LOW");
}

void setup() {
  // Open the debug serial port
  Serial.begin(115200);
  Serial.println("=== GPIO Control Demo ===");

  // Initialize the sensor and verify product ID 0x2310
  while (!sgx6410.begin()) {
    Serial.println("SGX6410 init fail!");
  }
  Serial.println("SGX6410 init success!");

  // configGPIO(0x01) configures GPIO1 as an output
  if (!sgx6410.configGPIO(0x01)) {
    Serial.println("Failed to configure GPIO1 as output");
  } else {
    Serial.println("GPIO1 configured as output");
  }
}

void loop() {
  // Toggle GPIO1 high, then low, and print bit0
  if (!sgx6410.setGPIO()) {
    Serial.println("Failed to set GPIO1; check the SGX6410 connection");
    delay(1000);
    return;
  }
  Serial.println("Set GPIO1");
  printGPIOValue();
  delay(1000);

  if (!sgx6410.clearGPIO()) {
    Serial.println("Failed to clear GPIO1; check the SGX6410 connection");
    delay(1000);
    return;
  }
  Serial.println("Clear GPIO1");
  printGPIOValue();
  delay(1000);
}