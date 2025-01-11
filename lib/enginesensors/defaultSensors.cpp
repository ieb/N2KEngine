#include "enginesensors.h"
#ifndef __AVR_TINY_2__

// ISR for frequency measurements.
volatile uint16_t pulseCount = 0;
volatile unsigned long lastPulse=0;
volatile unsigned long thisPulse=0;


// generally at 0-31KHz this works with minimal errors
// and is simple and portable, however the time spend in the ISR is larger than
// ideal.


void flywheelPuseHandler() {
  pulseCount++;
  if ( pulseCount == 10 ) {
    lastPulse = thisPulse;
    thisPulse = micros();
    pulseCount = 0;
  }
}

void flywheelPuseHandler2() {
  pulseCount++;
}

// nothing to do.
void setupAdc() {
}


void setupTimerFrequencyMeasurement(uint8_t flywheelPin) {
  // PULLUP is required with the LMV393 which is open collector.
  pinMode(flywheelPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flywheelPin), flywheelPuseHandler2, FALLING);
  Serial.println(F("ISR enabled"));
}

uint16_t countPrev = 0;
unsigned long timePrev = 0;

void EngineSensors::readEngineRPM() {
  uint16_t countNow = pulseCount;
  unsigned long now = millis();
  uint16_t pulses = countNow - countPrev;
  countPrev = countNow;
  unsigned long time = now - timePrev;
  timePrev = now;
  double frequency = (double)pulses*1000.0/(double)time;
  engineRPM = frequency * 2.0;
  Serial.print(F("RPM Pulses :"));
  Serial.print(pulses);
  Serial.print(F(" Time :"));
  Serial.print(time);
  Serial.print(F(" Hz:"));
  Serial.print(frequency);
  Serial.print(F(" RPM:"));
  Serial.println(engineRPM);
  return engineRPM;
}


/*
void EngineSensors::readEngineRPM() {
  noInterrupts();
  unsigned long lastP = lastPulse;
  unsigned long thisP = thisPulse;
  interrupts();
  unsigned long period = thisP - lastP;
  double frequency = 0;
  if ( (period > 0) ) {
    // periods is measured in us.
    // 10 periods measured
    frequency = 10000000/((double)period);
  }
  if ( frequency < 200 || frequency > 4000 ) {
    if ( fakeEngineRunning ) {
      engineRPM = 1000;
    } else {
      engineRPM = 0;
    }
  } else {
    engineRPM = 2.0*frequency;
  }
  Serial.print(F("RPM Period :"));
  Serial.print(period);
  Serial.print(F(" Hz:"));
  Serial.print(frequency);
  Serial.print(F(" RPM:"));
  Serial.println(engineRPM);
  return engineRPM;
} */

#endif