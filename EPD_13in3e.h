/**
 * Waveshare 13.3" E-Paper Display Driver
 * 
 * Hardware abstraction layer for Waveshare 13.3" 6-color e-ink display.
 * Supports dual-controller architecture for 1200x1600 resolution with
 * optimized SPI communication and power management.
 * 
 * Compatible with:
 * - Waveshare 13.3" E-Paper HAT (6-color)
 * - ESP32 microcontrollers
 * 
 * @author Stephane Bhiri
 * @version 2.0
 * @date January 2025
 */

#ifndef _EPD_13IN3E_H_
#define _EPD_13IN3E_H_

#include "DEV_Config.h"

// Display specifications
#define EPD_13IN3E_WIDTH    1200
#define EPD_13IN3E_HEIGHT   1600

// Color definitions for 6-color display
#define EPD_13IN3E_BLACK    0x0
#define EPD_13IN3E_WHITE    0x1  
#define EPD_13IN3E_YELLOW   0x2
#define EPD_13IN3E_RED      0x3
#define EPD_13IN3E_BLUE     0x5
#define EPD_13IN3E_GREEN    0x6

// E-Paper command constants
#define PSR               0x00
#define PWR_epd           0x01
#define POF               0x02
#define PON               0x04
#define BTST_N            0x05
#define BTST_P            0x06
#define DTM               0x10
#define DRF               0x12
#define CDI               0x50
#define TCON              0x60
#define TRES              0x61
#define AN_TM             0x74
#define AGID              0x86
#define BUCK_BOOST_VDDN   0xB0
#define TFT_VCOM_POWER    0xB1
#define EN_BUF            0xB6
#define BOOST_VDDP_EN     0xB7
#define CCSET             0xE0
#define PWS               0xE3
#define CMD66             0xF0

// Power management functions
void EPD_13IN3E_PowerOn(void);
void EPD_13IN3E_PowerOff(void);
void EPD_13IN3E_Init(void);
void EPD_13IN3E_Sleep(void);

// Display control functions
void EPD_13IN3E_RefreshNow(void);
void EPD_13IN3E_Clear(UBYTE color);

// Frame buffer functions for dual-controller architecture
void EPD_13IN3E_BeginFrameM(void);    // Master controller (left half)
void EPD_13IN3E_EndFrameM(void);
void EPD_13IN3E_WriteLineM(const UBYTE* line_data);

void EPD_13IN3E_BeginFrameS(void);    // Slave controller (right half)
void EPD_13IN3E_EndFrameS(void);
void EPD_13IN3E_WriteLineS(const UBYTE* line_data);

// Boot splash screen
void EPD_13IN3E_ShowBootSplash(const char* ssid, uint16_t port, int battery_level);

#endif