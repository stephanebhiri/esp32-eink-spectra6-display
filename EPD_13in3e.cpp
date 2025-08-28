/******************************************************************************
 * EPD 13.3" 6-Color E-Paper Display Driver
 * 
 * Driver for Waveshare 13.3inch e-Paper (K) with 6-color support
 * Resolution: 1200x1600 pixels
 * Colors: Black, White, Yellow, Red, Blue, Green
 * 
 * Copyright (c) 2025 Stephane Bhiri
 ******************************************************************************/

#include "EPD_13in3e.h"
#include "Debug.h"
#include <WiFi.h>
#include "esp_task_wdt.h"

// SPI Configuration Constants
const UBYTE PSR_V[2] = {0xDF, 0x69};
const UBYTE PWR_V[6] = {0x0F, 0x00, 0x28, 0x2C, 0x28, 0x38};
const UBYTE POF_V[1] = {0x00};
const UBYTE DRF_V[1] = {0x00};
const UBYTE CDI_V[1] = {0xF7};
const UBYTE TCON_V[2] = {0x03, 0x03};
const UBYTE TRES_V[4] = {0x04, 0xB0, 0x06, 0x40};  // 1200x1600 (was 1200x800)
const UBYTE CMD66_V[6] = {0x49, 0x55, 0x13, 0x5D, 0x05, 0x10};
const UBYTE EN_BUF_V[1] = {0x07};
const UBYTE CCSET_V[1] = {0x01};
const UBYTE PWS_V[1] = {0x22};
const UBYTE AN_TM_V[9] = {0xC0, 0x1C, 0x1C, 0xCC, 0xCC, 0xCC, 0x15, 0x15, 0x55};
const UBYTE AGID_V[1] = {0x10};
const UBYTE BTST_P_V[2] = {0xE8, 0x28};
const UBYTE BOOST_VDDP_EN_V[1] = {0x01};
const UBYTE BTST_N_V[2] = {0xE8, 0x28};
const UBYTE BUCK_BOOST_VDDN_V[1] = {0x01};
const UBYTE TFT_VCOM_POWER_V[1] = {0x02};

// Ultra-light font table - Only essential characters for boot splash
// 0-9 (10 chars) + A-Z (26 chars) + space, period, colon, dash, percent (5 chars) = 41 chars total
// 41 chars * 8 bytes = 328 bytes (vs 760 bytes for full ASCII)
const uint8_t font_essential[41][8] = {
    // Numbers 0-9
    {0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00}, // '0'
    {0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00}, // '1'
    {0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00}, // '2'
    {0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00}, // '3'
    {0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00}, // '4'
    {0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00}, // '5'
    {0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00}, // '6'
    {0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00}, // '7'
    {0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00}, // '8'
    {0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00}, // '9'
    // Letters A-Z
    {0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00}, // 'A'
    {0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00}, // 'B'
    {0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00}, // 'C'
    {0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00}, // 'D'
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00}, // 'E'
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00}, // 'F'
    {0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00}, // 'G'
    {0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00}, // 'H'
    {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // 'I'
    {0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00}, // 'J'
    {0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00}, // 'K'
    {0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00}, // 'L'
    {0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00}, // 'M'
    {0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00}, // 'N'
    {0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00}, // 'O'
    {0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00}, // 'P'
    {0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00}, // 'Q'
    {0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00}, // 'R'
    {0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00}, // 'S'
    {0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // 'T'
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00}, // 'U'
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // 'V'
    {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // 'W'
    {0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00}, // 'X'
    {0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00}, // 'Y'
    {0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00}, // 'Z'
    // Essential symbols
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ' ' (space)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00}, // '.' (period)
    {0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00}, // ':' (colon)
    {0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00}, // '-' (dash)
    {0x63, 0x63, 0x30, 0x18, 0x0C, 0x33, 0x33, 0x00}  // '%' (percent)
};

// Character mapping helper
static int getEssentialCharIndex(char c) {
    if (c >= '0' && c <= '9') return c - '0';                    // 0-9
    if (c >= 'A' && c <= 'Z') return 10 + (c - 'A');           // A-Z
    if (c >= 'a' && c <= 'z') return 10 + (c - 'a');           // a-z (map to A-Z)
    if (c == ' ') return 36;                                     // space
    if (c == '.') return 37;                                     // period
    if (c == ':') return 38;                                     // colon  
    if (c == '-') return 39;                                     // dash
    if (c == '%') return 40;                                     // percent
    return 36; // Default to space for unsupported characters
}

// Helper functions
static void EPD_13IN3E_CS_ALL(UBYTE Value) {
    DEV_Digital_Write(EPD_CS_M_PIN, Value);
    DEV_Digital_Write(EPD_CS_S_PIN, Value);
}

static void EPD_13IN3E_SPI_Sand(UBYTE Cmd, const UBYTE *buf, UDOUBLE Len) {
    DEV_SPI_WriteByte(Cmd);
    DEV_SPI_Write_nByte((UBYTE *)buf,Len);
}

static void EPD_13IN3E_Reset(void) {
    // Official Waveshare double reset sequence for dual-controller initialization
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(30);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(30);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(30);
    DEV_Digital_Write(EPD_RST_PIN, 0);  // Critical: Second reset cycle
    DEV_Delay_ms(30);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(30);
}

static void EPD_13IN3E_SendCommand(UBYTE Reg) {
    DEV_SPI_WriteByte(Reg);
}

static void EPD_13IN3E_SendData(UBYTE Reg) {
    DEV_SPI_WriteByte(Reg);
}

static void EPD_13IN3E_SendData2(const UBYTE *buf, uint32_t Len) {
    if (!buf || Len == 0) return;
    DEV_SPI_Write_nByte((UBYTE *)buf,Len);
}

static void EPD_13IN3E_ReadBusyH(void) {
    Debug("e-Paper busy\r\n");
    while(!DEV_Digital_Read(EPD_BUSY_PIN)) {
        DEV_Delay_ms(10);
        esp_task_wdt_reset();  // Reset watchdog during long e-ink operations
    }
    DEV_Delay_ms(20);
    Debug("e-Paper busy release\r\n");
}

static void EPD_13IN3E_TurnOnDisplay(void) {
    printf("Write PON \r\n");
    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SendCommand(0x04);
    EPD_13IN3E_CS_ALL(1);
    EPD_13IN3E_ReadBusyH();

    printf("Write DRF \r\n");
    DEV_Delay_ms(50);
    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SPI_Sand(DRF, DRF_V, sizeof(DRF_V));
    EPD_13IN3E_CS_ALL(1);
    EPD_13IN3E_ReadBusyH();
    printf("Write POF \r\n");
    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SPI_Sand(POF, POF_V, sizeof(POF_V));
    EPD_13IN3E_CS_ALL(1);
    // Critical: Official driver does NOT wait for busy after POF - timing sensitive
    printf("Display Done!! \r\n");
}

/******************************************************************************
 * Display Initialization and Control Functions
 ******************************************************************************/
void EPD_13IN3E_Init(void) {
    EPD_13IN3E_Reset();

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SPI_Sand(AN_TM, AN_TM_V, sizeof(AN_TM_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SPI_Sand(CMD66, CMD66_V, sizeof(CMD66_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SPI_Sand(PSR, PSR_V, sizeof(PSR_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SPI_Sand(CDI, CDI_V, sizeof(CDI_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SPI_Sand(TCON, TCON_V, sizeof(TCON_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SPI_Sand(AGID, AGID_V, sizeof(AGID_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SPI_Sand(PWS, PWS_V, sizeof(PWS_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SPI_Sand(CCSET, CCSET_V, sizeof(CCSET_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SPI_Sand(TRES, TRES_V, sizeof(TRES_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SPI_Sand(PWR_epd, PWR_V, sizeof(PWR_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SPI_Sand(EN_BUF, EN_BUF_V, sizeof(EN_BUF_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SPI_Sand(BTST_P, BTST_P_V, sizeof(BTST_P_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SPI_Sand(BOOST_VDDP_EN, BOOST_VDDP_EN_V, sizeof(BOOST_VDDP_EN_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SPI_Sand(BTST_N, BTST_N_V, sizeof(BTST_N_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SPI_Sand(BUCK_BOOST_VDDN, BUCK_BOOST_VDDN_V, sizeof(BUCK_BOOST_VDDN_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SPI_Sand(TFT_VCOM_POWER, TFT_VCOM_POWER_V, sizeof(TFT_VCOM_POWER_V));
    EPD_13IN3E_CS_ALL(1);
}

/******************************************************************************
Clear Screen Function
******************************************************************************/
void EPD_13IN3E_Clear(UBYTE color) {
    UDOUBLE Width, Height;
    UBYTE Color;
    Width = (EPD_13IN3E_WIDTH % 2 == 0)? (EPD_13IN3E_WIDTH / 2 ): (EPD_13IN3E_WIDTH / 2 + 1);
    Height = EPD_13IN3E_HEIGHT;
    Color = (color<<4)|color;

    // Use line buffer for better performance (300 bytes per line)
    // Full buffer would be 480KB which might be too large for ESP32
    const UDOUBLE line_size = Width/2;  // 300 bytes
    UBYTE line_buffer[line_size];
    
    // Fill line buffer with color
    for (UDOUBLE i = 0; i < line_size; i++) {
        line_buffer[i] = Color;
    }

    // Send to Master controller (left half)
    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);
    for (UDOUBLE j = 0; j < Height; j++) {
        EPD_13IN3E_SendData2(line_buffer, line_size);
    }
    EPD_13IN3E_CS_ALL(1);

    // Send to Slave controller (right half)
    DEV_Digital_Write(EPD_CS_S_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);
    for (UDOUBLE j = 0; j < Height; j++) {
        EPD_13IN3E_SendData2(line_buffer, line_size);
    }
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_TurnOnDisplay();
}

/******************************************************************************
 * Boot Splash Display Function
 ******************************************************************************/
void EPD_13IN3E_DisplayTextScreen(const char* ssid, uint16_t port, int battery_pct) {
    Serial.println("*** e-Frame with Color Bands + Text ***");
    
    // Line buffer for rendering
    static const int BYTES_PER_LINE_HALF = EPD_13IN3E_WIDTH / 4; // 300 bytes per half line
    uint8_t line[BYTES_PER_LINE_HALF];
    
    // Get WiFi info for display - convert to uppercase for better font rendering
    char ip_line[64];
    char wifi_line[64];
    char battery_line[64];
    char ssid_upper[32];
    
    if (battery_pct < 0) {
        strcpy(battery_line, "USB POWER");
    } else {
        // Get voltage for display (re-read ADC quickly)
        int raw = analogRead(35);  // Quick read
        float voltage = (raw / 4095.0) * 3.3 * 2.0;
        snprintf(battery_line, sizeof(battery_line), "BATTERY: %.1fV (%d%%)", voltage, battery_pct);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        snprintf(ip_line, sizeof(ip_line), "IP: %s PORT: %d", WiFi.localIP().toString().c_str(), port);
        
        // Convert SSID to uppercase
        strncpy(ssid_upper, ssid, sizeof(ssid_upper) - 1);
        ssid_upper[sizeof(ssid_upper) - 1] = '\0';
        for (int i = 0; ssid_upper[i]; i++) {
            ssid_upper[i] = toupper(ssid_upper[i]);
        }
        snprintf(wifi_line, sizeof(wifi_line), "WIFI: %s", ssid_upper);
    } else {
        strcpy(ip_line, "NO WIFI CONNECTION");
        strcpy(wifi_line, "OFFLINE MODE");
    }
    
    // MAX 30 CHARACTERS (1200px / 40px per char = 30 chars)
    // Each line below is <= 30 chars for perfect fit
    const char* band_texts[] = {
        "E-INK FRAME (C) 2025",                      // Band 0 (black) - 20 chars
        ip_line,                                     // Band 1 (white) - Dynamic, ~25 chars
        wifi_line,                                   // Band 2 (yellow) - Dynamic, ~15 chars
        battery_line,                                // Band 3 (red) - Battery info, ~12 chars
        "13.3 INCH COLOR DISPLAY",                   // Band 4 (blue) - 23 chars
        "READY FOR YOUR IMAGES"                      // Band 5 (green) - 21 chars
    };
    
    // Initialize the display (same as working code)
    EPD_13IN3E_Init();
    
    // Left half (Master)
    EPD_13IN3E_BeginFrameM();
    
    for (int y = 0; y < EPD_13IN3E_HEIGHT; y++) {
        // Determine color band and text for this line
        int band_index = y / 266;  // Which band (0-5)
        if (band_index > 5) band_index = 5;
        
        UBYTE band_color;
        if (band_index == 0) band_color = EPD_13IN3E_BLACK;
        else if (band_index == 1) band_color = EPD_13IN3E_WHITE;
        else if (band_index == 2) band_color = EPD_13IN3E_YELLOW;
        else if (band_index == 3) band_color = EPD_13IN3E_RED;
        else if (band_index == 4) band_color = EPD_13IN3E_BLUE;
        else band_color = EPD_13IN3E_GREEN;
        
        // Start with band background color
        UBYTE packed_color = (band_color << 4) | band_color;
        memset(line, packed_color, BYTES_PER_LINE_HALF);
        
        // Add text in the middle of each band using ultra-light font
        int y_in_band = y % 266;  // Position within the band (0-265)
        if (y_in_band >= 100 && y_in_band < 164) {  // Text zone (64 pixels tall)
            const char* text = band_texts[band_index];
            
            // Choose contrasting text color
            UBYTE text_color;
            if (band_color == EPD_13IN3E_BLACK) text_color = EPD_13IN3E_WHITE;
            else if (band_color == EPD_13IN3E_WHITE) text_color = EPD_13IN3E_BLACK;
            else if (band_color == EPD_13IN3E_YELLOW) text_color = EPD_13IN3E_BLACK;
            else text_color = EPD_13IN3E_WHITE;
            
            // Calculate which font row we're in (4x scaling vertically)
            int scaled_font_y = (y_in_band - 100) / 4;  // 0-15 range
            if (scaled_font_y < 8) {  // Only process first 8 rows (32 pixel height)
                int font_y = scaled_font_y;
                
                // Render text with 4x scaling (left half only)
                int text_x = 20;  // Start position
                for (const char* p = text; *p && text_x < 600; p++) {
                    int char_idx = getEssentialCharIndex(*p);
                    const uint8_t* font_char = font_essential[char_idx];
                    uint8_t line_data = font_char[font_y];
                    
                    // Draw character with 4x scaling horizontally (reversed bit order)
                    for (int bit = 0; bit < 8; bit++) {
                        if (line_data & (0x01 << bit)) {  // Changed from 0x80 >> bit to 0x01 << bit
                            // Draw 4x4 block for each original pixel
                            for (int scale_x = 0; scale_x < 4; scale_x++) {
                                int pixel_x = text_x + (bit * 4) + scale_x;
                                if (pixel_x < 600) {
                                    int byte_idx = pixel_x / 2;
                                    if (byte_idx < BYTES_PER_LINE_HALF) {
                                        if (pixel_x % 2 == 0) {
                                            line[byte_idx] = (line[byte_idx] & 0x0F) | (text_color << 4);
                                        } else {
                                            line[byte_idx] = (line[byte_idx] & 0xF0) | (text_color & 0x0F);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    text_x += 40;  // Character spacing (32 + 8)
                }
            }
        }
        
        EPD_13IN3E_WriteLineM(line);
        
        if ((y % 100) == 0) {
            Serial.printf("M line %d/%d\r", y, EPD_13IN3E_HEIGHT);
        }
    }
    EPD_13IN3E_EndFrameM();
    
    // Right half (Slave) - continue text seamlessly
    EPD_13IN3E_BeginFrameS();
    
    for (int y = 0; y < EPD_13IN3E_HEIGHT; y++) {
        // Same band logic as Master
        int band_index = y / 266;
        if (band_index > 5) band_index = 5;
        
        UBYTE band_color;
        if (band_index == 0) band_color = EPD_13IN3E_BLACK;
        else if (band_index == 1) band_color = EPD_13IN3E_WHITE;
        else if (band_index == 2) band_color = EPD_13IN3E_YELLOW;
        else if (band_index == 3) band_color = EPD_13IN3E_RED;
        else if (band_index == 4) band_color = EPD_13IN3E_BLUE;
        else band_color = EPD_13IN3E_GREEN;
        
        // Start with band background color
        UBYTE packed_color = (band_color << 4) | band_color;
        memset(line, packed_color, BYTES_PER_LINE_HALF);
        
        // Continue text from left half using ultra-light font
        int y_in_band = y % 266;
        if (y_in_band >= 100 && y_in_band < 164) {
            const char* text = band_texts[band_index];
            
            // Same contrasting text color as left
            UBYTE text_color;
            if (band_color == EPD_13IN3E_BLACK) text_color = EPD_13IN3E_WHITE;
            else if (band_color == EPD_13IN3E_WHITE) text_color = EPD_13IN3E_BLACK;
            else if (band_color == EPD_13IN3E_YELLOW) text_color = EPD_13IN3E_BLACK;
            else text_color = EPD_13IN3E_WHITE;
            
            int scaled_font_y = (y_in_band - 100) / 4;
            if (scaled_font_y < 8) {
                int font_y = scaled_font_y;
                
                // Calculate where text would end on left half
                int theoretical_end_x = 20;
                for (const char* p = text; *p; p++) {
                    theoretical_end_x += 40;
                }
                
                // Continue from where left half ended
                int text_x = 0;
                int current_x = 20;
                
                for (const char* p = text; *p && text_x < 600; p++) {
                    // Only render if this character extends into right half
                    if (current_x + 40 > 600) {
                        int char_start_in_right = max(0, 600 - current_x);
                        int char_end_in_right = min(40, 600 + 600 - current_x);
                        
                        int char_idx = getEssentialCharIndex(*p);
                        const uint8_t* font_char = font_essential[char_idx];
                        uint8_t line_data = font_char[font_y];
                        
                        for (int bit = 0; bit < 8; bit++) {
                            if (line_data & (0x01 << bit)) {  // Changed from 0x80 >> bit to 0x01 << bit
                                for (int scale_x = 0; scale_x < 4; scale_x++) {
                                    int pixel_offset = (bit * 4) + scale_x;
                                    
                                    if (pixel_offset >= char_start_in_right && pixel_offset < char_end_in_right) {
                                        int pixel_x = text_x + (pixel_offset - char_start_in_right);
                                        if (pixel_x < 600) {
                                            int byte_idx = pixel_x / 2;
                                            if (byte_idx < BYTES_PER_LINE_HALF) {
                                                if (pixel_x % 2 == 0) {
                                                    line[byte_idx] = (line[byte_idx] & 0x0F) | (text_color << 4);
                                                } else {
                                                    line[byte_idx] = (line[byte_idx] & 0xF0) | (text_color & 0x0F);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        text_x += (char_end_in_right - char_start_in_right);
                    }
                    current_x += 40;
                }
            }
        }
        
        EPD_13IN3E_WriteLineS(line);
        
        if ((y % 100) == 0) {
            Serial.printf("S line %d/%d\r", y, EPD_13IN3E_HEIGHT);
        }
    }
    EPD_13IN3E_EndFrameS();
    
    Serial.println("\nRefreshing display...");
    EPD_13IN3E_RefreshNow();
    
    Serial.println("Boot splash complete");
}


void EPD_13IN3E_ShowBootSplash(const char* ssid, uint16_t port, int battery_pct) {
    EPD_13IN3E_DisplayTextScreen(ssid, port, battery_pct);
}

/******************************************************************************
 * Power Management Functions
 ******************************************************************************/
void EPD_13IN3E_PowerOn(void) {
    #ifdef EPD_PWR_PIN
    DEV_Digital_Write(EPD_PWR_PIN, 1);
    DEV_Delay_ms(100);
    #endif
    
    // SPI already initialized in setup(), don't reinit
    // DEV_Module_Init();  // REMOVED - already done once
}

void EPD_13IN3E_PowerOff(void) {
    // Put display in deep sleep mode before cutting power
    EPD_13IN3E_Sleep();  // RESTORED - needed for low power consumption
    
    // Don't call DEV_Module_Exit() here - it breaks SPI for next use
    // DEV_Module_Exit();  // REMOVED - causes watchdog reset
    
    #ifdef EPD_PWR_PIN
    DEV_Delay_ms(100);  // Allow display to enter sleep
    DEV_Digital_Write(EPD_PWR_PIN, 0);
    #endif
}

void EPD_13IN3E_Sleep(void) {
    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SendCommand(0x07);
    EPD_13IN3E_SendData(0XA5);
    EPD_13IN3E_CS_ALL(1);
    DEV_Delay_ms(100);
}

/******************************************************************************
 * TCP Streaming Functions
 ******************************************************************************/
void EPD_13IN3E_BeginFrameM(void) {
    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);
}

void EPD_13IN3E_WriteLineM(const UBYTE *p300) {
    if (!p300) return;
    // Master handles left half - send all 300 bytes
    EPD_13IN3E_SendData2(p300, EPD_13IN3E_WIDTH/4);
}

void EPD_13IN3E_EndFrameM(void) {
    EPD_13IN3E_CS_ALL(1);
}

void EPD_13IN3E_BeginFrameS(void) {
    // Ensure Master is deselected before selecting Slave
    EPD_13IN3E_CS_ALL(1);  // Deselect all first
    DEV_Digital_Write(EPD_CS_S_PIN, 0);  // Select only Slave
    EPD_13IN3E_SendCommand(0x10);
}

void EPD_13IN3E_WriteLineS(const UBYTE *p300) {
    if (!p300) return;
    // Master handles left half - send all 300 bytes
    EPD_13IN3E_SendData2(p300, EPD_13IN3E_WIDTH/4);
}

void EPD_13IN3E_EndFrameS(void) {
    EPD_13IN3E_CS_ALL(1);
}

void EPD_13IN3E_RefreshNow(void) {
    EPD_13IN3E_TurnOnDisplay();
}

