 /* 
  NOTE: you must: #include <SoftwareSerial.h>
  BEFORE including the ParallaxLCD.h header
 */

#include <SoftwareSerial.h>
#include <ParallaxLCD.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

// Change these values if you're using the 4x20 LCD
#define ROWS 4
#define COLS 20

ParallaxLCD lcd(2,ROWS,COLS); // desired pin, rows, cols

RTC_DS1307 RTC;
const int chipSelect = 10;
File logfile;
const int LOG_INTERVAL = 1000;

/**
 * Initialize the 2-dimensional custom character byte array.
 * Even though each 'byte' is 0-255, only the lowest 5 bytes are used. So only
 *  0-31 are valid values.
 */
byte customCharacters[8][8] = {0, 4, 14, 31, 14, 4, 0, 0,   // Diamond
                               0, 10, 14, 31, 31, 14, 4, 0, // Heart
                               0, 4, 14, 31, 14, 4, 14, 0,  // Spade
                               4, 14, 4, 10, 31, 10, 4, 14, // Club (sorta...)
                               4, 14, 21, 4, 4, 4, 4, 4,    // Up
                               4, 4, 4, 4, 4, 21, 14, 4,    // Down
                               27, 22, 13, 27, 22, 13, 27, 22, // Stipple pattern #1
                               29, 14, 23, 27, 29, 14, 23, 27  // Stipple pattern #2
                             };

int moisture = A0;
int temperature = A1;
int airTemp=A2;
int light = A3;

long Entry = 1;
int Moisture = 0;
int Light = 0;
float Soil = 0.0;
float Air = 0.0;

int minMoisture = 50;
int coolDown = 0;
bool waterOn = false;
long lowCounter = 0;
bool lowWater = false;

int Red = 3;
int Green = 7;
int Blue = 8;
int index = 0;
const int WHITE = 0;
const int BLACK = 1;
const int RED = 2;
const int GREEN = 3;
const int BLUE = 4;
const int CYAN = 5;
const int YELLOW = 6;
const int MAGENTA = 7;

void setup () {
  //Serial.begin(9600);
  lcd.setup();
  delay(100);
  lcd.backLightOn();
  lcd.empty();
  //Serial.println("Starting");
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(Red, OUTPUT);
  pinMode(Green, OUTPUT);
  pinMode(Blue, OUTPUT);

  pinMode(10,OUTPUT);
  if (!SD.begin(chipSelect)) {
    //Serial.println("Card failed, or not present");
  }
  //Serial.println("init pins");
  /**
   * Initialize the custom character slots on the LCD with the predefined values.
   */
   char filename[] = "LOGGER00.CSV";
   for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
     
    
  }
  if (! logfile) {
    //Serial.println("couldnt create file");
  }
  //Serial.println("logfileCreated");
  Wire.begin();
  logfile.println("Entry,Timestamp,Moisture,Soil Temp, Air Temp, Light level,Water on?");
  
}

void loop () {
  //Serial.println("Loop Running");
  DateTime now;
   now = RTC.now();

   updateSensVal();
   UpdateLCD();
   checkMoisture();
   runLed();
   
/*
  if (waterOn==true) {
    digitalWrite(4, HIGH);
  }
  else {
    digitalWrite(4, LOW);
  }
*/

  //
  
  logfile.print(Entry);
  logfile.print(", ");
  logfile.print('"');
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print("/");
  logfile.print(now.year(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print('"');
  logfile.print(", ");
  logfile.print(Moisture);
  logfile.print(", ");
  logfile.print(Soil);
  logfile.print(", ");
  logfile.print(Air);
  logfile.print(", ");
  logfile.print(Light);
  logfile.print(", ");
  if (waterOn == true) {
    logfile.println(1);
  }
  else {
    logfile.println(0);
  }
  
  
  logfile.flush();
  long M = millis();
  delay(1000-(M%1000));
  Entry++;
}

void UpdateLCD() {
  lcd.empty();
  lcd.pos(0,0);
  lcd.print("Moisture:");
  lcd.print(Moisture);
  lcd.pos(1,0);
  lcd.print("Temp (soil):");
  lcd.print(Soil);
  //lcd.print(analogRead(temperature));
  lcd.pos(2,0);
  lcd.print("Temp (air):");
  lcd.print(Air);
  //lcd.print(analogRead(airTemp));
  lcd.pos(3,0);
  lcd.print("Light:");
  lcd.print(Light);
}

void updateSensVal() {
  Soil = (analogRead(temperature)*5.0 / 1024.0)*75.006 - 42.0;
  Air = (((analogRead(airTemp)*5.0/1024.0)-0.5)*100)*9.0/5.0+36;
  Moisture = analogRead(moisture);
  Light = analogRead(light);
}

void checkMoisture() {
  if (waterOn == true) {
    waterOn = false;
  }
  if (coolDown == 0) {
    if (Moisture <= minMoisture) {
      waterOn = true;
      coolDown == 10;
    }
  }
  else {
    coolDown--;
  }
  if (Moisture <= minMoisture) {
    lowCounter++;
  }
  else {
    lowCounter = 0;
  }
  if (lowCounter >= 1000) {
    lowWater = true;
  }
  else {
    lowWater = false;
  }
  if (lowWater == true) {
    //notify with an indicator
  }
}

void runLed() {
	int pattern[] = {RED, GREEN, BLUE, CYAN, MAGENTA, YELLOW, WHITE, BLACK};
	index++;
	int color = pattern[index];
	if (index >= (sizeof(pattern)/sizeof(int))-1) {
		index = 0;
	}
	switch(color) {
		case WHITE:
		digitalWrite(Red, HIGH);
		digitalWrite(Green, HIGH);
		digitalWrite(Blue, HIGH);
		
		break;
		case BLACK:
		digitalWrite(Red, LOW);
		digitalWrite(Green, LOW);
		digitalWrite(Blue, LOW);
		
		break;
		case RED:
		digitalWrite(Red, HIGH);
		digitalWrite(Green, LOW);
		digitalWrite(Blue, LOW);
		
		break;
		case BLUE:
		digitalWrite(Red, LOW);
		digitalWrite(Green, LOW);
		digitalWrite(Blue, HIGH);
		
		break;
		case GREEN:
		digitalWrite(Red, LOW);
		digitalWrite(Green, HIGH);
		digitalWrite(Blue, LOW);
		
		break;
		case YELLOW:
		digitalWrite(Red, LOW);
		digitalWrite(Green, HIGH);
		digitalWrite(Blue, HIGH);
		
		break;
		case CYAN:
		digitalWrite(Red, HIGH);
		digitalWrite(Green, HIGH);
		digitalWrite(Blue, LOW);
		
		break;
		case MAGENTA:
		digitalWrite(Red, HIGH);
		digitalWrite(Green, LOW);
		digitalWrite(Blue, HIGH);
		
		break;
		default:
		digitalWrite(Red, LOW);
		digitalWrite(Green, LOW);
		digitalWrite(Blue, LOW);
		
		break;
	}
}

