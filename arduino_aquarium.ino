#include <MsTimer2.h>
#include <OneWire.h>
#include <TM74HC595Display.h>
#include <TimerOne.h>
#define OLED_RESET 4

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(OLED_RESET);

int DS18x20 = 13;
OneWire  ds(DS18x20);  // on pin 10 (a 4.7K resistor is necessary)

byte first_sensor[8] = {0x28, 0xFF, 0xA8, 0x3F, 0x80, 0x16, 0x5, 0x8C};
//byte second_sensor[8] = {0x28, 0xFF, 0x99, 0x67, 0x80, 0x16, 0x5, 0x1};
float old_value;  


void setup(void) {
//  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.clearDisplay();
}

float getTemperature(byte addr[]) {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
//  
//  delay(1000);     // maybe 750ms is enough, maybe not
//  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  int16_t raw = (data[1] << 8) | data[0];
  byte cfg = (data[4] & 0x60);
  // at lower res, the low bits are undefined, so let's zero them
  if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
  else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
  else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms

  return (float)raw / 16.0;
}

void loop(void) {
  float new_value = getTemperature(first_sensor);
//  Serial.print("t1 = ");
//  Serial.println(new_value);

  if (new_value != old_value) {
    old_value = new_value;
    update_display(new_value);
  }

//  celsius = getTemperature(second_sensor);
//  fill_display_data(celsius, N2);
//  Serial.print(", t2 = ");
//  Serial.println(celsius);

  delay(500);
}


void update_display(float value) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  display.setTextSize(2);
  String text = "180L, C";
  display.setCursor(128 - textWidth(text, 2), 0);
  display.println("180L, C");
  
//  display.fillRect(0, 0, 12, 16, WHITE);
//  display.println("00123456789012345678901234567890");
//
  text = String(value);
  display.setTextSize(4);
  int margin = (128 - textWidth(text, 4)) / 2 + 2;

  Serial.println(margin);
  Serial.println(textWidth(text, 4));
  display.setCursor(margin, 24);
  display.println(text);

  display.display();
}

int textWidth(String text, int size) {
  return text.length() * 6 * size;
}

