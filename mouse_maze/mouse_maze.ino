//Define Variables
// Using Arduino Uno, total of 15 variables as well as SPI (Four pins)
#include <SPI.h> 
#include <SD.h>

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

/* SPI: Following pins are used by the SPI
 SDO - pin 11 
 SDI - pin 12 
 CLK - pin 13
 
 set up variables using the SD utility library functions:
 */
Sd2Card card;
SdVolume volume;
SdFile root;
const int chipSelect = 10;

// SD card file
File dataFile;
int sessionNumber = 1;
int trialNumber = 1;
int userDate = 0;
void writeToSD(int trial, const char* key, const char* value, int userDate, int sessionNumber, unsigned long trialStartTime) {
  char filename[13]; // 8 characters for name + 1 for dot + 3 for extension + 1 for null terminator

  sprintf(filename, "%08d%02d.TXT", userDate, sessionNumber);

  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    unsigned long elapsedTime = millis() - trialStartTime;

    dataFile.print(trial);
    dataFile.print(",");
    dataFile.print("IR beam,");
    dataFile.print(key);
    dataFile.print(",");
    dataFile.print("Timestamp,");
    dataFile.print(elapsedTime);
    dataFile.println();

    dataFile.close();
  } else {
    Serial.println("Error opening data file for writing.");
  }
}

enum IRStates {
  IR1_CHECK,
  IR2_CHECK,
  IR3_CHECK,
  IR4_CHECK,
  TRIAL_INACTIVE,
};

// Declare a variable to store the current IR state
IRStates currentIRState = IR1_CHECK;
 
unsigned long trialStartTime = 0;

void setup() {

  // Set up SPI for SD card
  Serial.begin(9600);
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed. Check your connections.");
    while (1);  // halt if SD card initialization failed
  }

  Serial.println("SD card initialization successful.");

  
  // Set up IR beam pins as inputs
  pinMode(IR1, INPUT);
  pinMode(IR2_Left, INPUT);
  pinMode(IR2_Right, INPUT);
  pinMode(IR3_Left, INPUT);
  pinMode(IR3_Right, INPUT);
  pinMode(IR4_Left, INPUT);
  pinMode(IR4_Right, INPUT);

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

  Serial.println("Enter date (YYYYMMDD): ");
  while (Serial.available() == 0);
  userDate = Serial.parseInt();
}



void loop() {

  // Check current state of the trial
  switch (currentIRState) {
    case IR1_CHECK:
      // Check state of IR1
      if (digitalRead(IR1) == LOW) {
        trialStartTime = millis();
        Serial.println("Trial started.");
        writeToSD(trialNumber, "IR1", "", userDate, sessionNumber, trialStartTime); // Log data for IR1
        // Turn on both lights when IR1 is triggered
        digitalWrite(LED_Left, HIGH);
        digitalWrite(LED_Right, HIGH);
        // Move to the next state
        currentIRState = IR2_CHECK;
      }
      break;

    case IR2_CHECK:
      // Check the state of IR2_Left or IR2_Right
      if (digitalRead(IR2_Left) == LOW) {
        // IR2_Left is triggered, play a tone and unlock the doors using Speaker_Left
        tone(Speaker_Left, 1000, 1000);  // Adjust frequency and duration as needed
        digitalWrite(Doors, HIGH);
        // Log data for IR2_Left
        writeToSD(trialNumber, "IR2_Left", "", userDate, sessionNumber, trialStartTime);
        // Move to the next state
        currentIRState = IR3_CHECK;
      }
      if (digitalRead(IR2_Right) == LOW) {
        // IR2_Right is triggered, play a tone and unlock the doors using Speaker_Right
        tone(Speaker_Right, 1000, 1000);  // Adjust frequency and duration as needed
        digitalWrite(Doors, HIGH);
        // Log data for IR2_Right
        writeToSD(trialNumber, "IR2_Right", "", userDate, sessionNumber, trialStartTime);
        // Move to the next state
        currentIRState = IR3_CHECK;
      }
      break;

    case IR3_CHECK:
      // Check the state of IR3_Left or IR3_Right
      if (digitalRead(IR3_Left) == LOW || digitalRead(IR3_Right) == LOW) {
        // IR3_Left or IR3_Right is triggered, add a timestamp to the SD card
        writeToSD(trialNumber, (digitalRead(IR3_Left) == LOW) ? "IR3_Left" : "IR3_Right", "", userDate, sessionNumber, trialStartTime);
        // Move to the next state
        currentIRState = IR4_CHECK;
      }
      break;

    case IR4_CHECK:
      // Check the state of IR4_Left or IR4_Right
      if (digitalRead(IR4_Left) == LOW || digitalRead(IR4_Right) == LOW) {
        // IR4_Left or IR4_Right is triggered, add a timestamp to the SD card, lock the doors, and end the trial
        writeToSD(trialNumber, (digitalRead(IR4_Left) == LOW) ? "IR4_Left" : "IR4_Right", "", userDate, sessionNumber, trialStartTime);
        // Lock the doors after IR4 is triggered
        digitalWrite(Doors, LOW);
        // End the trial
        // Reset the state for the next trial
        currentIRState = TRIAL_INACTIVE;
      }
      break;

    case TRIAL_INACTIVE:
      // Check the state of IR1 to start a new trial
      if (digitalRead(IR1) == LOW) {
        currentIRState = IR1_CHECK;
      }
      break;


     default: 
      //default is not necessary but left here in case none of the cases match the switch values
      // Handle unexpected state
      break;
  }

  trialNumber++;

  // Check if 100 trials are completed for a session
  if (trialNumber > 100) {
    Serial.println("Session complete. Starting a new session.");
    // Increment the session number
    sessionNumber++;
    // Reset the trial number for the new session
    trialNumber = 1;
  }

}
