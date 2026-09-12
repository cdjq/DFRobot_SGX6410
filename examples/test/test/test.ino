/**
 * @file  interrupt_readData.ino
 * @brief  Read SGX6410 gas data when the INT signal is asserted.
 * @details  INT is high when a sample is ready and returns low after the
 *           result is read. The interrupt only schedules the read; gas data
 *           is still obtained by the host with Get Measurement Results (0x18).
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author [PLELES](li.jia@dfrobot.com)
 * @version     V1.0.0
 * @date        2026-09-03
 * @url         https://github.com/cdjq/DFRobot_SGX6410
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
    DFRobot_SGX6410_UART sgx6410(&Serial1, SGX_UART_BAUDRATE,
                                 /*rx =*/38, /*tx =*/37);
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

volatile bool dataReady = false;

#if defined(ESP8266)
void IRAM_ATTR sgxInterrupt() {
#else
void sgxInterrupt() {
#endif
  dataReady = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("=== Interrupt Read Data Demo ===");

  while (!sgx6410.begin()) {
    Serial.println("SGX6410 init fail!");
  }
  Serial.println("SGX6410 init success!");

  if (!sgx6410.setOperationMode(DFRobot_SGX6410::SGX_MODE_IAQ)) {
    Serial.println("Failed to set IAQ mode!");
  }
  delay(100);

  // Same interrupt pin choices as the BMP58X interrupt example.
#if defined(ESP32)
  const uint8_t interruptPin = 20;  // D6
  pinMode(interruptPin, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(interruptPin), sgxInterrupt, RISING);
#elif defined(ESP8266)
  const uint8_t interruptPin = 13;  // I2C example: D7
  // Use an external 10 kOhm pulldown if INPUT_PULLDOWN is unavailable.
  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), sgxInterrupt, RISING);
#elif defined(ARDUINO_SAM_ZERO)
  const uint8_t interruptPin = 6;
  pinMode(interruptPin, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(interruptPin), sgxInterrupt, RISING);
#else
  // UNO: use digital pin 2 as interrupt pin.
  const uint8_t interruptPin = 2;
  // UNO has no universal INPUT_PULLDOWN; add a 10 kOhm pulldown to GND.
  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), sgxInterrupt, RISING);
#endif
}

void loop() {
  if (!dataReady) {
    return;
  }
  dataReady = false;

  if (!sgx6410.update()) {
    Serial.println("Failed to read gas data!");
    return;
  }
  const DFRobot_SGX6410::sSGXData_t &data = sgx6410.getAllGasData();
  if (!data.isNew) return;

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
}