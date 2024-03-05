//Define Variables
// Using Arduino Uno, total of 15 variables as well as SPI (Four pins)
#include <SPI.h>
#include <SD.h>
#include "Arduino_LED_Matrix.h"
#include <Wire.h>
#include <DS3231.h>

// RTC
DS3231 rtc;

ArduinoLEDMatrix matrix;
// heart
const uint32_t frame_heart[] = {
  0x3184a444,
  0x42081100,
  0xa0040000
};
const uint32_t frame_empty[] = {
  0x00000000,
  0x00000000,
  0x00000000
};
const uint32_t frame_full[] = {
  0xFFFFFFFF,
  0xFFFFFFFF,
  0xFFFFFFFF
};

// IR beam variables
int IR1 = A0;
int IR2_Left = A1;
int IR2_Right = A2;
int IR3_Left = A3;
int IR3_Right = A4;
int IR4_Left = A5;
int IR4_Right = 2;

// Light variables
int LED_Left = 3;
int LED_Right = 4;

// Solenoid variables
// Lickers use I2C
// Made both doors the same pin since we were out of pins
// Transmitters are always powered
int Doors = 7;
int Licker_Left = 8;
int Licker_Right = 9;

// Speaker variables
int Speaker_Left = 5;
int Speaker_Right = 6;

// Define tones
#define A 7000
#define B 10000
#define C 8000
#define D 12000
#define TONE_DURATION 5000

/* SPI: Following pins are used by the SPI
 SDO - pin 11 
 SDI - pin 12 
 CLK - pin 13
 
 set up variables using the SD utility library functions:
 */
const int chipSelect = 10;

// SD card file
// File dataFile;
int sessionNumber = 1;
int trialNumber = 0;
String filename;  // 8 characters for name + 1 for dot + 3 for extension + 1 for null terminator

unsigned long trialStartTime = 0;
unsigned long sessionStartTime = 0;

enum IRStates {
  IR1_CHECK,
  IR2_CHECK,
  IR3_CHECK,
  IR4_CHECK,
  TRIAL_INACTIVE,
};

// Declare a variable to store the current IR state
IRStates currentIRState = IR1_CHECK;

void setup() {
  // Set up IR beam pins as inputs
  pinMode(IR1, INPUT_PULLUP);
  pinMode(IR2_Left, INPUT_PULLUP);
  pinMode(IR2_Right, INPUT_PULLUP);
  pinMode(IR3_Left, INPUT_PULLUP);
  pinMode(IR3_Right, INPUT_PULLUP);
  pinMode(IR4_Left, INPUT_PULLUP);
  pinMode(IR4_Right, INPUT_PULLUP);

  // Set up light pins as outputs
  pinMode(LED_Left, OUTPUT);
  pinMode(LED_Right, OUTPUT);

  // Set up solenoid pins as outputs
  pinMode(Doors, OUTPUT);
  pinMode(Licker_Left, OUTPUT);
  pinMode(Licker_Right, OUTPUT);

  // Set up speaker pins as outputs
  pinMode(Speaker_Left, OUTPUT);
  pinMode(Speaker_Right, OUTPUT);

  // Arduino R4 LED matrix
  matrix.begin();
  matrix.loadFrame(frame_heart);

  Wire.begin();  // I2C RTC
  delay(500);
  rtc.setClockMode(false);  // set to 24h

  // Set up SPI for SD card
  Serial.begin(115200);
  long startTime = millis();
  while ((millis() - startTime) < 5000) {
    if (Serial) {
      Serial.println("\n\n*** Hello, info preference test! ***");
      updateRTC();
      Serial.println("Updated RTC.");
      break;  // Exit the loop if a serial connection is established
    }
  }
  showTime();

  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed. Check your connections.");
    while (1)
      ;  // halt if SD card initialization failed
  }
  Serial.println("SD card initialization successful.");

  matrix.loadFrame(frame_empty);
  Serial.println("Entering task...");
}

void writeToSD(String key, String value, bool createNewFile = false) {
  // Open the file for writing, creating it if necessary
  // If createNewFile is true, truncate the file to zero length (overwrite it)
  File dataFile = SD.open(filename, createNewFile ? (O_WRITE | O_CREAT | O_TRUNC) : (FILE_WRITE));

  int timestamp = millis() - sessionStartTime;
  if (dataFile) {
    dataFile.print(trialNumber);
    dataFile.print(",");
    dataFile.print(key);
    dataFile.print(",");
    dataFile.print(value);
    dataFile.print(",");
    dataFile.print(timestamp);
    dataFile.println();
    dataFile.close();
    Serial.println("Wrote to " + filename);
  } else {
    Serial.println("SD card removed! Replace and restart.");
    while (1) {
      matrix.loadFrame(frame_full);
      delay(500);
      matrix.loadFrame(frame_empty);
      delay(500);
    }
  }
}

void loop() {
  switch (currentIRState) {
    case IR1_CHECK:
      // Check state of IR1
      if (digitalRead(IR1) == LOW) {
        trialNumber++;
        Serial.println("Trial: " + String(trialNumber));
        if (trialNumber == 1) {
          sessionStartTime = millis();
          String currentDate = getDate();
          Serial.println("Current date: " + currentDate);
          filename = currentDate + (sessionNumber < 10 ? "0" : "") + String(sessionNumber) + ".TXT";  // 8 + 4 chars
          writeToSD("date", currentDate, true);                                                       // create file flag
          writeToSD("time", getTime());
        }

        if (trialNumber > 100) {
          currentIRState = TRIAL_INACTIVE;
        }

        trialStartTime = millis();
        writeToSD("IR1", "");  // Log data for IR1
        digitalWrite(LED_Left, HIGH);
        digitalWrite(LED_Right, HIGH);
        Serial.println("IR1 broken");
        currentIRState = IR2_CHECK;
      }
      break;

    case IR2_CHECK:
      if (digitalRead(IR2_Left) == LOW) {
        tone(Speaker_Left, A, TONE_DURATION);  // Adjust frequency and duration as needed
        digitalWrite(Doors, HIGH);
        writeToSD("Tone Frequency", String(A));  // !! update
        writeToSD("Tone Duration", String(TONE_DURATION));
        Serial.println("IR2_Left broken");
        currentIRState = IR3_CHECK;
      }
      if (digitalRead(IR2_Right) == LOW) {
        tone(Speaker_Right, B, TONE_DURATION);  // Adjust frequency and duration as needed
        digitalWrite(Doors, HIGH);
        writeToSD("IR2_Right", "");
        writeToSD("Tone Frequency", String(B));  // !! update
        writeToSD("Tone Duration", String(TONE_DURATION));
        Serial.println("IR2_Right broken");
        currentIRState = IR3_CHECK;
      }
      break;

    case IR3_CHECK:
      if (digitalRead(IR3_Left) == LOW) {
        writeToSD("IR3_Left", "");
        Serial.println("IR3_Left broken");
        currentIRState = IR4_CHECK;
      }
      if (digitalRead(IR3_Right) == LOW) {
        writeToSD("IR3_Left", "");
        Serial.println("IR3_Right broken");
        currentIRState = IR4_CHECK;
      }
      break;

    case IR4_CHECK:
      if (digitalRead(IR4_Left) == LOW) {
        writeToSD("IR4_Left", "");
        Serial.println("IR4_Left broken");
        currentIRState = IR1_CHECK;
      }
      if (digitalRead(IR4_Right) == LOW) {
        writeToSD("IR4_Left", "");
        Serial.println("IR4_Right broken");
        currentIRState = IR1_CHECK;
      }
      break;

    case TRIAL_INACTIVE:
      Serial.println("Session complete. Starting a new session.");
      while (1) {
        // wait for something/button/user input/etc.
        matrix.loadFrame(frame_full);
        delay(500);
        matrix.loadFrame(frame_heart);
        delay(500);
      }
      sessionNumber++;
      trialNumber = 0;  // Reset the trial number for the new session
      currentIRState = IR1_CHECK;
      break;

    default:
      break;
  }
}

void updateRTC() {
  // Get compile date and time
  const char* date = __DATE__;  // "Mmm dd yyyy"
  const char* time = __TIME__;  // "hh:mm:ss"

  // Parse date
  char monthStr[4];
  int day, year;
  sscanf(date, "%s %d %d", monthStr, &day, &year);

  // Convert month to number
  int month = 0;
  const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  for (int i = 0; i < 12; i++) {
    if (strncmp(monthStr, months + i * 3, 3) == 0) {
      month = i + 1;
      break;
    }
  }

  // Parse time
  int hour, minute, second;
  sscanf(time, "%d:%d:%d", &hour, &minute, &second);

  // Set RTC date and time
  rtc.setYear(year - 2000);
  rtc.setMonth(month);
  rtc.setDate(day);
  rtc.setHour(hour);
  rtc.setMinute(minute);
  rtc.setSecond(second);
}

String getTime() {
  bool h12, PM_time;
  byte second = rtc.getSecond();
  byte minute = rtc.getMinute();
  byte hour = rtc.getHour(h12, PM_time);

  // Create a String in the format "HH:MM:SS"
  String timeString = (hour < 10 ? "0" : "") + String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute) + ":" + (second < 10 ? "0" : "") + String(second);
  return timeString;
}

String getDate() {
  bool h12, PM_time, Century;
  byte second = rtc.getSecond();
  byte minute = rtc.getMinute();
  byte hour = rtc.getHour(h12, PM_time);
  byte day = rtc.getDate();
  byte month = rtc.getMonth(Century);
  byte year = rtc.getYear();

  // Create a String in the format "YYMMDD"
  String dateString = String(year) + (month < 10 ? "0" : "") + String(month) + (day < 10 ? "0" : "") + String(day);
  return dateString;
}

void showTime() {
  bool h12, PM_time, Century;
  byte second = rtc.getSecond();
  byte minute = rtc.getMinute();
  byte hour = rtc.getHour(h12, PM_time);
  byte day = rtc.getDate();
  byte month = rtc.getMonth(Century);
  byte year = rtc.getYear();

  Serial.print(year + 2000, DEC);
  Serial.print('/');
  Serial.print(month, DEC);
  Serial.print('/');
  Serial.print(day, DEC);
  Serial.print(" ");
  Serial.print(hour < 10 ? "0" : "");  // Add a leading zero if the hour is less than 10
  Serial.print(hour, DEC);
  Serial.print(':');
  Serial.print(minute < 10 ? "0" : "");  // Add a leading zero if the minute is less than 10
  Serial.print(minute, DEC);
  Serial.print(':');
  Serial.print(second < 10 ? "0" : "");  // Add a leading zero if the second is less than 10
  Serial.print(second, DEC);
  Serial.println();
}
