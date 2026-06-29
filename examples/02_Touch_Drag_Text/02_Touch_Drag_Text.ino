#include <Arduino.h>
#include <M5GFX.h>
#include <lgfx/v1/panel/Panel_ST7789.hpp>
#include <SPI.h>

// ST7789 Pin mapping
#define TFT_CS    5
#define TFT_RST   3
#define TFT_DC    6
#define TFT_MOSI  14
#define TFT_SCLK  40
#define TFT_MISO  39

// Touch Pin
#define TOUCH_CS  4

// Create a custom M5GFX configuration
class Custom_ST7789 : public lgfx::LGFX_Device
{
  lgfx::v1::Panel_ST7789  _panel_instance;
  lgfx::v1::Bus_SPI       _bus_instance;

public:
  Custom_ST7789(void)
  {
    { // Bus Config
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;
      cfg.spi_3wire  = false;
      cfg.use_lock   = true; // Required to share SPI bus with touch
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = TFT_SCLK;
      cfg.pin_mosi = TFT_MOSI;
      cfg.pin_miso = TFT_MISO;
      cfg.pin_dc   = TFT_DC;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    { // Panel Config
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = TFT_CS;
      cfg.pin_rst          = TFT_RST;
      cfg.pin_busy         = -1;
      cfg.panel_width      = 240;
      cfg.panel_height     = 320;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = true;
      cfg.invert           = true;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = true; // Share the bus
      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance);
  }
};

Custom_ST7789 display;

// Standard SPI bus used by Touch
SPIClass mySPI(FSPI);

int yPosOffset = 0;
int direction = 1;

// Base coordinates for the text
int textX = 120;
int textY = 160;

// Sprite to reduce flickering during drag
LGFX_Sprite sprite(&display);

// Raw SPI read function for XPT2046
uint16_t xpt2046_read_data(uint8_t command) {
  mySPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  digitalWrite(TOUCH_CS, LOW);
  mySPI.transfer(command);
  uint16_t val = mySPI.transfer16(0);
  digitalWrite(TOUCH_CS, HIGH);
  mySPI.endTransaction();
  return val >> 3; // 12-bit ADC value
}

void setup() {
  Serial.begin(115200);
  
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);

  // Initialize shared SPI bus
  mySPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, -1);

  display.init();
  display.setRotation(2); // Portrait, flipped 180 degrees
  display.fillScreen(TFT_BLACK);
  
  // Create a sprite the size of the screen
  sprite.createSprite(display.width(), display.height());
  sprite.setTextDatum(middle_center);
}

void loop() {
  // Read touch pressure (Z)
  uint16_t z1 = xpt2046_read_data(0xB1);
  uint16_t z2 = xpt2046_read_data(0xC1);
  int z = z1 + 4095 - z2;
  
  bool isTouched = (z > 400); // Z threshold
  
  if (isTouched) {
    // Read X and Y
    uint16_t rawX = xpt2046_read_data(0xD1);
    uint16_t rawY = xpt2046_read_data(0x91);

    // Typical XPT2046 calibration mapping
    // You may need to flip the axes if it moves in the opposite direction
    textX = map(rawX, 300, 3800, 0, display.width());
    textY = map(rawY, 300, 3800, display.height(), 0); // Inverted Y for rotation 2
    
    // Constrain to screen bounds
    textX = constrain(textX, 0, display.width());
    textY = constrain(textY, 0, display.height());
  } else {
    // If not touched, bounce vertically
    yPosOffset += direction * 2;
    if (yPosOffset > 15 || yPosOffset < -15) {
      direction *= -1;
    }
  }

  // Draw everything to the sprite first
  sprite.fillScreen(TFT_BLACK);

  int currentYOffset = isTouched ? 0 : yPosOffset;

  // Draw "LukeKitt"
  sprite.setTextColor(display.color565(0, 255, 255)); // Cyan
  sprite.setFont(&fonts::Orbitron_Light_32);
  sprite.setTextSize(1.0);
  sprite.drawString("LukeKitt", textX, (textY - 30) + currentYOffset);

  // Draw "Maker"
  sprite.setTextColor(display.color565(255, 50, 255)); // Magenta
  sprite.setFont(&fonts::Roboto_Thin_24);
  sprite.setTextSize(1.0);
  sprite.drawString("Maker", textX, (textY + 20) + currentYOffset);

  // Cool animated Border
  uint16_t borderColor = display.color565(
    (sin(millis() / 500.0) * 127) + 128, 
    (cos(millis() / 400.0) * 127) + 128, 
    (sin(millis() / 300.0) * 127) + 128
  );
  sprite.drawRect(10, 10, sprite.width() - 20, sprite.height() - 20, borderColor);
  sprite.drawRect(11, 11, sprite.width() - 22, sprite.height() - 22, borderColor);
  
  // Push the sprite to the physical screen
  sprite.pushSprite(0, 0);

  delay(20);
}
