/* 
Uses the standard Attiny 3226 Frequency measurement mechanism by triggering 
an event on every pulse and capturing a timer. The event triggering is handled 
by the chip internally without the CPU so it should be faster than the cpu freq 
according to the datasheet.
Tested as a starting point for measuring between 300Hz and 3KHz.
Found to not deliver stable readings over time, although they are mostly accurate.
There seems to be some correlation between the millis and micos functions and the 
counter on TIMERB0. Timer B1 is used for millis.
eg calling delay results in very bad readings indicating interference with the timers.

TL;DR doesnt work on a attiny well enough to really use... odd as its the method in the
datasheet. See main2.cpp which does look stable and accurate.


output , at a verified stable and noise free 1KHz signal shows the measurements are not stable.

Timer capture: 16032 edges: 11691  Frequency: 998.00396  RPM: 1996
Timer capture: 16033 edges: 12199  Frequency: 997.94177  RPM: 1996
Timer capture: 16033 edges: 12709  Frequency: 997.94177  RPM: 1996
Timer capture: 16061 edges: 13220  Frequency: 996.20196  RPM: 1992
Timer capture: 16039 edges: 13727  Frequency: 997.56842  RPM: 1995
Timer capture: 16048 edges: 14231  Frequency: 997.00897  RPM: 1994


*/

#ifdef USE_FREQUENCY_CAPTURE



#import <Arduino.h>
volatile uint16_t timer_capture = 0;
volatile uint16_t edges = 0;
uint16_t lastEdges = 0;

void setup () {
  //Serial.swap();// use this to set the TXD*/RXD* to their alternate pins
  Serial.begin(19200);// Default Serial TX is on pin 0 (PA6)
  PORTC.PIN2CTRL = PORT_PULLUPEN_bm ; // pullup on the input pin

  // configure event channel
#if defined(__AVR_TINY_2__)// The 2 series parts have a different event system configuration
  EVSYS.CHANNEL5 = EVSYS_CHANNEL5_PORTC_PIN2_gc; //link event system ch0 to pin A2
  EVSYS.USERTCB0CAPT = EVSYS_USER_CHANNEL5_gc;// Connect TCB0 capture to event channel 0
#else// This is for the 0 and 1 series parts
  EVSYS.ASYNCCH0 = EVSYS_ASYNCCH0_PORTA_PIN2_gc; //link event system async ch0 to pin A2
  EVSYS.ASYNCUSER0 = EVSYS_ASYNCUSER0_ASYNCCH0_gc;// 0x01 Link TimerB (=AYNCUSER0) to event ch0
#endif

  // configure timer B
  TCB0.CTRLB |= TCB_CNTMODE_FRQ_gc; // (0x04<<0), Input Capture Frequency Measurement
  //  on rising edge, will capture into CCMP and reset counter
  TCB0.EVCTRL |= TCB_FILTER_bm ; // activate Input Capture Noise Cancellation Filter
  TCB0.EVCTRL |= TCB_CAPTEI_bm ; // Capture Event Input Enable
//  TCB0.EVCTRL |= TCB_EDGE_bm ; // uncomment if the falling edge needs to be used. Default rising edge
  TCB0.INTCTRL |= TCB_CAPT_bm ; // Capture Interrupt Enable
  TCB0.CTRLA |= (1 << 0); // enable TCB0
}

void loop () {
  uint16_t currentEdges = edges;
  uint16_t ticks  = timer_capture;
  if ( ticks > 4000 && currentEdges != lastEdges) {
    lastEdges = currentEdges;    

    double frequency = ((double)F_CPU/(double)ticks);
    Serial.print("Timer capture: ");
    Serial.print(ticks);
    Serial.print(" edges: ");
    Serial.print(currentEdges);
    Serial.print("  Frequency: ");
    Serial.print(frequency,5);
    Serial.print("  RPM: ");
    Serial.println(round(frequency*2.0));
  } else {
    Serial.println("  Frequency: 0Hz ");
  }
  delay(1000);
}

ISR (TCB0_INT_vect) {
  /*The CAPT Interrupt flag is automatically cleared 
  after the low byte of the Compare/Capture (TCBn.CCMP) register has been read.
  datasheet chapter 21.3.3.1.5  */ 
  edges++;
  timer_capture = TCB0_CCMP;
}

#endif