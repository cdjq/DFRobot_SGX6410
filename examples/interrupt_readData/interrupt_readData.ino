/**
 * @file  interrupt_readData.ino
 * @brief  Read SGX6410 gas data when the INT signal is asserted.
 * @details  INT is high when a sample is ready and returns low after the
 *           result is read. The interrupt only schedules the read; gas data
 *           is still obtained by the host with Get Measurement Results (0x18).
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author      [PLELES](li.jia@dfrobot.com)
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

volatile uint8_t interruptFlag = 0;

/*
 * Select the controller pin connected to the SGX6410 INT output.
 * The same pin is used for both UART and I2C communication modes.
 *
 * ESP32 DevKit: D5 = GPIO5
 * ESP8266 NodeMCU: D6 = GPIO12
 */
#if defined(ESP32)
const uint8_t interruptPin = 5;   // D5 / GPIO5
#elif defined(ESP8266)
const uint8_t interruptPin = 12;  // D6 / GPIO12
#endif

#if defined(ESP8266)
void IRAM_ATTR sgxInterrupt() {
#else
void sgxInterrupt() {
#endif
  if (interruptFlag == 0) {
    interruptFlag = 1;
  }
}

void setup() {
  // Open the debug serial port
  Serial.begin(115200);
  Serial.println("=== Interrupt Read Data Demo ===");

  // Initialize the sensor and verify product ID 0x2310
  while (!sgx6410.begin()) {
     Serial.println("SGX6410 init failed");
  }
  Serial.println("SGX6410 init success!");

  // IAQ: about 3 s sample period, 5 min warm-up
  if (!sgx6410.setOperationMode(DFRobot_SGX6410::SGX_MODE_IAQ)) {
    Serial.println("Failed to set IAQ mode!");
  }
  delay(100);

  /*
   * The SGX6410 INT pin is a push-pull output. It goes high when new data is
   * available and returns low after Get Measurement Results (0x18) is read.
   * Connect INT to an external-interrupt-capable pin on the controller, then
   * select the corresponding interrupt number below.
   *
   * The Correspondence Table of AVR Series Arduino Interrupt Pins and Numbers
   * ---------------------------------------------------------------------------------------
   * |                                        | Digital Pin  | 2  | 3  |                   |
   * |    UNO, Nano, Mini, other 328-based    |--------------------------------------------|
   * |                                        | Interrupt No | 0  | 1  |                   |
   * |-------------------------------------------------------------------------------------|
   * |                                        | Digital Pin  | 2  | 3  | 21 | 20 | 19 | 18 |
   * |               Mega2560                 |--------------------------------------------|
   * |                                        | Interrupt No | 0  | 1  | 2  | 3  | 4  | 5  |
   * |-------------------------------------------------------------------------------------|
   * |                                        | Digital Pin  | 3  | 2  | 0  | 1  | 7  |    |
   * |    Leonardo, other 32U4-based          |--------------------------------------------|
   * |                                        | Interrupt No | 0  | 1  | 2  | 3  | 4  |    |
   * |-------------------------------------------------------------------------------------|
   *
   * micro:bit Interrupt Pins and Numbers
   * -------------------------------------------------------------------------------
   * | micro:bit      | Digital Pin  | P0-P20 can be used as external interrupts.   |
   * |                | Interrupt No | The interrupt number is the pin number.       |
   * |-------------------------------------------------------------------------------|
   *
   * For controllers that use the GPIO pin number as the interrupt number, consult
   * the controller documentation and replace 0 with the selected pin number.
   */
#if defined(ESP32) || defined(ESP8266)
  pinMode(interruptPin, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(interruptPin), sgxInterrupt, RISING);
#else
  attachInterrupt(/* Interrupt No */ 0, sgxInterrupt, RISING);
#endif
}

void loop() {
  // Wait until INT schedules a read; data is still fetched with update()
  if (interruptFlag == 0) {
    return;
  }
  interruptFlag = 0;

  // update() reads the measurement frame and scales it
  if (!sgx6410.update()) {
    Serial.println("Failed to read gas data!");
    return;
  }
  const DFRobot_SGX6410::sSGXData_t &data = sgx6410.getAllGasData();
  if (!data.isNew) return;

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
}
