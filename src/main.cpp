#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "distance.h"

const int SOUND_IN = 2;

const int NUM_MIC = 4;
const int LED_OUT[] = {4, 5, 6, 7};
const int MIC_IN[] = {8, 9, 10, 11};
const long ONE_SECOND = 1000000;


long lastLedOnTime = 0;

long lastDistance = 0;
long lastDisplayTime = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);
Distance distance(SOUND_IN);

void ISR_sound() {
  distance.ISR_sound();
}

void setup() {
  Serial.begin(115200);
  Serial.println("setup1");

  for (int pin : LED_OUT)
    pinMode(pin, OUTPUT);
  for (int pin : MIC_IN)
    pinMode(pin, INPUT);

  pinMode(SOUND_IN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SOUND_IN), ISR_sound, FALLING);

  // Initialize the I2C LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Calibration...");
}

void loop() {
  long now = micros();
  if (distance.interruptTurnOnTime > 0 && now > distance.interruptTurnOnTime) {
    Serial.println("Turn on interrupt");
    attachInterrupt(digitalPinToInterrupt(SOUND_IN), ISR_sound, FALLING);
    distance.interruptTurnOnTime = 0;
  }

  int turnOnLed = -1;
  for (int i = 0; i < NUM_MIC; i++) {
    int v = digitalRead(MIC_IN[i]);
    if (v == LOW) {
      turnOnLed = i;
      break;
    }
  }

  if (turnOnLed >= 0 && now - lastLedOnTime > 700000) {
    lastLedOnTime = now;
    for (int i = 0; i < NUM_MIC; i++) {
      digitalWrite(LED_OUT[i], turnOnLed == i);
    }
  }

  // For debugging
  if (now - lastDisplayTime >= ONE_SECOND) {
    distance.displayDistance();
    lastDisplayTime = now;
  }

  long lDistance = distance.getDistance();
  if (lDistance != lastDistance) {
    lastDistance = lDistance;
    lcd.setCursor(0, 0);
    if (!distance.isCalibrated()) {
      lcd.print("Calibrating...");
      lcd.print(distance.getCalibratingCount());
    } else {
      char buf[16];
      double dDistance = distance.getDistance() / 100.;
      dtostrf(dDistance, 6, 2, buf); // Convert double to string with 2 decimal places
      lcd.print("Distance (cm):  ");
      lcd.setCursor(0, 1);
      lcd.print(buf);
      lcd.setCursor(0, 1);
      // lcd.print("Pulse:");
      // lcd.print(distance.getPulseInterval());
    }
  }
}