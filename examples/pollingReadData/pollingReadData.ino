/**
 * @file  pollingReadData.ino
 * @brief  Read SGX6410 gas data by polling.
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author      [PLELES](li.jia@dfrobot.com)
 * @version     V1.0.0
 * @date        2026-09-03
 * @url         https://github.com/cdjq/DFRobot_SGX6410
 */
#include "DFRobot_SGX6410.h"

/* >> 1. Choose one communication method below. */
// #define SGX6410_COMM_UART
#define SGX6410_COMM_I2C

#if defined(SGX6410_COMM_UART)
/* ---------------------------------------------------------------------------------------------------------
 *   SGX6410  |      MCU       | Leonardo/Mega2560/M0 | UNO | ESP8266 | ESP32 | Other Serial1 boards
 *     VCC    |      3.3V      |        3.3V           | 3.3V|   3.3V  | 3.3V  |        3.3V
 *     GND    |      GND       |        GND            | GND |   GND   | GND   |        GND
 *      RX    |      TX        |     Serial1 TX1       |  7  |  GPIO5  |  TX1  |     Serial1 TX
 *      TX    |      RX        |     Serial1 RX1       |  6  |  GPIO4  |  RX1  |     Serial1 RX
 * --------------------------------------------------------------------------------------------------------- */
#if defined(ARDUINO_AVR_UNO) || defined(ESP8266)
#include <SoftwareSerial.h>
#endif
#if defined(ARDUINO_AVR_UNO)
SoftwareSerial       sgxSerial(/*rx =*/6, /*tx =*/7);
DFRobot_SGX6410_UART sgx6410(&sgxSerial, SGX_UART_BAUDRATE);
#elif defined(ESP8266)
SoftwareSerial       sgxSerial(/*rx =*/4, /*tx =*/5);
DFRobot_SGX6410_UART sgx6410(&sgxSerial, SGX_UART_BAUDRATE);
#elif defined(ESP32)
// Use Serial1 default pins; pass RX/TX explicitly to match custom wiring.
DFRobot_SGX6410_UART sgx6410(&Serial1, SGX_UART_BAUDRATE);
#else
DFRobot_SGX6410_UART sgx6410(&Serial1, SGX_UART_BAUDRATE);
#endif
#elif defined(SGX6410_COMM_I2C)
/* ---------------------------------------------------------------------------------------------------------
 *   SGX6410  |      MCU       | Leonardo/Mega2560/M0 | UNO | ESP8266 | ESP32 | Other Wire boards
 *     VCC    |      3.3V      |        3.3V           | 3.3V|   3.3V  | 3.3V  |        3.3V
 *     GND    |      GND       |        GND            | GND |   GND   | GND   |        GND
 *     SCL    |      SCL       |        SCL            | SCL |  GPIO5  | SCL   |        SCL
 *     SDA    |      SDA       |        SDA            | SDA |  GPIO4  | SDA   |        SDA
 * --------------------------------------------------------------------------------------------------------- */
DFRobot_SGX6410_I2C sgx6410(&Wire, SGX_I2C_ADDR);
#else
#error "Select SGX6410_COMM_UART or SGX6410_COMM_I2C"
#endif

void setup()
{
  // Open the debug serial port
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Polling Read Data Demo ===");

  // Initialize the sensor and read the current operation mode
  while (!sgx6410.begin()) {
    Serial.println("SGX6410 init fail!");
  }
  Serial.println("SGX6410 init success!");

  // IAQ: about 3 s sample period, 5 min warm-up
  if (sgx6410.setOperationMode(SGX_MODE_IAQ) != SGX_FDL_OK) {
    Serial.println("Failed to set IAQ mode!");
  }
  Serial.println("IAQ mode set success!");
  delay(100);
}

void loop()
{
  // update() reads the measurement frame and scales it
  if (sgx6410.update()) {
    const sSGXData_t &data = sgx6410.getAllGasData();
    // Skip if the sample counter has not changed
    if (!data.isNew) {
      delay(500);
      return;
    }
    // Print gas data. valid stays false during datasheet warm-up
    Serial.print("IAQ:      ");
    Serial.println(data.IAQ, 2);
    Serial.print("TVOC:     ");
    Serial.print(data.TVOC, 2);
    Serial.println(" mg/m3");
    Serial.print("ETOH:     ");
    Serial.print(data.ETOH, 2);
    Serial.println(" ppm");
    Serial.print("ECO2:     ");
    Serial.print((uint16_t)data.ECO2);
    Serial.println(" ppm");
    Serial.print("RELIAQ:   ");
    Serial.println(data.RELIAQ, 2);
    Serial.print("Data valid: ");
    Serial.println(data.valid ? "YES" : "NO (warming up)");
    Serial.println("--------------------");
  } else {
    Serial.println("Failed to read gas data!");
  }

  delay(1000);
}
