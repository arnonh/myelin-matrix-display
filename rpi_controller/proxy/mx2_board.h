// gpio configuration for matrix display MX2 board

// ATMEGA328P
// - CE0 is connected to the pin marked /CS, which was intended for the RF24, but is now for RPi-AVR comms
#define PIN_AVR_CE0 8
#define PIN_AVR_RESET 23

// nRF24L01+
#define RF24_SPI_CHANNEL 1
#define PIN_RF24_CE 24
#define PIN_RF24_IRQ 25
// - CE1 is connected to the pin marked D7, and jumpered to /CS on the RF24
#define PIN_RF24_CE1 7
#define PIN_RF24_MISO 9
// - MOSI and SCLK on the RF24 are connected to GPIOs on the RPi, because the SPI port is needed to drive the LEDs
#define PIN_RF24_MOSI 18
#define PIN_RF24_SCK 17

// WS2801
#define LED_SPI_CHANNEL 0
// - SPI pins are connected to the LEDs, not the AVR/RF24
#define PIN_LED_CLK 11
#define PIN_LED_DATA 10