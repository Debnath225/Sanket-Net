#ifndef SANKATNET_PIN_CONFIG_H
#define SANKATNET_PIN_CONFIG_H

/*
 * ============================================================
 * SANKAT-NET
 * Hardware Pin Configuration
 * ============================================================
 *
 * MCU:
 *   ESP32-WROOM-32 / ESP32 DevKit V1
 *
 * IMPORTANT:
 *   This file contains ONLY hardware pin assignments.
 *   No sensor logic should be placed here.
 * ============================================================
 */

#include <Arduino.h>

namespace Pins {

// ============================================================
// I2C BUS
// ============================================================

constexpr uint8_t I2C_SDA = 21;
constexpr uint8_t I2C_SCL = 22;


// ============================================================
// SPI BUS — SX1278
// ============================================================

constexpr uint8_t LORA_SCK  = 18;
constexpr uint8_t LORA_MISO = 19;
constexpr uint8_t LORA_MOSI = 23;

constexpr uint8_t LORA_NSS  = 5;
constexpr uint8_t LORA_RST  = 14;
constexpr uint8_t LORA_DIO0 = 26;


// ============================================================
// GPS — UART2
// ============================================================

constexpr uint8_t GPS_RX = 16;
constexpr uint8_t GPS_TX = 17;


// ============================================================
// DS18B20
// ============================================================

constexpr uint8_t DS18B20 = 27;


// ============================================================
// WEATHER SENSORS
// ============================================================

// Anemometer pulse input
constexpr uint8_t ANEMOMETER = 25;

// Rain gauge pulse input
constexpr uint8_t RAIN_GAUGE = 33;


// ============================================================
// FIRE SENSOR
// ============================================================

constexpr uint8_t FLAME_SENSOR = 13;


// ============================================================
// LOCAL ALARM
// ============================================================

constexpr uint8_t BUZZER = 4;


// ============================================================
// STATUS LED
// ============================================================

constexpr uint8_t STATUS_LED = 2;


// ============================================================
// BATTERY MONITOR
// ============================================================

constexpr uint8_t BATTERY_ADC = 34;


// ============================================================
// ADC BUS
// ============================================================
//
// ADS1115 #1
//
// 0x48
//   A0 = MQ-2
//   A1 = MQ-135
//   A2 = Water Level
//   A3 = Soil Moisture
//
// ADS1115 #2
//
// 0x49
//   A0 = Turbidity
//   A1 = pH
//   A2 = TDS
//   A3 = Reserved
//
// ============================================================

namespace ADC1 {

constexpr uint8_t MQ2 = 0;
constexpr uint8_t MQ135 = 1;
constexpr uint8_t WATER_LEVEL = 2;
constexpr uint8_t SOIL_MOISTURE = 3;

}

namespace ADC2 {

constexpr uint8_t TURBIDITY = 0;
constexpr uint8_t PH = 1;
constexpr uint8_t TDS = 2;
constexpr uint8_t RESERVED = 3;

}


// ============================================================
// I2C ADDRESSES
// ============================================================

namespace I2CAddress {

constexpr uint8_t BME280 = 0x76;

constexpr uint8_t ADS1115_1 = 0x48;
constexpr uint8_t ADS1115_2 = 0x49;

constexpr uint8_t MPU6050 = 0x68;

}


// ============================================================
// HARDWARE BUS SPEED
// ============================================================

constexpr uint32_t I2C_FREQUENCY = 400000;

constexpr uint32_t GPS_BAUDRATE = 9600;

constexpr uint32_t SERIAL_BAUDRATE = 115200;


// ============================================================
// HARDWARE VALIDATION
// ============================================================

static_assert(
    I2C_SDA != I2C_SCL,
    "I2C SDA and SCL cannot use the same GPIO"
);

static_assert(
    LORA_SCK != LORA_MISO,
    "LoRa SPI pins conflict"
);

static_assert(
    LORA_SCK != LORA_MOSI,
    "LoRa SPI pins conflict"
);

static_assert(
    LORA_MISO != LORA_MOSI,
    "LoRa SPI pins conflict"
);

}

#endif