#include <OneWire.h> 
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"
//#include "TimerOne.h"
#include "Button.h"

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

#define DEBOUNCE_MILLIS 30

OneWire ds(PIN_TEMP);
LiquidCrystal_I2C lcd(0x27,16,2);
RTC_DS3231 rtc;

byte first_sensor[8] = {0x28, 0xFF, 0xA8, 0x3F, 0x80, 0x16, 0x5, 0x8C};
byte second_sensor[8] = {0x28, 0xFF, 0x99, 0x67, 0x80, 0x16, 0x5, 0x1};
float first_temperature = 0;
float second_temperature = 0;
DateTime time = DateTime(0);

bool led_pomp = 0;
bool relay_pomp = 0;

bool led_day = 1;
bool led_night = 0;
bool relay_light = 0;

bool led_button_state = HIGH;

Button button_light = Button(PIN_BUTTON_LIGHT);

void setup(void) {
  Serial.begin(9600);
  button_light.onClick(button_light_click);
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

void button_light_click() {
  led_day = !led_day;
  led_night = !led_night;
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
  if (updateTime()) {
    showTime();
  }
  
  if (updateFirstTemperature()) {
    showTemperature(1, first_temperature);
  }

  if (updateSecondTemperature()) {
    showTemperature(2, second_temperature);
  }

  button_light.loop();
  applyStates();
  
//  delay(100);
}

void updateLightState() {
  byte hour = time.hour();
  byte minute = time.minute();

  bool after_day_start = hour > DAY_START_HOUR || (hour == DAY_START_HOUR && minute >= DAY_START_MINUTE);
  bool before_day_end = hour < DAY_END_HOUR || (hour == DAY_END_HOUR && minute < DAY_END_MINUTE);

  if (after_day_start && before_day_end) {
    led_day = 1;
    led_night = 0;
    relay_light = 1;    
  } else {
    led_day = 0;
    led_night = 1;
    relay_light = 0;    
  }
}

void applyStates() {
//  digitalWrite(PIN_RELAY_LIGHT, relay_light);
  digitalWrite(PIN_LED_DAY, led_day);
  digitalWrite(PIN_LED_NIGHT, led_night);

//  digitalWrite(PIN_RELAY_POMP, relay_pomp);
  digitalWrite(PIN_LED_POMP, led_pomp);
}

bool updateFirstTemperature() {
  float t = getTemperature(first_sensor);
  if (first_temperature == t) {
    return false;
  }

  first_temperature = t;
  return true;
}

bool updateSecondTemperature() {
  float t = getTemperature(second_sensor);
  if (second_temperature == t) {
    return false;
  }

  second_temperature = t;
  return true;
}


bool updateTime() {
  DateTime now = rtc.now();
  byte hour = now.hour();
  byte minute = now.minute();

  if (time.minute() == now.minute()) {
    return false;
  }

  time = now;
  return true;
}

void showTime() {
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

