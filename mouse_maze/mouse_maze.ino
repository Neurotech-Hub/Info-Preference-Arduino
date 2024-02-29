//Define Variables
// Using Arduino Uno, total of 15 variables as well as SPI (Four pins)
#include <SPI.h> 
#include <SD.h>

// IR beam variables
// Seven IR beams
int IR1 = A0;   
int IR2_Left = A1;   
int IR2_Right = A2;   
int IR3_Left = A3;   
int IR3_Right = A4; 
int IR4_Left = A5;
int IR4_Right = 2;

// Light variables
// Two LED lights
int LED_Left = 3;
int LED_Right = 4;

// Solenoid variables
// Two doors and two lickers
// Lickers use I2C
// Made both doors the same pin since we were out of pins
// transmitters are always powered
int Doors = 7;
int Licker_Left = 8;
int Licker_Right = 9;

// Two speakers
// Speakers just need to play a tone
int Speaker_Left = 5;
int Speaker_Right = 6;

/* SPI
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

enum IRStates {
  IR1_TRIGGERED,
  IR2_TRIGGERED,
  IR3_TRIGGERED,
  IR4_TRIGGERED,
  TRIAL_INACTIVE,
  NUM_IR_STATES  // Optional: To keep track of the number of states
};

// Declare a variable to store the current IR state
IRStates currentIRState = IR1_TRIGGERED;

// Function for IR beam being triggered 
volatile boolean trialInProgress = false;
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
}



void loop() {
  // Check the current state of the trial
  switch (currentIRState) {
    case IR1_TRIGGERED:
      // IR1 is triggered, start the trial
      trialInProgress = true;
      trialStartTime = millis();
      Serial.println("Trial started.");
        dataFile = SD.open("trialData.txt", FILE_WRITE);
        if (dataFile) {
          dataFile.print("Timestamp IR1: ");
          dataFile.print(millis() - trialStartTime);
          dataFile.println(" ms");
          dataFile.close();
        } else {
          Serial.println("Error opening file for writing.");
        }

      // Turn on both lights when IR1 is triggered
      digitalWrite(LED_Left, HIGH);
      digitalWrite(LED_Right, HIGH);

      // Move to the next state
      currentIRState = IR2_TRIGGERED;
      break;

    case IR2_TRIGGERED:
      // Check the state of IR2_Left or IR2_Right
      if ((digitalRead(IR2_Left) == LOW) && trialInProgress) {
       // IR2_Left is triggered, play a tone and unlock the doors using Speaker_Left
      tone(Speaker_Left, 1000, 1000);  // Adjust frequency and duration as needed
      digitalWrite(Doors, HIGH);

      // Move to the next state
      currentIRState = IR3_TRIGGERED;
      }
      else if ((digitalRead(IR2_Right) == LOW) && trialInProgress) {
      // IR2_Right is triggered, play a tone and unlock the doors using Speaker_Right
      tone(Speaker_Right, 1000, 1000);  // Adjust frequency and duration as needed
      digitalWrite(Doors, HIGH);

      // Move to the next state
      currentIRState = IR3_TRIGGERED;
      }
      break;

    case IR3_TRIGGERED:
      // Check the state of IR3_Left or IR3_Right
      if ((digitalRead(IR3_Left) == LOW || digitalRead(IR3_Right) == LOW) && trialInProgress) {
        // IR3_Left or IR3_Right is triggered, add a timestamp to the SD card
        dataFile = SD.open("trialData.txt", FILE_WRITE);
        if (dataFile) {
          dataFile.print("Timestamp IR3: ");
          dataFile.print(millis() - trialStartTime);
          dataFile.println(" ms");
          dataFile.close();
        } else {
          Serial.println("Error opening file for writing.");
        }

        // Move to the next state
        currentIRState = IR4_TRIGGERED;
      }
      break;

    case IR4_TRIGGERED:
      // Check the state of IR4_Left or IR4_Right
      if ((digitalRead(IR4_Left) == LOW || digitalRead(IR4_Right) == LOW) && trialInProgress) {
        // IR4_Left or IR4_Right is triggered, add a timestamp to the SD card, lock the doors, and end the trial
        dataFile = SD.open("trialData.txt", FILE_WRITE);
        if (dataFile) {
          dataFile.print("Timestamp IR4: ");
          dataFile.print(millis() - trialStartTime);
          dataFile.println(" ms");
          dataFile.close();
        } else {
          Serial.println("Error opening file for writing.");
        }
        // Lock the doors after IR4 is triggered
        digitalWrite(Doors, LOW);
        // End the trial
        trialInProgress = false;
        // Reset the state for the next trial
        currentIRState = TRIAL_INACTIVE;
      }
      break;

    case TRIAL_INACTIVE:
      // Check the state of IR1 to start a new trial
      if (digitalRead(IR1) == LOW && !trialInProgress) {
        currentIRState = IR1_TRIGGERED;
      }
      break;

    default:
      // Handle unexpected state
      break;
  }

}
