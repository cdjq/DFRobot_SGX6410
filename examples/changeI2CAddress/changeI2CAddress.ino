/*!
 * @file changeI2CAddress.ino
 * @brief Change and persist the SGX6410 I2C slave address.
 * @details Writes the address to Flash Shadow, saves it to data Flash, then resets the SGX6410.
 * @n Do not reset or remove power while saveFlash() is writing data Flash.
 * @copyright Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license The MIT License (MIT)
 * @author [PLELES](li.jia@dfrobot.com)
 * @version V1.0.0
 * @date 2026-09-03
 * @url https://github.com/cdjq/DFRobot_SGX6410
 */
#include "DFRobot_SGX6410.h"

// Default is 0x38. Set OLD to the address currently answering on the bus.
// Set NEW to the address to persist (bits 6:0, 0x00-0x7F).
const uint8_t OLD_I2C_ADDRESS = 0x38;  // Factory address
const uint8_t NEW_I2C_ADDRESS = 0x39;  // Example target address

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
    SoftwareSerial sgxSerial(/*rx =*/6, /*tx =*/7);
    DFRobot_SGX6410_UART sgx6410(&sgxSerial, SGX_UART_BAUDRATE);
  #elif defined(ESP8266)
    SoftwareSerial sgxSerial(/*rx =*/4, /*tx =*/5);
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
  DFRobot_SGX6410_I2C sgx6410(&Wire, OLD_I2C_ADDRESS);
#else
  #error "Select SGX6410_COMM_UART or SGX6410_COMM_I2C"
#endif

void setup() {
  // Open the debug serial port
  Serial.begin(115200);
  Serial.println("=== Change I2C Address Demo ===");
  delay(1000);

  // Initialize with the current (old) address
  while (!sgx6410.begin()) {
    Serial.println("SGX6410 init fail!");
  }
  Serial.println("SGX6410 init successfully!");
  Serial.print("Changing I2C address from 0x");
  Serial.print(OLD_I2C_ADDRESS, HEX);
  Serial.print(" to 0x");
  Serial.println(NEW_I2C_ADDRESS, HEX);
  Serial.println("Keep power stable during this process.");

  // Write the new address to Flash Shadow, then commit Flash
  uint8_t status = sgx6410.setI2CAddress(NEW_I2C_ADDRESS, true);
  if (status != SGX_FDL_OK) {
    Serial.print("Address change failed with status: 0x");
    Serial.println(status, HEX);
    return;
  }
  Serial.println("Flash write: OK");

  Serial.println("I2C address saved to Flash.");
  // Reset so the new I2C address takes effect
  Serial.println("Now resetting sensor...");
  if (!sgx6410.reset()) {
    Serial.println("Failed to reset sensor");
    return;
  }
  Serial.print("Sensor will restart with new address (0x");
  Serial.print(NEW_I2C_ADDRESS, HEX);
  Serial.println(").");
  Serial.println("Please reconnect using the new address!");
}

void loop() {
  // Address change runs once in setup()
}
