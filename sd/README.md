Implmentation of SD/USB interface.

SD card connected to SPI2 interface is exported on USB.

SPI2	Full-Duplex Master	SPI2_MISO	PB14
SPI2	Full-Duplex Master	SPI2_MOSI	PB15
SPI2	Full-Duplex Master	SPI2_SCK	PF9
SD_CS                                           PB8

If you regenerate the code, make sure in Src/main.c to reverse the
initialization order for SPI2 and USB.   Initialization of the USB
interface requires access to the SD card which in turn requires access
to SPI2