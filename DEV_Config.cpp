#include "DEV_Config.h"

int DEV_Module_Init(void)
{
  // GPIO
  pinMode(EPD_CS_M_PIN, OUTPUT);  DEV_Digital_Write(EPD_CS_M_PIN, HIGH);
  pinMode(EPD_CS_S_PIN, OUTPUT);  DEV_Digital_Write(EPD_CS_S_PIN, HIGH);
  pinMode(EPD_DC_PIN,    OUTPUT); DEV_Digital_Write(EPD_DC_PIN,    HIGH);
  pinMode(EPD_RST_PIN,   OUTPUT); DEV_Digital_Write(EPD_RST_PIN,   HIGH);
  pinMode(EPD_BUSY_PIN,  INPUT);

  #ifdef EPD_PWR_PIN
    pinMode(EPD_PWR_PIN, OUTPUT);
    DEV_Digital_Write(EPD_PWR_PIN, HIGH); // HAT rev 2.3
  #endif

  // SPI matériel (VSPI)
  #ifdef EPD_MISO_PIN
    SPI.begin(EPD_SCK_PIN, EPD_MISO_PIN, EPD_MOSI_PIN);
  #else
    SPI.begin(EPD_SCK_PIN, -1,            EPD_MOSI_PIN);
  #endif
  SPI.beginTransaction(SPISettings(SPI_SPEED_HZ, MSBFIRST, SPI_MODE0));
  return 0;
}

void DEV_Module_Exit(void)
{
  SPI.endTransaction();
  SPI.end();
  DEV_Digital_Write(EPD_CS_M_PIN, HIGH);
  DEV_Digital_Write(EPD_CS_S_PIN, HIGH);
}

// ========== SPI low-level utilisés par le driver ==========
void DEV_SPI_WriteByte(UBYTE data)
{
  SPI.transfer(data);
}

void DEV_SPI_Write_nByte(UBYTE *data, UDOUBLE len)
{
  while (len--) {
    SPI.transfer(*data++);
  }
}