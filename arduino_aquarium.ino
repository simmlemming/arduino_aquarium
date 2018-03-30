#include <OneWire.h> 
#include <LiquidCrystal_I2C.h>
#include <RTC_DS3231_DST.h>
#include "TimerOne.h"
#include "Button.h"
#include "ObservableFloat.h"
#include "ObservableInt.h"
#include "ObservableDateTime.h"

#define PIN_TEMP 12
#define PIN_LED_NIGHT 10
#define PIN_LED_DAY 9
#define PIN_LED_POMP 8
#define PIN_RELAY_POMP 7
#define PIN_RELAY_LIGHT 6
#define PIN_BUTTON_POMP 5
#define PIN_BUTTON_LIGHT 4

#define DAY_START_HOUR_1 7
#define DAY_START_MINUTE_1 30
#define DAY_END_HOUR_1 14
#define DAY_END_MINUTE_1 0

#define DAY_START_HOUR_2 16
#define DAY_START_MINUTE_2 0
#define DAY_END_HOUR_2 19
#define DAY_END_MINUTE_2 30

#define DAY 1
#define NIGHT 2
#define ON 3
#define OFF 4
#define OFF_TIMER 5

#define POMP_TIMER_MIN 15

OneWire ds(PIN_TEMP);
LiquidCrystal_I2C lcd(0x27,16,2);
RTC_DS3231_DST rtc;

byte first_sensor[8] = {0x28, 0xFF, 0xA8, 0x3F, 0x80, 0x16, 0x5, 0x8C};
byte second_sensor[8] = {0x28, 0xFF, 0x99, 0x67, 0x80, 0x16, 0x5, 0x1};

volatile bool led_pomp = 0;
bool relay_pomp = 0;
int state_pomp = ON;
DateTime timeToTurnOnPomp;
TimeSpan tmpTimeSpan = TimeSpan(0, 0, 0, 0);

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

void onButtonPompClick() {
  if (state_pomp == ON) {
    switchPomp(OFF_TIMER);
  } else if (state_pomp == OFF_TIMER) {
    switchPomp(OFF);
  } else {
    switchPomp(ON);
  }
}

void switchPomp(int new_state) {
  state_pomp = new_state;
  DateTime now = rtc.now();

  if (new_state == ON) {
    Timer1.stop();
    relay_pomp = 1;
    led_pomp = 1;
    showTime(now.hour(), now.minute());
    return;
  }
  
  if (new_state == OFF_TIMER) {
    Timer1.start();
    relay_pomp = 0;
    led_pomp = 0;
    tmpTimeSpan = TimeSpan(0, 0, POMP_TIMER_MIN, 0);
    timeToTurnOnPomp = now + tmpTimeSpan;
    showTime(tmpTimeSpan.minutes(), tmpTimeSpan.seconds());
    return;
  }

  // OFF
  Timer1.stop();
  relay_pomp = 0;
  led_pomp = 0;
  showTime(now.hour(), now.minute());
}

void onMinuteChanged(DateTime time) {
  if (state_pomp != OFF_TIMER) {
    showTime(time.hour(), time.minute());
  }
}

void onSecondChanged(DateTime time) {
  if (state_pomp != OFF_TIMER) {
    return;
  }

  tmpTimeSpan = timeToTurnOnPomp - rtc.now();
  if (tmpTimeSpan.seconds() < 0) {
    switchPomp(ON);
    showTime(time.hour(), time.minute());
  } else {
    showTime(tmpTimeSpan.minutes(), tmpTimeSpan.seconds());
  }
}

ObservableFloat t1 = ObservableFloat(0, onT1Changed);
ObservableFloat t2 = ObservableFloat(0, onT2Changed);
ObservableDateTime time = ObservableDateTime(0, onMinuteChanged, onSecondChanged);

Button buttonLight = Button(PIN_BUTTON_LIGHT, onButtonLightClick);
Button buttonPomp = Button(PIN_BUTTON_POMP, onButtonPompClick);

void toggleLedPomp() {
  led_pomp = !led_pomp;
}

void setup(void) {
//  Serial.begin(9600);
  
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
  
  Timer1.initialize(250000);
  Timer1.attachInterrupt(toggleLedPomp);
  Timer1.stop();

  switchPomp(ON);
  switchToDay();
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

class TemperatureUpdater {
  private:
    unsigned long _delay_ms = 500;
    unsigned long _last_measure_ms = 0;

  public:
    void loop() {
      if (millis() - _last_measure_ms < _delay_ms) {
        return;
      }

      t1.setValue(getTemperature(first_sensor));
      t2.setValue(getTemperature(second_sensor));
  
      _last_measure_ms = millis();
    }
};

TemperatureUpdater temperatureUpdater = TemperatureUpdater();

void loop(void) {
  time.setValue(rtc.now());

  temperatureUpdater.loop();

  // This changes led_* and relay_* states
  timeOfDay.setValue(toTimeOfDay(time.getValue()));
  
  buttonLight.loop();
  buttonPomp.loop();
  
  applyStates();
//  delay(500);
}

int toTimeOfDay(DateTime time) {
  byte hour = time.hour();
  byte minute = time.minute();

  bool isDay1 = between(hour, minute, DAY_START_HOUR_1, DAY_START_MINUTE_1, DAY_END_HOUR_1, DAY_END_MINUTE_1);
  bool isDay2 = between(hour, minute, DAY_START_HOUR_2, DAY_START_MINUTE_2, DAY_END_HOUR_2, DAY_END_MINUTE_2);
  
  if (isDay1 || isDay2) {
    return DAY;
  }
  
  return NIGHT;
}

void applyStates() {
  digitalWrite(PIN_RELAY_LIGHT, relay_light);
  digitalWrite(PIN_LED_DAY, led_day);
  digitalWrite(PIN_LED_NIGHT, led_night);

  digitalWrite(PIN_RELAY_POMP, relay_pomp);
  digitalWrite(PIN_LED_POMP, led_pomp);
}

void showTime(int first, int second) {
  lcd.setCursor(5, 0);
  if (first < 10) {
    lcd.print(0);
  }
  
  lcd.print(first, DEC);
  lcd.print(":");

  if (second < 10) {
    lcd.print(0);
  }
  
  lcd.print(second, DEC);
}

void showTemperature(int position, float value) {
  if (position == 1) {
    lcd.setCursor(0, 1);
  } else {
    lcd.setCursor(11, 1);  
  }
  
  lcd.print(value);
}

bool between(int h, int m, int h1, int m1, int h2, int m2) {
  bool after_start = h > h1 || (h == h1 && m >= m1);
  bool before_end = h < h2 || (h == h2 && m < m2);

  return after_start && before_end;
}

void adjust_rtc_time() {
    // Sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    
    // January 21, 2014 at 3am
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
}

