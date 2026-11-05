/*!
 * @file changeStartupMode.ino
 * @brief Change and persist the SGX6410 startup operation mode.
 * @details Writes the startup mode to Flash Shadow and commits it to data Flash.
 * @n Do not reset or remove power while saveFlash() is writing data Flash.
 * @copyright Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license The MIT License (MIT)
 * @author [PLELES](li.jia@dfrobot.com)
 * @version V1.0.0
 * @date 2026-09-03
 * @url https://github.com/cdjq/DFRobot_SGX6410
 */
#include "DFRobot_SGX6410.h"

const uint8_t STARTUP_MODE = SGX_MODE_IAQ;

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

const char* getModeName(uint8_t mode) {
  switch (mode) {
    case SGX_MODE_SUSPEND:     return "SUSPEND";
    case SGX_MODE_IAQ:         return "IAQ";
    case SGX_MODE_ULP:         return "ULP";
    case SGX_MODE_PBAQ:        return "PBAQ";
    default: return "UNKNOWN";
  }
}

void setup() {
  // Open the debug serial port
  Serial.begin(115200);
  Serial.println("=== Change Startup Mode Demo ===");

  // Initialize the sensor and verify product ID 0x2310
  while (!sgx6410.begin()) {
    Serial.println("SGX6410 init fail!");
  }
  Serial.println("SGX6410 initialized successfully!");
  Serial.print("Changing startup mode to ");
  Serial.print(getModeName(STARTUP_MODE));
  Serial.println("...");
  Serial.println("Keep power stable during this process.");

  // Write the startup mode to Flash Shadow, then commit Flash
  if (!sgx6410.writeFlash(SGX_STARTUP_MODE_SHADOW_ADDR, STARTUP_MODE)) {
    Serial.println("Failed to write startup mode to Flash Shadow");
    return;
  }

  uint8_t status = SGX_FDL_PARAMETER_ERROR;
  if (!sgx6410.saveFlash(status)) {
    Serial.println("Failed to receive Flash write status");
    return;
  }
  if (status != SGX_FDL_OK) {
    Serial.print("Flash write failed with status: 0x");
    Serial.println(status, HEX);
    return;
  }
  Serial.println("Flash write: OK");

  Serial.println("Startup mode saved to Flash.");
  // Reset so the new startup mode takes effect
  Serial.println("Now resetting sensor...");
  if (!sgx6410.reset()) {
    Serial.println("Failed to reset sensor");
    return;
  }
  Serial.print("Sensor will restart with ");
  Serial.print(getModeName(STARTUP_MODE));
  Serial.println(" mode.");
}

void loop() {
  // Startup mode change runs once in setup()
}