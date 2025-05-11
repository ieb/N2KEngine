#include "enginesensors.h"
#ifdef FREQENCY_METHOD_0

#define PULSE_TIME_METHOD
// ISR for frequency measurements.
volatile uint16_t pulseCount = 0;
volatile unsigned long lastPulse=0;
volatile unsigned long thisPulse=0;


// generally at 0-31KHz this works with minimal errors
// and is simple and portable, however the time spend in the ISR is larger than
// ideal.


void flywheelPuseHandler() {
  pulseCount++;
  if ( pulseCount == 100 ) {
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
#ifdef PULSE_COUNT_METHOD
  attachInterrupt(digitalPinToInterrupt(flywheelPin), flywheelPuseHandler2, FALLING);
#endif
#ifdef PULSE_TIME_METHOD
    attachInterrupt(digitalPinToInterrupt(flywheelPin), flywheelPuseHandler, FALLING);
#endif

  Serial.println(F("ISR enabled"));
}

uint16_t countPrev = 0;
unsigned long timePrev = 0;


#ifdef PULSE_COUNT_METHOD
/**
 * Read RPM simply counting pulses and dividing by time
 * Reading at 1s with 30 notches should give 0.5RPM resolution.
 * 
 * However, capturing outside the ISR results in 
 * jitter
 RPM Pulses :1004 Time :1001019 Hz:1002.98 RPM:2005.96
RPM Pulses :1006 Time :1000932 Hz:1005.06 RPM:2010.13
RPM Pulses :1003 Time :1001036 Hz:1001.96 RPM:2003.92
RPM Pulses :1009 Time :1001007 Hz:1007.98 RPM:2015.97
RPM Pulses :1003 Time :1001009 Hz:1001.99 RPM:2003.98
RPM Pulses :1005 Time :1000975 Hz:1004.02 RPM:2008.04
RPM Pulses :1003 Time :1000958 Hz:1002.04 RPM:2004.08
RPM Pulses :1011 Time :1001075 Hz:1009.91 RPM:2019.83
RPM Pulses :1006 Time :1000967 Hz:1005.03 RPM:2010.06
 * millis and micros both have jitter
 */
void EngineSensors::readEngineRPM() {
  uint16_t countNow = pulseCount;
  unsigned long now = micros();

  uint16_t pulses = countNow - countPrev;
  countPrev = countNow;
  unsigned long time = now - timePrev;
  timePrev = now;
  // time is in ms
  double frequency = (double)pulses*1000000.0/(double)time;
  engineRPM = frequency * 2.0;
  if (fakeEngineRunning) {
    engineRPM = 1000;
  }
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

#endif


#ifdef PULSE_TIME_METHOD

/**
 * 
 * Measure the time in ms for 100 pulses, insde the ISR
 * Then calculate the frequency on demand
 * Very stable and accurate on both 3226 and 328p MCUs.
 * But on 3226 will be unstable if micors is disrupted by bugs calling no interpts too often.
 * Eg writing to EEPROM all the time
 * 
RPM Period :100170 Hz:998.30 RPM:1997
RPM Period :100165 Hz:998.35 RPM:1997
RPM Period :100165 Hz:998.35 RPM:1997
RPM Period :100190 Hz:998.10 RPM:1996
Skip, EEPROM written RPM Period :RPM Period :97198 Hz:1028.83 RPM:1996
RPM Period :100190 Hz:998.10 RPM:1996
RPM Period :98179 Hz:1018.55 RPM:2037
RPM Period :100201 Hz:997.99 RPM:1996
RPM Period :100207 Hz:997.93 RPM:1996
RPM Period :100210 Hz:997.90 RPM:1996
RPM Period :97209 Hz:1028.71 RPM:2057
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ Odd
RPM Period :100224 Hz:997.77 RPM:1996
RPM Period :100217 Hz:997.83 RPM:1996
RPM Period :100221 Hz:997.79 RPM:1996
RPM Period :100214 Hz:997.86 RPM:1996
RPM Period :100216 Hz:997.84 RPM:1996
RPM Period :100214 Hz:997.86 RPM:1996
RPM Period :100210 Hz:997.90 RPM:1996
RPM Period :100201 Hz:997.99 RPM:1996
Skip, EEPROM written RPM Period :RPM Period :100228 Hz:997.73 RPM:1996
RPM Period :100206 Hz:997.94 RPM:1996
RPM Period :100197 Hz:998.03 RPM:1996
RPM Period :100196 Hz:998.04 RPM:1996
RPM Period :100213 Hz:997.87 RPM:1996
RPM Period :100222 Hz:997.78 RPM:1996
RPM Period :100198 Hz:998.02 RPM:1996
RPM Period :99222 Hz:1007.84 RPM:2016
RPM Period :100217 Hz:997.83 RPM:1996
RPM Period :100201 Hz:997.99 RPM:1996
RPM Period :100213 Hz:997.87 RPM:1996
Luna engine monitor start
Opening CAN
 *
 */

void EngineSensors::readEngineRPM(bool outoutDebug) {
  noInterrupts();
  unsigned long lastP = lastPulse;
  unsigned long thisP = thisPulse;
  interrupts();

  unsigned long period = thisP - lastP;
  double frequency = 0;
  // non zero period and update in the last second
  if ( (period > 0) && (micros() - thisP) < 1000000) {
    // over 10 periods, so multiply top by 10.
    frequency = 100000000.0/((double)period);
  }


  if ( eepromWritten ) {
    if ( outoutDebug ) {
     Serial.print(F("Skip, EEPROM written RPM Period :"));
    }
    eepromWritten = false;

  } else {
    if ( frequency < 200 || frequency > 4000 ) {
      if ( fakeEngineRunning ) {
        engineRPM = 1000;
      } else {
        engineRPM = 0;
      }
    } else {
      engineRPM = 2.0*frequency;
    }
  }
  if (fakeEngineRunning) {
    engineRPM = 1000;
  }

  if ( outoutDebug ) {
    Serial.print(F("RPM Period :"));
    Serial.print(period);
    Serial.print(F(" Hz:"));
    Serial.print(frequency);
    Serial.print(F(" RPM:"));
    Serial.println(round(engineRPM));
  }
  return engineRPM;
}
#endif

#endif