/*!
 * @file setHumidityCompensate.ino
 * @brief 设置相对湿度补偿，并打印接口调用结果。
 * @details 将 HUMIDITY_RH 修改为实际环境相对湿度（0.0-100.0 %RH）。
 * @n Open the serial monitor at 115200 baud. Compensation is sent every 5 seconds.
 * @n Set HUMIDITY_RH to 101.0f to demonstrate an invalid-value failure.
 * @n Restore a value in range to demonstrate a successful write with the sensor connected.
 * @n Success means the command was written; this API does not read back the setting.
 * @copyright Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license The MIT License (MIT)
 * @author [PLELES](li.jia@dfrobot.com)
 * @version V1.0.0
 * @date 2026-11-04
 * @url https://github.com/cdjq/DFRobot_SGX6410
 */
#include "DFRobot_SGX6410.h"

// Relative humidity for compensation, unit: %RH, valid range: 0.0-100.0.
#define HUMIDITY_RH 50.0f

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

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Set Humidity Compensation Demo ===");

  while (!sgx6410.begin()) {
    Serial.println("SGX6410 initialization failed. Check the connection.");
    delay(1000);
  }
  Serial.println("SGX6410 initialization successful.");
}

void loop() {
  Serial.print("Compensation humidity: ");
  Serial.print(HUMIDITY_RH, 1);
  Serial.println(" %RH");

  if (sgx6410.setHumidityCompensate(HUMIDITY_RH)) {
    Serial.println("Humidity compensation setting successful.");
  } else {
    Serial.println("Humidity compensation setting failed: invalid humidity or command write failed.");
  }
  Serial.println("--------------------");
  delay(5000);
}
