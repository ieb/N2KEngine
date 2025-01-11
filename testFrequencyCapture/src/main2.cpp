#ifdef USE_COUNTER_CAPTURE


#include <Arduino.h>


/*
 * Targeting a Attiny3226 using TCA0. Would work on most Attiny chips.
 *  This method uses a free running timer at 250KHz divided down from the peripheral clock.
 *  Each edge is counted by an ISR. When the count reaches 50 the value of the counter is captured
 * representing a count of the number of 250KHz cycles over 50 input pulses.
 *
 *  Provided the uint16_t counter does not overflow, which it will only do less than about 200Hz.
 *  The methods appears consistent, stable measurements for stable frequencies tracking changes in
 *  50 cycles.  Accuracy after tuning the internal oscillator measured at +-0.2%., ie 2 RPM at 1K RPM.
 * 
 *  Measurements are stable and as accurate as the clock is.
    blocking:edgeInterrupts:1028 overflowInterrupts:3 ticks:12525 overflows:0 frequency:998.0040 RPM:1996
    blocking:edgeInterrupts:1034 overflowInterrupts:4 ticks:12524 overflows:0 frequency:998.0837 RPM:1996
    blocking:edgeInterrupts:1033 overflowInterrupts:4 ticks:12527 overflows:0 frequency:997.8447 RPM:1996
    blocking:edgeInterrupts:1036 overflowInterrupts:4 ticks:12528 overflows:0 frequency:997.7650 RPM:1996
    blocking:edgeInterrupts:1035 overflowInterrupts:4 ticks:12528 overflows:0 frequency:997.7650 RPM:1996
    blocking:edgeInterrupts:1033 overflowInterrupts:4 ticks:12527 overflows:1 frequency:997.8447 RPM:1996
    blocking:edgeInterrupts:1033 overflowInterrupts:4 ticks:12529 overflows:1 frequency:997.6854 RPM:1995
    blocking:edgeInterrupts:1036 overflowInterrupts:4 ticks:12527 overflows:1 frequency:997.8447 RPM:1996
    blocking:edgeInterrupts:1036 overflowInterrupts:4 ticks:12528 overflows:1 frequency:997.7650 RPM:1996
    blocking:edgeInterrupts:1035 overflowInterrupts:3 ticks:12530 overflows:0 frequency:997.6058 RPM:1995
non blocking:edgeInterrupts:1034 overflowInterrupts:4 ticks:12526 overflows:0 frequency:997.9243 RPM:1996
non blocking:edgeInterrupts:1008 overflowInterrupts:4 ticks:12526 overflows:0 frequency:997.9243 RPM:1996
non blocking:edgeInterrupts:1014 overflowInterrupts:4 ticks:12528 overflows:0 frequency:997.7650 RPM:1996
non blocking:edgeInterrupts:1014 overflowInterrupts:4 ticks:12530 overflows:0 frequency:997.6058 RPM:1995
non blocking:edgeInterrupts:1012 overflowInterrupts:4 ticks:12526 overflows:1 frequency:997.9243 RPM:1996
non blocking:edgeInterrupts:1011 overflowInterrupts:3 ticks:12526 overflows:0 frequency:997.9243 RPM:1996
non blocking:edgeInterrupts:1014 overflowInterrupts:4 ticks:12525 overflows:0 frequency:998.0040 RPM:1996
non blocking:edgeInterrupts:1010 overflowInterrupts:4 ticks:12524 overflows:0 frequency:998.0837 RPM:1996
non blocking:edgeInterrupts:1011 overflowInterrupts:4 ticks:12526 overflows:0 frequency:997.9243 RPM:1996

 *
 */


#if defined(MILLIS_USE_TIMERA0) || defined(__AVR_ATtinyxy2__)
  #error "This sketch takes over TCA0, don't use for millis here.  Pin mappings on 8-pin parts are different"
#endif

uint8_t InputPin = PIN_PC2;

void setup() {
  Serial.begin(19200);

  // setup PIN_PC2 to generate an interrupt
  //pinMode(PIN_PC0, INPUT_PULLUP);

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


  Serial.println("Started");
}


volatile uint8_t edges = 0;
volatile uint8_t slot = 0;
volatile uint8_t capturedSlot = 0;
volatile uint16_t capturedCounts[2] = { 0,0 };
volatile uint16_t capturedOverflows[2] = { 0,0 };
volatile uint16_t timerISRCalls = 0;
volatile uint16_t portISRCalls = 0;

uint16_t edgeInterrupts[2] = {0,0};
uint16_t overflowInterrupts[2] = {0,0};

void readFrequency() {
  // noInterupts seems not to be needed here.
  uint8_t l = 0, p = 1;
  if (capturedSlot == 1) {
    l = 1;
    p = 0;
  }
  uint16_t ticks = capturedCounts[l] - capturedCounts[p];
  uint16_t overflows = capturedOverflows[l] - capturedOverflows[p];
  // end of region copying ISR measurements, rest is not so time sensitive.

  edgeInterrupts[1] = edgeInterrupts[0];
  overflowInterrupts[1] = overflowInterrupts[0];
  edgeInterrupts[0] = portISRCalls;
  overflowInterrupts[0] = timerISRCalls;
  uint16_t nEdgeInterrupts = edgeInterrupts[0] - edgeInterrupts[1];
  uint16_t nOverflowInterrupts = overflowInterrupts[0] - overflowInterrupts[1];


  double frequency = 0;
  if ( nEdgeInterrupts > 200 && overflows < 2) {
    frequency = 50.0*250000.0/(double) ticks;
    Serial.print("edgeInterrupts:");
    Serial.print(nEdgeInterrupts);
    Serial.print(" overflowInterrupts:");
    Serial.print(nOverflowInterrupts);
    Serial.print(" ticks:");
    Serial.print(ticks);
    Serial.print(" overflows:");
    Serial.print(overflows);
    Serial.print(" frequency:");
    Serial.print(frequency,4);
    Serial.print(" RPM:");
    Serial.println(round(frequency*2.0));
  } else {
    Serial.print("edgeInterrupts:");
    Serial.print(nEdgeInterrupts);
    Serial.print(" overflowInterrupts:");
    Serial.print(nOverflowInterrupts);
    Serial.print(" overflows:");
    Serial.print(overflows);
    Serial.print(" frequency:");
    Serial.print(frequency,4);
    Serial.print(" RPM:");
    Serial.println(round(frequency*2.0));
  }
}



void loop() { // Not even going to do anything in here
  static unsigned long lastCall = 0;
  static uint8_t test = 0;
  if (test > 10) {
    unsigned long now = millis();
    if ( now - lastCall > 1000 ) {
      lastCall = now;
      Serial.print("non blocking:");
      readFrequency();
      test++;
    }    
  } else {
    Serial.print("    blocking:");
    readFrequency();
    test++;
    delay(1000);
  }
  if (test == 20) {
    test = 0;
  }
}


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

#endif