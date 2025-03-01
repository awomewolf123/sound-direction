#include <Arduino.h>  // Include the Arduino library for core functionality
#include <LiquidCrystal_I2C.h>  // Include the library for the I2C LCD display
#include "distance.h"  // Include the custom distance measurement class

const int SOUND_IN = 2;  // Define the pin number for the sound sensor

const int NUM_MIC = 4;  // Define the number of microphones used
const int LED_OUT[] = {4, 5, 6, 7};  // Define an array of pin numbers for the LED outputs
const int MIC_IN[] = {8, 9, 10, 11};  // Define an array of pin numbers for the microphone inputs
const long ONE_SECOND = 1000000;  // Define the duration of one second in microseconds

long lastLedOnTime = 0;  // Variable to store the last time an LED was turned on
long lastDistance = 0;  // Variable to store the last measured distance
long lastDisplayTime = 0;  // Variable to store the last time distance was displayed

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Initialize the 16x2 LCD with I2C address 0x27
Distance distance(SOUND_IN);  // Create an instance of the Distance class with the sound sensor pin

// Interrupt Service Routine (ISR) function for handling sound detection
void ISR_sound() {
  distance.ISR_sound();  // Call the ISR function from the Distance class
}

// The setup function runs once when the microcontroller starts
void setup() {
  Serial.begin(115200);  // Initialize serial communication at 115200 baud rate
  Serial.println("setup1");  // Print "setup1" to indicate that setup has started

  for (int pin : LED_OUT)  // Loop through all LED output pins
    pinMode(pin, OUTPUT);  // Set each LED pin as an OUTPUT

  for (int pin : MIC_IN)  // Loop through all microphone input pins
    pinMode(pin, INPUT);  // Set each microphone pin as an INPUT

  pinMode(SOUND_IN, INPUT);  // Set the sound sensor pin as an INPUT
  attachInterrupt(digitalPinToInterrupt(SOUND_IN), ISR_sound, FALLING);  // Attach an interrupt to trigger on FALLING signal

  lcd.init();  // Initialize the LCD
  lcd.backlight();  // Turn on the LCD backlight
  lcd.setCursor(0, 0);  // Move the cursor to the first row, first column
  lcd.print("Calibration...");  // Display "Calibration..." on the LCD
}

// The loop function runs continuously after setup is completed
void loop() {
  long now = micros();  // Get the current time in microseconds

  // Check if the interrupt should be turned back on
  if (distance.interruptTurnOnTime > 0 && now > distance.interruptTurnOnTime) {
    Serial.println("Turn on interrupt");  // Print message indicating interrupt is being re-enabled
    attachInterrupt(digitalPinToInterrupt(SOUND_IN), ISR_sound, FALLING);  // Re-enable the interrupt
    distance.interruptTurnOnTime = 0;  // Reset the interrupt turn-on time
  }

  int turnOnLed = -1;  // Variable to store which LED should be turned on
  for (int i = 0; i < NUM_MIC; i++) {  // Loop through all microphone inputs
    int v = digitalRead(MIC_IN[i]);  // Read the current state of the microphone
    if (v == LOW) {  // Check if the microphone detected sound
      turnOnLed = i;  // Store the index of the microphone that detected sound
      break;  // Exit the loop since we found an active microphone
    }
  }

  // If an LED should be turned on and enough time has passed since last activation
  if (turnOnLed >= 0 && now - lastLedOnTime > 700000) {
    lastLedOnTime = now;  // Update the last LED activation time
    for (int i = 0; i < NUM_MIC; i++) {  // Loop through all LED outputs
      digitalWrite(LED_OUT[i], turnOnLed == i);  // Turn on only the LED corresponding to the active microphone
    }
  }

  // Check if it's time to display distance (every 1 second)
  if (now - lastDisplayTime >= ONE_SECOND) {
    distance.displayDistance();  // Call the function to display distance
    lastDisplayTime = now;  // Update the last display time
  }

  long lDistance = distance.getDistance();  // Get the latest distance measurement
  if (lDistance != lastDistance) {  // Check if the distance has changed
    lastDistance = lDistance;  // Update the stored last distance value
    lcd.setCursor(0, 0);  // Move the cursor to the first row, first column
    
    if (!distance.isCalibrated()) {  // Check if the system is still calibrating
      lcd.print("Calibrating...");  // Display "Calibrating..." on the LCD
      lcd.print(distance.getCalibratingCount());  // Display the calibration count
    } else {  // If calibration is complete
      char buf[16];  // Create a buffer to store the distance as a string
      double dDistance = distance.getDistance() / 100.0;  // Convert distance to centimeters
      dtostrf(dDistance, 6, 2, buf);  // Convert double value to string with 2 decimal places
      lcd.print("Distance (cm):  ");  // Print "Distance (cm):" on the LCD
      lcd.setCursor(0, 1);  // Move cursor to second row
      lcd.print(buf);  // Print the formatted distance value on the LCD
      lcd.setCursor(0, 1);  // Move cursor back to second row (redundant line)
    }
  }
}
