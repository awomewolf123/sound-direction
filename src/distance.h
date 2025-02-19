#include <Arduino.h>

const double SOUND_SPEED=340.4;

class Distance {
  unsigned int soundPin = 0;
  volatile unsigned long preTriggerTime = 0;
  volatile uint64_t initialTriggerTime = 0;

  volatile unsigned long nPulseInterval = 0;
  volatile unsigned long nPulseIntervalCount = 0;
  volatile unsigned long pulseInterval = 0;

  volatile unsigned long lastTriggerTime = 0;
  volatile unsigned long filteredTiggerTime = 0;
  volatile unsigned long delta = 0;
  volatile unsigned long acceptedDelta = 0;
  volatile long lastOffset = 0;
  volatile long distance = 0;
  volatile long filteredDistance = 0;
  volatile unsigned long cnt = 0;
  volatile int diff = 0;

public:
  volatile long interruptTurnOnTime = 0;
  Distance(int soundPin) {
    this->soundPin = soundPin;
  }

  bool isCalibrated() {
    return pulseInterval != 0;
  }

  int getCalibratingCount() {
    return nPulseIntervalCount;
  }

  long getPulseInterval() {
    return pulseInterval;
  }

  long getDistance() {
    return filteredDistance;
  }

  void ISR_sound() {
    if (digitalRead(soundPin) == HIGH) {
      return;
    }
    uint64_t now = micros();
    cnt ++;

    if (!preTriggerTime) {
      preTriggerTime = now;
      return;
    }

    if (!initialTriggerTime) {
      // Wait for 500ms quiet time
      if (now - preTriggerTime < 900000) {
        preTriggerTime = now;
        return;
      }
      initialTriggerTime = now;
      lastTriggerTime = now;
      return;
    }

    if (!pulseInterval) {
      // Ignore noise that happens close to last trigger
      if (now - lastTriggerTime < 900000)
        return;

      lastTriggerTime = now;
      nPulseIntervalCount ++;
      nPulseInterval = now - initialTriggerTime;

      // Hardcoded for specific device (M10)
      nPulseInterval = 100001785;
      nPulseIntervalCount = 100;

      // Take at least 5s for calibration
      if (nPulseInterval > 4500000) {
        pulseInterval = nPulseInterval / nPulseIntervalCount;
      }
      return;
    }

    // Calibration is done, will do normal calculation
    delta = now - lastTriggerTime;
    lastTriggerTime = now;

    // Make sure there's 900ms of silent time before next trigger
    if (delta < pulseInterval * 900 / 1000) {
      return;
    }
    
    // Turn off the interrupt to avoid CPU overload cause timeer interrupt misfire which
    // causes arduino clock slow down
    detachInterrupt(digitalPinToInterrupt(soundPin));
    // Turn on the interrupt after 990ms
    interruptTurnOnTime = now + 999000;

    filteredTiggerTime = now;
    acceptedDelta = delta;

    // // Adjust initialTriggerTime to avoid overflow
    // initialTriggerTime += (now - initialTriggerTime) / nPulseInterval * nPulseInterval;

    // lastTriggerTime = now;
    lastOffset = ((now - initialTriggerTime) * nPulseIntervalCount) % nPulseInterval;

    lastOffset /= nPulseIntervalCount;
    if (lastOffset > (signed long)pulseInterval / 2) {
      lastOffset -= pulseInterval;
    }

    // In unit of 0.01 millimeter
    long newDistance = lastOffset * SOUND_SPEED / 100;
    // Filter out noise that exceed 50cm
    if (abs(newDistance - distance) < 500000) {
      // The time data will jump up/down for each sample. 
      filteredDistance = (distance + newDistance) / 2;
      distance = newDistance;
    }
  }


  void displayDistance() {
    Serial.print("Sound detected time: ");
    Serial.print(filteredTiggerTime);
    Serial.print(",nPulseInterval: ");
    Serial.print(nPulseInterval);
    Serial.print(",nPulseIntervalCount: ");
    Serial.print(nPulseIntervalCount);
    Serial.print(",pulseInterval: ");
    Serial.print(pulseInterval);
    Serial.print(", Delta: ");
    Serial.print(delta);
    Serial.print(", acceptedDelta: ");
    Serial.print(acceptedDelta);
    Serial.print(", cnt: ");
    Serial.print(cnt);
    Serial.print(", diff: ");
    Serial.print(diff);
    Serial.print(", offset: ");
    Serial.print(lastOffset);
    Serial.println();

    // Plot 
    Serial.print(">offset:");
    Serial.println(lastOffset);
    Serial.print(">distance:");
    Serial.println(distance/100.);
    Serial.print(">filteredDistance:");
    Serial.println(filteredDistance/100.);
  }
};