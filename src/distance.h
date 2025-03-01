#include <Arduino.h>  // Include the Arduino library for basic functionality

const double SOUND_SPEED = 340.4;  // Define the speed of sound in meters per second

// Define the Distance class to handle sound-based distance measurement
class Distance {
  unsigned int soundPin = 0;  // Variable to store the pin number for the sound sensor
  volatile unsigned long preTriggerTime = 0;  // Stores the time of the last detected sound event before triggering
  volatile uint64_t initialTriggerTime = 0;  // Stores the first valid trigger time after calibration

  volatile unsigned long nPulseInterval = 0;  // Stores the accumulated pulse interval duration
  volatile unsigned long nPulseIntervalCount = 0;  // Stores the number of counted pulse intervals
  volatile unsigned long pulseInterval = 0;  // Stores the final calibrated pulse interval

  volatile unsigned long lastTriggerTime = 0;  // Stores the time of the most recent detected sound trigger
  volatile unsigned long filteredTiggerTime = 0;  // Stores the filtered version of the trigger time
  volatile unsigned long delta = 0;  // Stores the time difference between the last two triggers
  volatile unsigned long acceptedDelta = 0;  // Stores the accepted delta after filtering noise
  volatile long lastOffset = 0;  // Stores the time offset used for distance calculations
  volatile long distance = 0;  // Stores the raw measured distance
  volatile long filteredDistance = 0;  // Stores the filtered (smoothed) distance
  volatile unsigned long cnt = 0;  // Stores the number of interrupts triggered
  volatile int diff = 0;  // Stores the difference between calculated distances for filtering

public:
  volatile long interruptTurnOnTime = 0;  // Stores the time to re-enable the interrupt

  // Constructor to initialize the sound sensor pin
  Distance(int soundPin) {
    this->soundPin = soundPin;  // Assign the input pin to the class variable
  }

  // Function to check if the system has completed calibration
  bool isCalibrated() {
    return pulseInterval != 0;  // Returns true if pulse interval has been calculated
  }

  // Function to get the count of pulse intervals recorded during calibration
  int getCalibratingCount() {
    return nPulseIntervalCount;  // Returns the number of pulse intervals counted
  }

  // Function to get the calculated pulse interval
  long getPulseInterval() {
    return pulseInterval;  // Returns the pulse interval value
  }

  // Function to get the filtered distance measurement
  long getDistance() {
    return filteredDistance;  // Returns the filtered distance
  }

  // Interrupt Service Routine (ISR) for detecting sound pulses
  void ISR_sound() {
    if (digitalRead(soundPin) == HIGH) {  // Check if the sensor is HIGH (no sound detected)
      return;  // Exit function if no valid sound pulse detected
    }
    
    uint64_t now = micros();  // Get the current time in microseconds
    cnt++;  // Increment the count of detected signals

    if (!preTriggerTime) {  // Check if this is the first trigger event
      preTriggerTime = now;  // Store the current time as the pre-trigger time
      return;  // Exit the function
    }

    if (!initialTriggerTime) {  // Check if the initial trigger time is not set
      if (now - preTriggerTime < 900000) {  // Ensure at least 900ms of quiet time
        preTriggerTime = now;  // Update the pre-trigger time
        return;  // Exit the function
      }
      initialTriggerTime = now;  // Set the initial trigger time
      lastTriggerTime = now;  // Store the last trigger time
      return;  // Exit the function
    }

    if (!pulseInterval) {  // Check if calibration is still in progress
      if (now - lastTriggerTime < 900000)  // Ignore noise that happens too soon after last trigger
        return;  // Exit function

      lastTriggerTime = now;  // Update last trigger time
      nPulseIntervalCount++;  // Increase the pulse interval count
      nPulseInterval = now - initialTriggerTime;  // Calculate the total pulse interval duration

      // Hardcoded calibration values for a specific device (M10)
      nPulseInterval = 100001785;  
      nPulseIntervalCount = 100;

      if (nPulseInterval > 4500000) {  // Ensure calibration takes at least 5 seconds
        pulseInterval = nPulseInterval / nPulseIntervalCount;  // Calculate the pulse interval
      }
      return;  // Exit function
    }

    // Normal operation after calibration
    delta = now - lastTriggerTime;  // Calculate time difference between last two triggers
    lastTriggerTime = now;  // Update last trigger time

    if (delta < pulseInterval * 900 / 1000) {  // Ensure at least 900ms of silence
      return;  // Exit function
    }

    detachInterrupt(digitalPinToInterrupt(soundPin));  // Temporarily disable interrupts
    interruptTurnOnTime = now + 999000;  // Schedule re-enabling after 990ms

    filteredTiggerTime = now;  // Store the filtered trigger time
    acceptedDelta = delta;  // Store the accepted delta value

    lastOffset = ((now - initialTriggerTime) * nPulseIntervalCount) % nPulseInterval;  // Compute offset
    lastOffset /= nPulseIntervalCount;  // Normalize the offset

    if (lastOffset > (signed long)pulseInterval / 2) {  // Adjust offset if it exceeds half the pulse interval
      lastOffset -= pulseInterval;  // Adjust the offset
    }

    long newDistance = lastOffset * SOUND_SPEED / 100;  // Convert offset to distance in 0.01 mm units

    if (abs(newDistance - distance) < 500000) {  // Apply noise filtering
      filteredDistance = (distance + newDistance) / 2;  // Apply smoothing
      distance = newDistance;  // Update distance
    }
  }

  // Function to display calculated distance on Serial Monitor
  void displayDistance() {
    Serial.print("Sound detected time: ");  // Print label
    Serial.print(filteredTiggerTime);  // Print filtered trigger time
    Serial.print(",nPulseInterval: ");  // Print label
    Serial.print(nPulseInterval);  // Print pulse interval
    Serial.print(",nPulseIntervalCount: ");  // Print label
    Serial.print(nPulseIntervalCount);  // Print count of pulse intervals
    Serial.print(",pulseInterval: ");  // Print label
    Serial.print(pulseInterval);  // Print pulse interval value
    Serial.print(", Delta: ");  // Print label
    Serial.print(delta);  // Print delta time
    Serial.print(", acceptedDelta: ");  // Print label
    Serial.print(acceptedDelta);  // Print accepted delta time
    Serial.print(", cnt: ");  // Print label
    Serial.print(cnt);  // Print interrupt count
    Serial.print(", diff: ");  // Print label
    Serial.print(diff);  // Print difference variable
    Serial.print(", offset: ");  // Print label
    Serial.print(lastOffset);  // Print offset value
    Serial.println();  // Newline

    Serial.print(">offset:");  // Print label
    Serial.println(lastOffset);  // Print offset value
    Serial.print(">distance:");  // Print label
    Serial.println(distance / 100.0);  // Print distance in cm
    Serial.print(">filteredDistance:");  // Print label
    Serial.println(filteredDistance / 100.0);  // Print filtered distance in cm
  }
};
