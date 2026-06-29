#include <Arduino.h>
#include <SPI.h>

// ST7789 Pin definitions based on mapping
#define TFT_CS    5   // GPIO 5
#define TFT_RST   3   // GPIO 3
#define TFT_DC    6   // GPIO 6 (BUSY)
#define TFT_MOSI  14  // GPIO 14 (MOSI/SDI)
#define TFT_SCLK  40  // GPIO 40 (SCK)
#define TFT_MISO  39  // GPIO 39 (Not strictly needed for writing to display)

// Use standard SPIClass, can use HSPI or FSPI on ESP32-S3
SPIClass tftSPI(FSPI); 

void sendCommand(uint8_t cmd) {
  digitalWrite(TFT_DC, LOW);
  digitalWrite(TFT_CS, LOW);
  tftSPI.transfer(cmd);
  digitalWrite(TFT_CS, HIGH);
}

void sendData(uint8_t data) {
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_CS, LOW);
  tftSPI.transfer(data);
  digitalWrite(TFT_CS, HIGH);
}

void sendData16(uint16_t data) {
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_CS, LOW);
  tftSPI.transfer16(data);
  digitalWrite(TFT_CS, HIGH);
}

void initST7789() {
  // Hardware reset
  digitalWrite(TFT_RST, HIGH);
  delay(50);
  digitalWrite(TFT_RST, LOW);
  delay(50);
  digitalWrite(TFT_RST, HIGH);
  delay(120);

  sendCommand(0x11); // Sleep Out
  delay(120);

  sendCommand(0x36); // Memory Data Access Control (MADCTL)
  sendData(0x00);

  sendCommand(0x3A); // Color Format
  sendData(0x05);    // 16-bit/pixel (RGB565)

  sendCommand(0x21); // Display Inversion On

  sendCommand(0x2A); // Column Address Set (for 240 width)
  sendData(0x00);
  sendData(0x00);
  sendData(0x00);
  sendData(0xEF); // 240 - 1 = 239 (0xEF)

  sendCommand(0x2B); // Row Address Set (for 320 height)
  sendData(0x00);
  sendData(0x00);
  sendData(0x01);
  sendData(0x3F); // 320 - 1 = 319 (0x013F)

  sendCommand(0x29); // Display On
  delay(120);
}

void fillScreen(uint16_t color) {
  sendCommand(0x2A); // Column Address Set
  sendData(0x00);
  sendData(0x00);
  sendData(0x00);
  sendData(0xEF);

  sendCommand(0x2B); // Row Address Set
  sendData(0x00);
  sendData(0x00);
  sendData(0x01);
  sendData(0x3F);

  sendCommand(0x2C); // Memory Write
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_CS, LOW);

  // Transfer 16-bit color to all pixels (240 * 320 = 76800 pixels)
  for (uint32_t i = 0; i < 76800; i++) {
    tftSPI.transfer16(color);
  }
  
  digitalWrite(TFT_CS, HIGH);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting ST7789 Raw SPI Test...");

  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  pinMode(TFT_DC, OUTPUT);

  // Initialize CS high (deselected)
  digitalWrite(TFT_CS, HIGH);

  // Initialize SPI bus: sck, miso, mosi, ss
  tftSPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, -1);
  // Configure SPI Settings (ST7789 can handle high speeds, 20-40 MHz)
  tftSPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));

  Serial.println("Initializing screen...");
  initST7789();
  Serial.println("Screen initialized.");
}

void loop() {
  Serial.println("Red");
  fillScreen(0xF800); // Red
  delay(1000);
  
  Serial.println("Green");
  fillScreen(0x07E0); // Green
  delay(1000);

  Serial.println("Blue");
  fillScreen(0x001F); // Blue
  delay(1000);
  
  Serial.println("White");
  fillScreen(0xFFFF); // White
  delay(1000);
}
