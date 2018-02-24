#include <OneWire.h> 
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"
//#include "TimerOne.h"
#include "Button.h"
#include "ObservableFloat.h"
#include "ObservableInt.h"
#include "ObservableDateTime.h"

#define PIN_TEMP 12
#define PIN_LED_NIGHT 10
#define PIN_LED_DAY 9
#define PIN_LED_POMP 8
#define PIN_RELAY_LIGHT 7
#define PIN_RELAY_POMP 6
#define PIN_BUTTON_POMP 5
#define PIN_BUTTON_LIGHT 4

#define DAY_START_HOUR 7
#define DAY_START_MINUTE 30
#define DAY_END_HOUR 19
#define DAY_END_MINUTE 30

#define DAY 1
#define NIGHT 2

OneWire ds(PIN_TEMP);
LiquidCrystal_I2C lcd(0x27,16,2);
RTC_DS3231 rtc;

byte first_sensor[8] = {0x28, 0xFF, 0xA8, 0x3F, 0x80, 0x16, 0x5, 0x8C};
byte second_sensor[8] = {0x28, 0xFF, 0x99, 0x67, 0x80, 0x16, 0x5, 0x1};


bool led_pomp = 0;
bool relay_pomp = 0;

bool led_day = 0;
bool led_night = 0;
bool relay_light = 0;

void onT1Changed(float old_value, float new_value) {
  showTemperature(1, new_value);
}

void onT2Changed(float old_value, float new_value) {
  showTemperature(2, new_value);
}

void switchToDay() {
    led_day = 1;
    led_night = 0;
    relay_light = 1;
}

void switchToNight() {
    led_day = 0;
    led_night = 1;
    relay_light = 0;
}

void onTimeOfDayChangedByRtc(int old_value, int new_value) {
  if (new_value == DAY) {
    switchToDay();
    lcd.setCursor(15, 0);
    lcd.print("D");
  } else {
    switchToNight();
    lcd.setCursor(15, 0);
    lcd.print("N");
  }
}

ObservableInt timeOfDay = ObservableInt(-1, onTimeOfDayChangedByRtc);

void onButtonLightClick() {
  if (relay_light == 0) {
    switchToDay();
  } else {
    switchToNight();
  }
}

void onTimeChanged(DateTime old_value, DateTime new_value) {
  showTime(new_value);
}

ObservableFloat t1 = ObservableFloat(0, onT1Changed);
ObservableFloat t2 = ObservableFloat(0, onT2Changed);
Button buttonLight = Button(PIN_BUTTON_LIGHT, onButtonLightClick);
ObservableDateTime time = ObservableDateTime(0, onTimeChanged);

void setup(void) {
  Serial.begin(9600);
  
  lcd.init();                     
  lcd.backlight();

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  //adjust_rtc_time();

  pinMode(11, OUTPUT);
  digitalWrite(11, HIGH);

  pinMode(PIN_LED_DAY, OUTPUT);
  pinMode(PIN_LED_NIGHT, OUTPUT);
  pinMode(PIN_LED_POMP, OUTPUT);
  pinMode(PIN_RELAY_LIGHT, OUTPUT);
  pinMode(PIN_RELAY_POMP, OUTPUT);
//  pinMode(PIN_BUTTON_LIGHT, INPUT_PULLUP);
  pinMode(PIN_BUTTON_POMP, INPUT_PULLUP);

  digitalWrite(PIN_RELAY_LIGHT, LOW);
  digitalWrite(PIN_RELAY_POMP, LOW);
//
//  digitalWrite(PIN_LED_NIGHT, LOW);
//  digitalWrite(PIN_LED_POMP, HIGH);

//  Timer1.initialize(500000);
//  Timer1.attachInterrupt(callback);
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
  time.setValue(rtc.now());
  t1.setValue(getTemperature(first_sensor));
  t2.setValue(getTemperature(second_sensor));
  timeOfDay.setValue(toTimeOfDay(time.getValue()));
  
  buttonLight.loop();
  applyStates();
  
//  delay(100);
}

int toTimeOfDay(DateTime time) {
  byte hour = time.hour();
  byte minute = time.minute();

  bool after_day_start = hour > DAY_START_HOUR || (hour == DAY_START_HOUR && minute >= DAY_START_MINUTE);
  bool before_day_end = hour < DAY_END_HOUR || (hour == DAY_END_HOUR && minute < DAY_END_MINUTE);

  if (after_day_start && before_day_end) {
    return DAY;
  }
  
  return NIGHT;
}

void applyStates() {
//  digitalWrite(PIN_RELAY_LIGHT, relay_light);
  digitalWrite(PIN_LED_DAY, led_day);
  digitalWrite(PIN_LED_NIGHT, led_night);

//  digitalWrite(PIN_RELAY_POMP, relay_pomp);
  digitalWrite(PIN_LED_POMP, led_pomp);
}

void showTime(DateTime time) {
  byte hour = time.hour();
  byte minute = time.minute();
  
  lcd.setCursor(5, 0);
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

void showTemperature(int position, float value) {
  if (position == 1) {
    lcd.setCursor(0, 1);
  } else {
    lcd.setCursor(11, 1);  
  }
  
  lcd.print(value);
}

void adjust_rtc_time() {
    // Sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    
    // January 21, 2014 at 3am
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
}

