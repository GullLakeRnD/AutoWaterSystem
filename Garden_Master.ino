 /* 
  NOTE: you must: #include <SoftwareSerial.h>
  BEFORE including the ParallaxLCD.h header
 */

#include <SoftwareSerial.h>
#include <ParallaxLCD.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

ParallaxLCD lcd(2,4,20); // desired pin, rows, cols

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

int lowMoisture = 50;
int highMoisture = 300;
bool Valve = LOW;
int moistureLedColor = 0;
const int valvePin = 4;

int Red = 3;
int Green = 6;
int Blue = 9;
int index = 0;
int ledMode = 1;
int lastMoistureLedColor = 0;
int lastLedMode = 0;
const int WHITE = 0;
const int BLACK = 1;
const int RED = 2;
const int GREEN = 3;
const int BLUE = 4;
const int CYAN = 5;
const int YELLOW = 6;
const int MAGENTA = 7;
const int Party = 0;
const int Working = 1;
const int Off = 2;


//Timing
const int secondInterval = 1000;
const int hundredthInterval = 100;
const int tenthInterval = 10;
long prevMillisParty = 0;
long prevMillisLogAndLCD = 0;
long prevMillisUpdateSens = 0;
long prevMillisCheckMoisture = 0;
long prevMillisRegularLED = 0;

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
  if (millis() - prevMillisUpdateSens >= hundredthInterval) {
   UpdateSensVal();
  }
  if (millis() - prevMillisLogAndLCD >= secondInterval) {
   UpdateLogAndLCD();
  }
  if (millis() - prevMillisCheckMoisture >= hundredthInterval) {
   checkMoisture();
  }
  if (ledMode == Party and millis() - prevMillisParty >= secondInterval) {
   party();
  }
  if (ledMode == Working and moistureLedColor != lastMoistureLedColor) {
  	regularLED();
  }
  if (ledMode == Off and ledMode != lastLedMode) {
  	ledOff();
  }
}

void UpdateLogAndLCD() {

  DateTime now;
   now = RTC.now();
  lcd.empty();
  lcd.pos(0,0);
  lcd.print("Moisture:");
  lcd.print(Moisture);
  lcd.pos(1,0);
  lcd.print("Temp (soil):");
  lcd.print(Soil);
  lcd.pos(2,0);
  lcd.print("Temp (air):");
  lcd.print(Air);
  lcd.pos(3,0);
  lcd.print("Light:");
  lcd.print(Light);
  
  
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
  Entry++;
}

void updateSensVal() {
  Soil = (analogRead(temperature)*5.0 / 1024.0)*75.006 - 42.0;
  Air = (((analogRead(airTemp)*5.0/1024.0)-0.5)*100)*9.0/5.0+36;
  Moisture = analogRead(moisture);
  Light = analogRead(light);
}

void checkMoisture() {
  lastMoistureLedColor = moistureLedColor;
  lastValve = Valve;
  if (Moisture >= lowMoisture and Moisture <= highMoisture) {
  	Valve = LOW;
  	moistureLedColor = GREEN;
  }
  else if (Moisture < lowMoisture) {
  	Valve = HIGH;
  	moistureLedColor = RED;
  }
  else if (Moisture > highMoisture) {
  	Valve = LOW;
  	moistureLedColor = BLUE;
  }
  if (Valve != lastValve) {
  	digitalWrite(valvePin, Valve);
  }
}

void party() {
	int pattern[] = {RED, GREEN, BLUE, CYAN, MAGENTA, YELLOW, WHITE};
	index++;
	if (index >= (sizeof(pattern)/sizeof(int))-1) {
		index = 0;
	}
	int color = pattern[index];
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

void regularLED() {
	switch(moistureLedColor) {
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

void ledOff() {
	digitalWrite(Red, LOW);
	digitalWrite(Green, LOW);
	digitalWrite(Blue, LOW);
}
