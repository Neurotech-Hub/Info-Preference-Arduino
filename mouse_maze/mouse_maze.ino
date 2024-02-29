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

// Function for IR beam being triggered 
volatile boolean trialInProgress = false;
unsigned long trialStartTime = 0;

// IR1 function
void IRBreak() {
  if (!trialInProgress) {
    trialInProgress = true;
    trialStartTime = millis();
    Serial.println("Trial started.");
    // Turn on both lights when IR1 is triggered
    digitalWrite(LED_Left, HIGH);
    digitalWrite(LED_Right, HIGH);
  }
}

// IR2 function
void IR2Break() {
  if (trialInProgress) {
    // Play a tone
    tone(Speaker_Left, 1000, 1000);  // Adjust frequency and duration as needed
    // Unlock the doors
    digitalWrite(Doors, HIGH);
  }
}

// IR3 function
// Later adjust how timestamp is set
void IR3Break() {
  if (trialInProgress) {
    // Add a timestamp to the SD card
    dataFile = SD.open("trialData.txt", FILE_WRITE);
    if (dataFile) {
      dataFile.print("Timestamp IR3: ");
      dataFile.print(millis() - trialStartTime); // convert milliseconds to seconds since the trial started
      dataFile.println(" ms");
      dataFile.close();
    } else {
      Serial.println("Error opening file for writing.");
    }
  }
}

void IR4Break() {
  if (trialInProgress) {
    // Add a timestamp to the SD card
    dataFile = SD.open("trialData.txt", FILE_WRITE);
    if (dataFile) {
      dataFile.print("Timestamp IR4: ");
      dataFile.print(millis() - trialStartTime); // convert milliseconds to seconds since the trial started
      dataFile.println(" ms");
      dataFile.close();
    } else {
      Serial.println("Error opening file for writing.");
    }
    // Lock the doors after IR4 is triggered
    digitalWrite(Doors, LOW);
    // End the trial
    trialInProgress = false;
  }
}

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

  // Attach interrupt to IR1
  // If IR1 goes from high to low then the interupt is triggered causing the trial to begin (same for the following IR beams!)
  attachInterrupt(digitalPinToInterrupt(IR1), IRBreak, FALLING);

  // Attach interrupts to IR2_Left and IR2_Right
  attachInterrupt(digitalPinToInterrupt(IR2_Left), IR2Break, FALLING);
  attachInterrupt(digitalPinToInterrupt(IR2_Right), IR2Break, FALLING);

  // Attach interrupts to IR3_Left and IR3_Right
  attachInterrupt(digitalPinToInterrupt(IR3_Left), IR3Break, FALLING);
  attachInterrupt(digitalPinToInterrupt(IR3_Right), IR3Break, FALLING);

  // Attach interrupts to IR4_Left and IR4_Right
  attachInterrupt(digitalPinToInterrupt(IR4_Left), IR4Break, FALLING);
  attachInterrupt(digitalPinToInterrupt(IR4_Right), IR4Break, FALLING);
}



void loop() {

  if (trialInProgress) {
    unsigned long elapsedTime = millis() - trialStartTime;

    // Print elapsed time to Serial Monitor
    Serial.print("Elapsed Time: ");
    Serial.print(elapsedTime / 1000); // convert milliseconds to seconds
    Serial.println(" seconds");

    // Save elapsed time to SD card
    dataFile = SD.open("trialData.txt", FILE_WRITE);
    if (dataFile) {
      dataFile.print("Elapsed Time: ");
      dataFile.print(elapsedTime / 1000); // convert milliseconds to seconds
      dataFile.println(" seconds");
      dataFile.close();
    } else {
      Serial.println("Error opening file for writing.");
    }

    trialInProgress = false; // Reset for the next trial
    delay(5000); // Wait for 5 seconds before starting the next trial (adjust as needed)

    //Turn off LED once trial is completed
    digitalWrite(LED_Left, LOW);
    digitalWrite(LED_Right, LOW);

    // Lock the doors after the trial is completed
    digitalWrite(Doors, LOW);

    delay(5000); // Wait for 5 seconds before starting the next trial (adjust as needed)
  }
}
