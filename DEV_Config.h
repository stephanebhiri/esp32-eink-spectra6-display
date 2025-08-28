/**
 * Hardware Configuration for ESP32 E-Ink Display
 * 
 * Pin mapping and hardware abstraction for ESP32 microcontrollers
 * connected to Waveshare 13.3" E-Paper displays. Optimized for
 * Adafruit HUZZAH32 Feather with battery operation support.
 * 
 * @author Stephane Bhiri
 * @version 2.0
 * @date January 2025
 */

#pragma once
#include <Arduino.h>
#include <SPI.h>

// Data type definitions
typedef uint8_t  UBYTE;
typedef uint16_t UWORD;
typedef uint32_t UDOUBLE;

// SPI Configuration
#define SPI_SPEED_HZ    10000000  // 10MHz - tested stable with long cables

// Hardware SPI pins (HUZZAH32 Feather)
#define EPD_SCK_PIN      5    // Hardware SCK
#define EPD_MOSI_PIN    18    // Hardware MOSI

// Display control pins - optimized for HUZZAH32 Feather GPIO layout
#define EPD_CS_M_PIN    33    // Chip Select Master (GPIO33)
#define EPD_CS_S_PIN    15    // Chip Select Slave (GPIO15)
#define EPD_DC_PIN      14    // Data/Command (GPIO14)
#define EPD_RST_PIN     32    // Reset (GPIO32)
#define EPD_BUSY_PIN    27    // Busy status (GPIO27)

// Optional power control - can be tied to VCC for always-on operation
#define EPD_PWR_PIN     21    // Power control (GPIO21)

// Hardware abstraction macros
#define DEV_Digital_Write(pin, val) digitalWrite((pin), (val))
#define DEV_Digital_Read(_pin) digitalRead(_pin)
#define DEV_Delay_ms(__xms) delay(__xms)

// Function declarations
int DEV_Module_Init(void);
void DEV_Module_Exit(void);
void DEV_SPI_Write_nByte(UBYTE* pData, uint32_t len);
void DEV_SPI_WriteByte(UBYTE data);