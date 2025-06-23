#include "enginesensors.h"

#ifdef __AVR_TINY_2__
#ifdef FREQENCY_METHOD_2

/**
 * Uses a fast running timer to measure the period between 10 or more pulses.
 * Does not reset the timer which is free running.
 * May need to track timer overflows to handle long periods.
 * 
 * uint16_t timer running at clock speed which is how CPU_PER is setup gives.
 * 4ms before overflow or about 246Hz, so tracking overflow is a requirement
 * Or a timer with lower division Timer B cant be divided as it runs the millis and micros
 * unless we disable both and rely on some other timer for making the stack event driven, not so crazy.
 * 
 * could use overflow, however reading the timer becomes problematic as it may overflow during reading.
 * (same problem in micros). We already know that using micros to time n pulses is not stable.
 * 
 * Timer A allows divisors which will allow a lower count rate.
 * CPU_PER/16 = 1uS per tick, CPU_PER/64 gives 250 Khz and 0.26s before overflow, which would allow a max of 40
 * cycles to be sampled at idle (415Hz)  Will need to take over Timer A and make it 16 bit.
 */ 

// Falling edge will contain the numbe of F_CPU clock ticks for the previous 8 cycles 
// of the input.

volatile uint8_t edges = 0;
volatile uint8_t slot = 0;
volatile uint8_t capturedSlot = 0;
volatile uint16_t capturedCounts[2] = { 0,0 };
volatile uint16_t capturedOverflows[2] = { 0,0 };
volatile uint16_t timerISRCalls = 0;
volatile uint16_t portISRCalls = 0;



// Interrupt on edge and capture the counter ever 10 edges.
ISR(PORTC_PORT_vect) {
  uint8_t flags = PORTC.INTFLAGS;
  if ((flags & 0x04) == 0x04) {
    portISRCalls++;
    edges++;
    if ( edges == 50 ) {
      capturedCounts[slot] = TCA0.SINGLE.CNT;
      capturedOverflows[slot] = timerISRCalls;
      capturedSlot = slot;
      slot = (slot+1)&0x01;
      edges = 0;
    }
  }
  PORTC.INTFLAGS = flags;
}

// interupt on overflow.
ISR(TCA0_OVF_vect) { 
  timerISRCalls++;
  TCA0.SINGLE.INTFLAGS  = TCA_SINGLE_OVF_bm; // Always remember to clear the interrupt flags, otherwise the interrupt will fire continually!
}


void setupAdc() {
  analogReference(VDD); // set reference to the desired voltage, and set that as the ADC reference.
  delay(100);
  analogClockSpeed(300); // 300KHz sample rate
}

void setupTimerFrequencyMeasurement(uint8_t flywheelPin) {
 

   PORTC.PIN2CTRL |= PORT_ISC_FALLING_gc | PORT_PULLUPEN_bm ; // pullup on the input pin

  // Counter needs to be setup with clock/64 and overflow interrupts running
  takeOverTCA0();           
  
  // 16000000/64 = 250KHz 250000/2^16 =  3.8146972656 overflows/s
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL0_bm | TCA_SINGLE_CLKSEL2_bm; // div 64
  TCA0.SINGLE.CTRLB = 0x00; // norma, just want the counter, no output.
  TCA0.SINGLE.CTRLC = 0x00; // nothing to set
  //TCA0.SINGLE.EVCTRL = 0x00; // disable event actions
  TCA0.SINGLE.PER = 0xFFFF; // Count all the way up to 0xFFFF
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm; //enable overflow interrupt
  TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm; //enable the timer keeping the prescaler.

#ifdef WITH_DEBUG
  Serial.println("Frequency measurement 2: Timer A0 setup done");
#endif

}



void EngineSensors::readEngineRPM(bool outputDebug) {

  noInterrupts();
  uint8_t l = 0, p = 1;
  if (capturedSlot == 1) {
    l = 1;
    p = 0;
  }
  uint16_t edgesL = edges;
  uint16_t ticks = capturedCounts[l] - capturedCounts[p];
  uint16_t overflows = capturedOverflows[l] - capturedOverflows[p];
  interrupts();

  edgeInterrupts[1] = edgeInterrupts[0];
  overflowInterrupts[1] = overflowInterrupts[0];
  edgeInterrupts[0] = portISRCalls;
  overflowInterrupts[0] = timerISRCalls;
  uint16_t nEdgeInterrupts = edgeInterrupts[0] - edgeInterrupts[1];
  uint16_t nOverflowInterrupts = overflowInterrupts[0] - overflowInterrupts[1];


  double frequency = 0;
  // ignore noise at over 4KHz (ie 8KRPM) indicated ( ticks > 3000)
  // and ignore where not enough pulses have been seen (nEdgeInterupts > 150)
  if ( nEdgeInterrupts > 150 && overflows < 2 && ticks > 3000) {
    frequency = 50.0*250000.0/(double) ticks;
#ifdef WITH_DEBUG
    if ( outputDebug ) {
      Serial.print(F("valid   "));
    }
  } else {
    if ( outputDebug ) {
      Serial.print(F("invalid "));
    }
  }
  if ( outputDebug ) {
    Serial.print(F(" edges:"));
    Serial.print(edgesL);
    Serial.print(F(" ticks:"));
    Serial.print(ticks);      
#endif
  }
  engineRPM = round(frequency*2.0);
  if (fakeEngineRunning) {
    engineRPM = 1000;
  }

#ifdef WITH_DEBUG
  if ( outputDebug ) {
    Serial.print(F(" edgeInterrupts:"));
    Serial.print(nEdgeInterrupts);
    Serial.print(F(" overflowInterrupts:"));
    Serial.print(nOverflowInterrupts);
    Serial.print(F(" overflows:"));
    Serial.print(overflows);
    Serial.print(F(" frequency:"));
    Serial.print(frequency,4);
    Serial.print(F(" RPM:"));
    Serial.println(round(frequency*2.0));
  }
#endif

}

#endif
#endif