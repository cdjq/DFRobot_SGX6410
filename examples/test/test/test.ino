/**
 * @file  sensorClean.ino
 * @brief Run the SGX6410 one-time sensor cleaning sequence on request.
 *
 * Cleaning takes about 60 seconds and must not be interrupted. Start it by
 * sending 'c' in the Serial Monitor. The example does not start cleaning
 * automatically at boot.
 * @copyright Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license The MIT License (MIT)
 * @author [PLELES](li.jia@dfrobot.com)
 * @version V1.0.0
 * @date 2026-09-03
 * @url https://github.com/cdjq/DFRobot_SGX6410
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
  DFRobot_SGX6410_I2C sgx6410(&Wire, SGX_I2C_ADDR,8,7);
#else
  #error "Select SGX6410_COMM_UART or SGX6410_COMM_I2C"
#endif

void setup() {
  // Open the debug serial port
  Serial.begin(115200);
  Serial.println("=== Sensor Clean Demo ===");

  // Initialize the sensor. Cleaning is not started at boot
  while (!sgx6410.begin()) {
    Serial.println("SGX6410 init fail!");
  }
   Serial.println("SGX6410 init sucess!");
  delay(200);
  if(!sgx6410.setOperationMode(DFRobot_SGX6410::SGX_MODE_SENSOR_CLEAN)){
    Serial.println("set mode failed");
    while(1);
  }
  Serial.println("SetMode OK");
  if(!sgx6410.getOperationMode()){
    Serial.println("get mode failed");
  }
  Serial.println("get mode OK");
  // Serial.println("SGX6410 init success!");
  // Serial.println("Send 'c' to start one-time sensor cleaning.");
  // Serial.println("Warning: keep power stable; do not interrupt cleaning.");
}

void loop() {
  // // Wait for 'c'. Cleaning is one-time and should not be started automatically
  // if (!Serial.available()) return;
  // char command = (char)Serial.read();
  // if (command != 'c' && command != 'C') return;

  // // About 60 s. Do not reset or remove power while cleaning
  // Serial.println("Cleaning started; wait at least 60 seconds...");
  // uint8_t result = sgx6410.runSensorClean(90000);
  // switch (result) {
  //   case CLEAN_OK:
  //     Serial.println("Cleaning completed successfully.");
  //     break;
  //   case CLEAN_DONE:
  //     Serial.println("Cleaning was already performed.");
  //     break;
  //   case CLEAN_TIMEOUT:
  //     Serial.println("Cleaning timed out; keep the sensor powered and inspect it.");
  //     break;
  //   default:
  //     Serial.println("Cleaning failed due to communication error.");
  //     break;
  // }
}