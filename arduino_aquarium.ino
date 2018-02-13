#include <MsTimer2.h>
#include <OneWire.h>
#include <TM74HC595Display.h>
#include <TimerOne.h>

int SCLK = 7;
int RCLK = 6;
int DIO = 5;
int DS18x20 = 13;

TM74HC595Display first_disp(SCLK, RCLK, DIO);
TM74HC595Display second_disp(10, 9, 8);

OneWire  ds(DS18x20);  // on pin 10 (a 4.7K resistor is necessary)

byte first_sensor[8] = {0x28, 0xFF, 0xA8, 0x3F, 0x80, 0x16, 0x5, 0x8C};
byte second_sensor[8] = {0x28, 0xFF, 0x99, 0x67, 0x80, 0x16, 0x5, 0x1};
float celsius;  

byte FP[10];
byte N[10];

volatile byte N1[4];
volatile byte N2[4];

void setup(void) {
N[0] = 0xC0; //0
N[1] = 0xF9; //1
N[2] = 0xA4; //2
N[3] = 0xB0; //3
N[4] = 0x99; //4
N[5] = 0x92; //5
N[6] = 0x82; //6
N[7] = 0xF8; //7
N[8] = 0x80; //8
N[9] = 0x90; //9
FP[0] = 0b01000000; //.0
FP[1] = 0b01111001; //.1
FP[2] = 0b00100100; //.2
FP[3] = 0b00110000; //.3
FP[4] = 0b00011001; //.4
FP[5] = 0b00010010; //.5
FP[6] = 0b00000010; //.6
FP[7] = 0b01111000; //.7
FP[8] = 0b00000000; //.8
FP[9] = 0b00010000; //.9

//  Serial.begin(9600);

  Timer1.initialize(1999);
  Timer1.attachInterrupt(update_first_display);

  MsTimer2::set(1, update_second_display);
  MsTimer2::start();
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
  celsius = getTemperature(first_sensor);
  fill_display_data(celsius, N1);
//  Serial.print("t1 = ");
//  Serial.print(celsius);

  celsius = getTemperature(second_sensor);
  fill_display_data(celsius, N2);
//  Serial.print(", t2 = ");
//  Serial.println(celsius);

  delay(500);
}

void fill_display_data(float number, byte data[]) {
  int whole = (int) number;

  data[0] = N[whole/10];
  data[1] = FP[whole % 10];

  int fr = (int) ((number - whole) * 100);
  data[2] = N[fr / 10];
  data[3] = N[fr % 10];
}

void update_first_display() {
  first_disp.send(N1[0], 0b1000);
  first_disp.send(N1[1], 0b100);
  first_disp.send(N1[2], 0b10);
  first_disp.send(N1[3], 0b1);
}

void update_second_display() {
  second_disp.send(N2[0], 0b1000);
  second_disp.send(N2[1], 0b100);
  second_disp.send(N2[2], 0b10);
  second_disp.send(N2[3], 0b1);
}
