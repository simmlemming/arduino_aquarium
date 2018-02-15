#include <OneWire.h>
//#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"

int DS18x20 = 12;
OneWire  ds(DS18x20);

//byte first_sensor[8] = {0x28, 0xFF, 0xA8, 0x3F, 0x80, 0x16, 0x5, 0x8C};
byte second_sensor[8] = {0x28, 0xFF, 0x99, 0x67, 0x80, 0x16, 0x5, 0x1};
float temperature;  

LiquidCrystal_I2C lcd(0x27,16,2);  // Устанавливаем дисплей
RTC_DS3231 rtc;

void setup(void) {
  Serial.begin(9600);

  lcd.init();                     
  lcd.backlight();// Включаем подсветку дисплея

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  //adjust_rtc_time();
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
  float new_value = getTemperature(second_sensor);
//  Serial.print("t1 = ");
//  Serial.println(new_value);

  if (new_value != temperature) {
    temperature = new_value;
    show_temperature(temperature);
  }

//  celsius = getTemperature(second_sensor);
//  fill_display_data(celsius, N2);
//  Serial.print(", t2 = ");
//  Serial.println(celsius);

  show_time();
  delay(500);
}

void show_time() {
  DateTime now = rtc.now();
  byte hour = now.hour();
  byte minute = now.minute();
  
  lcd.setCursor(11, 0);
  if (hour < 10) {
    lcd.print(0);
  }
  
  lcd.print(hour, DEC);
  lcd.print(":");

  if (minute < 10) {
    lcd.print(0);
  }
  
  lcd.print(minute, DEC);
}

void show_temperature(float value) {
  lcd.setCursor(8, 1);
  lcd.print(value);
}

void adjust_rtc_time() {
    // Sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    
    // January 21, 2014 at 3am
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
}

