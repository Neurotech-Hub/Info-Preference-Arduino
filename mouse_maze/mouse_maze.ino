// A = "Left" side, B = "Right" side
//Define Variables
// Using Arduino Uno, total of 15 variables as well as SPI (Four pins)
//add tone freq in statement
#include <SPI.h>
#include <SD.h>
#include "Arduino_LED_Matrix.h"
#include <Wire.h>
#include <DS3231.h>

#define MAX_TRIALS_IN_SESSION 2
const char* currentCompileID = __DATE__ " " __TIME__;  // Unique ID for each compilation
const char* compilationFilename = "COMPILE.TXT";

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
int IR2_A = A1;
int IR2_B = A2;
int IR3_A = A3;
int IR3_B = A4;
int IR4_A = A5;
int IR4_B = 2;

// Light variables
int LED_A = 3;
int LED_B = 4;

// Solenoid variables
// Lickers use I2C
// Made both doors the same pin since we were out of pins
// Transmitters are always powered
int Doors = 7;
int Licker_A = 8;
int Licker_B = 9;

// Speaker variables
int Speaker_A = 5;
int Speaker_B = 6;

bool finishIR3;  // special case for tone/IR3 logging

// Define tones
#define A 7000
#define B 10000
#define C 8000
#define D 12000
#define TONE_DURATION 5000
unsigned long toneStartTime = 0;

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
  pinMode(IR2_A, INPUT_PULLUP);
  pinMode(IR2_B, INPUT_PULLUP);
  pinMode(IR3_A, INPUT_PULLUP);
  pinMode(IR3_B, INPUT_PULLUP);
  pinMode(IR4_A, INPUT_PULLUP);
  pinMode(IR4_B, INPUT_PULLUP);

  // Set up light pins as outputs
  pinMode(LED_A, OUTPUT);
  pinMode(LED_B, OUTPUT);

  // Set up solenoid pins as outputs
  pinMode(Doors, OUTPUT);
  pinMode(Licker_A, OUTPUT);
  pinMode(Licker_B, OUTPUT);

  // Set up speaker pins as outputs
  pinMode(Speaker_A, OUTPUT);
  pinMode(Speaker_B, OUTPUT);

  // Arduino R4 LED matrix
  matrix.begin();
  matrix.loadFrame(frame_heart);

  // Set up SPI for SD card
  Serial.begin(115200);
  delay(2000);  // startup delay
  long startTime = millis();
  while (millis() - startTime < 5000) {
    if (Serial) {
      Serial.println("\n\n*** Hello, info preference test! ***");
      break;  // Exit the loop if a serial connection is established
    }
  }

  // Serial.println("looping sensors...");
  // while(1) {
  //   Serial.print("IR1: ");
  //   Serial.println(digitalRead(A0));
  //   Serial.print("IR2A: ");
  //   Serial.println(digitalRead(A1));
  //   Serial.print("IR2B: ");
  //   Serial.println(digitalRead(A2));
  //   Serial.print("IR3A: ");
  //   Serial.println(digitalRead(A3));
  //   Serial.print("IR3B: ");
  //   Serial.println(digitalRead(A4));
  //   Serial.print("IR4A: ");
  //   Serial.println(digitalRead(A5));
  //   Serial.print("IR4B: ");
  //   Serial.println(digitalRead(2));
  //   Serial.println("");
  //   delay(500);
  // }

  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed. Check your connections.");
    while (1)
      ;  // halt if SD card initialization failed
  }
  Serial.println("SD card initialization successful.");

  Wire.begin();  // I2C RTC
  // Wire.setClock(25000); // slow it down
  delay(500);
  // if(checkAndUpdateCompileID()) {
    // updateRTC();
    // rtc.setClockMode(false);  // set to 24h
  // }
  showTime();

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
    // Print statement for debugging in Serial monitor
    showTime(); // !! DEBUGGING ONLY
    Serial.print("Saved ");
    Serial.print(filename);
    Serial.print(": ");
    Serial.print(key);
    Serial.print(",");
    Serial.print(value);
    Serial.print(",");
    Serial.println(timestamp);
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
        Serial.println("Trial: " + String(trialNumber) + ", Session: " + String(sessionNumber));
        if (trialNumber == 1) {
          sessionStartTime = millis();
          String currentDate = getDate();
          Serial.println("Current date: " + currentDate);
          filename = currentDate + (sessionNumber < 10 ? "0" : "") + String(sessionNumber) + ".TXT";  // 8 + 4 chars
          writeToSD("date", currentDate, true);                                                       // create file flag
          writeToSD("time", getTime());
        }

        if (trialNumber > MAX_TRIALS_IN_SESSION) {
          currentIRState = TRIAL_INACTIVE;
        } else {

          trialStartTime = millis();
          writeToSD("IR1", "");  // Log data for IR1
          digitalWrite(LED_A, HIGH);
          digitalWrite(LED_B, HIGH);
          currentIRState = IR2_CHECK;
        }
      }
      break;

    case IR2_CHECK:
      if (digitalRead(IR2_A) == LOW) {
        writeToSD("IR2_A", "");
        int randomNumber = random(4);  // random int 0-3
        if (randomNumber == 3) {
          tone(Speaker_A, A, TONE_DURATION);  // Play tone A
          writeToSD("Tone_A", String(A));
        } else {
          tone(Speaker_A, B, TONE_DURATION);  // Play tone B
          writeToSD("Tone_A", String(B));
        }
        unsigned long toneStartTime = millis();
        while (millis() - toneStartTime < TONE_DURATION) {
          // Allow the tone to play for TONE_DURATION (5 seconds)
        }
        if (randomNumber == 3 && millis() - toneStartTime >= TONE_DURATION) {
          pinMode(Licker_A, HIGH);  // Activate Licker_A after 5 seconds of tone A
          writeToSD("Licker_A", "");
        }
        currentIRState = IR3_CHECK;
      }
      if (digitalRead(IR2_B) == LOW) {
        writeToSD("IR2_B", "");  // Log data for IR2_B
        // Generate a random number between 0 and 1
        int randomNumber = random(2);
        // Choose tone based on the random number
        if (randomNumber == 1) {
          tone(Speaker_B, C, TONE_DURATION);  // Play tone C
          writeToSD("Tone_A", String(C));
        } else {
          tone(Speaker_B, D, TONE_DURATION);  // Play tone D
          writeToSD("Tone_A", String(D));
        }
        currentIRState = IR3_CHECK;
      }
      // exiting
      if (currentIRState == IR3_CHECK) {
        toneStartTime = millis();
        finishIR3 = false;  // reset
      }
      break;

    case IR3_CHECK:
      // time between IR2-3 is critical, no blocking
      // !finishIR3 && ensures IR3 is only logged once
      if (!finishIR3 && digitalRead(IR3_A) == LOW) {
        writeToSD("IR3_A", "");
        finishIR3 = true;
      }
      if (!finishIR3 && digitalRead(IR3_B) == LOW) {
        writeToSD("IR3_B", "");
        finishIR3 = true;
      }

      // if IR was triggered AND tone has played thru
      if (finishIR3 && (millis() - toneStartTime >= TONE_DURATION)) {
        int randomNumber2 = random(4);
        // Deliver reward based on random number
        if (randomNumber2 == 3) {
          pinMode(Licker_B, HIGH);  // Activate Licker_B if the random number is 3
          writeToSD("Licker_B", "");
        }
        digitalWrite(Doors, HIGH);
        writeToSD("Doors Open", "");
        currentIRState = IR4_CHECK;  // progress to next state here
      }
      break;

    case IR4_CHECK:
      if (digitalRead(IR4_A) == LOW) {
        writeToSD("IR4_A", "");
        currentIRState = IR1_CHECK;
      }
      if (digitalRead(IR4_B) == LOW) {
        writeToSD("IR4_B", "");
        currentIRState = IR1_CHECK;
      }
      break;

    case TRIAL_INACTIVE:
      Serial.println("Session complete. Starting a new session.");
      matrix.loadFrame(frame_full);
      delay(500);
      matrix.loadFrame(frame_heart);
      delay(500);
      /*while (1) {
        // wait for something/button/user input/etc.
        matrix.loadFrame(frame_full);
        delay(500);
        matrix.loadFrame(frame_heart);
        delay(500);
      }*/
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
  return "TEST";

  bool h12, PM_time;
  byte second, minute, hour;
  unsigned long startMillis = millis();
  unsigned long timeout = 5000;  // 5 seconds timeout

  // Try to get valid time within timeout
  do {
    second = rtc.getSecond();
    minute = rtc.getMinute();
    hour = rtc.getHour(h12, PM_time);
  } while ((hour > 23 || minute > 59 || second > 59) && (millis() - startMillis < timeout));

  // Check if valid time was obtained
  if (hour > 23 || minute > 59 || second > 59) {
    return "INVALID";
  }

  // Create a String in the format "HH:MM:SS"
  String timeString = (hour < 10 ? "0" : "") + String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute) + ":" + (second < 10 ? "0" : "") + String(second);
  return timeString;
}

String getDate() {
  return "TEST";

  bool h12, PM_time, Century;
  byte year = 255, month = 255, day = 255;
  unsigned long startMillis = millis();
  unsigned long timeout = 5000;  // 5 seconds timeout

  // Try to mitigate errors with I2C and validate date within timeout
  while ((year > 28 || year < 24 || month > 12 || month < 1 || day > 31 || day < 1) && (millis() - startMillis < timeout)) {
    byte second = rtc.getSecond();
    byte minute = rtc.getMinute();
    byte hour = rtc.getHour(h12, PM_time);
    day = rtc.getDate();
    month = rtc.getMonth(Century);
    year = rtc.getYear();
  }

  // Check if valid date was obtained
  if (year > 28 || year < 24 || month > 12 || month < 1 || day > 31 || day < 1) {
    return "INVALID";
  }

  // Create a String in the format "YYMMDD"
  String dateString = String(year) + (month < 10 ? "0" : "") + String(month) + (day < 10 ? "0" : "") + String(day);
  return dateString;
}

void showTime() {
  bool h12, PM_time, Century;
  byte second, minute, hour, day, month, year;
  unsigned long startMillis = millis();
  unsigned long timeout = 5000;  // 5 seconds timeout

  Serial.println("DATETIME OFFLINE");
  return;

  // Try to get valid date and time within timeout
  do {
    second = rtc.getSecond();
    minute = rtc.getMinute();
    hour = rtc.getHour(h12, PM_time);
    day = rtc.getDate();
    month = rtc.getMonth(Century);
    year = rtc.getYear();
  } while ((year > 28 || year < 24 || month > 12 || month < 1 || day > 31 || day < 1 || hour > 23 || minute > 59 || second > 59) && (millis() - startMillis < timeout));

  // Check if valid date and time were obtained
  if (year > 28 || year < 24 || month > 12 || month < 1 || day > 31 || day < 1 || hour > 23 || minute > 59 || second > 59) {
    Serial.println("Invalid Datetime");
    return;
  }

  // Display the date and time in the format "YYYY/MM/DD HH:MM:SS"
  Serial.print(year + 2000, DEC);
  Serial.print('/');
  Serial.print(month < 10 ? "0" : "");
  Serial.print(month, DEC);
  Serial.print('/');
  Serial.print(day < 10 ? "0" : "");
  Serial.print(day, DEC);
  Serial.print(" ");
  Serial.print(hour < 10 ? "0" : "");
  Serial.print(hour, DEC);
  Serial.print(':');
  Serial.print(minute < 10 ? "0" : "");
  Serial.print(minute, DEC);
  Serial.print(':');
  Serial.print(second < 10 ? "0" : "");
  Serial.print(second, DEC);
  Serial.println();
}

bool checkAndUpdateCompileID() {
  bool updateTime = false;
  // Read the stored compile ID from the file
  String storedCompileID;
  File file = SD.open(compilationFilename, FILE_READ);
  if (file) {
    storedCompileID = file.readStringUntil('\n');
    storedCompileID.trim(); // end char
    Serial.print("Stored compilation: ");
    Serial.println(storedCompileID);
    file.close();
  }

  // compareStringsByteByByte(storedCompileID, currentCompileID);

  // Compare the stored ID with the current compile ID
  if (storedCompileID != currentCompileID) {
    // IDs don't match, so it's a new compilation
    Serial.print("New compilation (saving): ");
    Serial.println(currentCompileID);
    updateTime = true;

    // Update the file with the current compile ID
    file = SD.open(compilationFilename, O_WRITE | O_CREAT | O_TRUNC);
    if (file) {
      file.println(currentCompileID);
      file.close();
    }
  } else {
    Serial.println("No new compilation, file not updated.");
  }
}

void compareStringsByteByByte(const String& storedCompileID, const char* currentCompileID) {
  Serial.println("Comparing strings byte by byte:");
  int i = 0;
  bool stringsMatch = true;
  while (currentCompileID[i] != '\0' || i < storedCompileID.length()) {
    char storedChar = i < storedCompileID.length() ? storedCompileID[i] : '\0';
    char currentChar = currentCompileID[i];
    Serial.print("Byte ");
    Serial.print(i);
    Serial.print(": Stored = ");
    Serial.print((int)storedChar);
    Serial.print(" (");
    Serial.print(storedChar);
    Serial.print("), Current = ");
    Serial.print((int)currentChar);
    Serial.print(" (");
    Serial.print(currentChar);
    Serial.println(")");
    if (storedChar != currentChar) {
      stringsMatch = false;
      Serial.println(" -> Mismatch detected!");
    }
    i++;
  }
  if (stringsMatch) {
    Serial.println("Strings match!");
  } else {
    Serial.println("Strings do not match.");
  }
}