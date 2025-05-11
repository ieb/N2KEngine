#include "enginesensors.h"

#ifdef __AVR_TINY_2__
#ifdef FREQENCY_METHOD_1

// Falling edge will contain the numbe of F_CPU clock ticks for the previous 8 cycles 
// of the input.

//volatile uint16_t fallingEdge[8];
//volatile uint8_t slot = 0;
//volatile uint8_t overflows = 0;
//volatile uint16_t over = 0;
//volatile uint16_t captures = 0;

volatile uint16_t singleCapture = 0;
volatile uint16_t interupts = 0;

ISR(TCB0_INT_vect) {
  singleCapture = TCB0.CCMP;
  interupts++;
//  fallingEdge[(slot++)&0x07] = TCB0.CCMP;
}

void setupAdc() {
  analogReference(VDD); // set reference to the desired voltage, and set that as the ADC reference.
  delay(100);
  analogClockSpeed(300); // 300KHz sample rate
}

void setupTimerFrequencyMeasurement(uint8_t flywheelPin) {
  
  // Hard coded flywheel pin to PIN_PC2.
  PORTC.PIN2CTRL = PORT_PULLUPEN_bm ; // pullup on the input pin

  // Wire the pin through the event mechanism on ch5 to B0 Capture.
  EVSYS.CHANNEL5 = EVSYS_CHANNEL5_PORTC_PIN2_gc; //link event system ch0 to pin A2
  EVSYS.USERTCB0CAPT = EVSYS_USER_CHANNEL5_gc;// Connect TCB0 capture to event channel 5


  // https://ww1.microchip.com/downloads/en/DeviceDoc/ATtiny3224-3226-3227-Data-Sheet-DS40002345A.pdf
  //11.5.2 for the main clock prescaler, defined in megaTinyCore wiring.c must not be changed, set to F_CPU
  // 22.1 page 243 for the Timer B description.
  // Using Frequency Capture 22.3.3.1.4  
  // the peripheral clock (CLK_PER) has no prescale on it, so its running at 16MHz
  // good and bad. It will overflow every 0.25s (ie 4Hz so thats ok, but we must detect it)
  // could divide CLK_PER/2 if we wanted to, but its not necessary.
  // zero B to reset ut
  TCB0.CTRLA = 0x0;
  TCB0.CTRLB = 0x0;
  TCB0.CNT = 0x0;
  TCB0.CCMP = 0x0;
  TCB0.CTRLB |= TCB_CNTMODE_FRQ_gc; // TCB_CNTMODE_FRQ_gc
  TCB0.INTCTRL |= TCB_CAPT_bm; // enable interrupt on capture
  //TCB0.INTCTRL |= TCB_OVF_bm; // enable interrupt on overflow.. no dont, this breaks the capture.
  TCB0.EVCTRL  |= TCB_CAPTEI_bm; // enable capture event
  TCB0.EVCTRL  |= TCB_EDGE_bm; // falling edge 
  TCB0.EVCTRL  |= TCB_FILTER_bm; // enable noise cancellation 


  TCB0.CTRLA |= TCB_ENABLE_bm; // enable counter



  Serial.println(F("Frequency Measurement 1 Timer B0 and event routing  setup done"));

}

void EngineSensors::readEngineRPM(bool outputDebug) {

  // copy the capture over, and the interupts, these are volatile.
  noInterrupts();
  uint16_t ticks = singleCapture;
  uint16_t interruptCount = interupts;
  interrupts();

  //
  uint16_t nInterrupts = interruptCount - lastInteruptCount;
  uint16_t frequency = 0;
  uint16_t rpm = 0;
  if ( nInterrupts > 10) {
    lastInteruptCount = interruptCount;
    if ( ticks > 4000 ) {
      frequency = 160000000/ticks;
      rpm = frequency*2;
    }
  }
  smoothedRPM[(slot++)&0x03] = rpm;
  float frpm = (float)smoothedRPM[0] 
    + (float)smoothedRPM[1] 
    + (float)smoothedRPM[2]
    + (float)smoothedRPM[3];
  engineRPM = frpm/40.0;
  if (fakeEngineRunning) {
    engineRPM = 1000;
  }

  if ( outputDebug ) {
    Serial.print(F("RPM pulses :"));
    Serial.print(ticks);
    Serial.print(F(" interupts:"));
    Serial.print(nInterrupts);
    Serial.print(F(" Hz:"));
    Serial.print(frequency);
    Serial.print(F(" Hz:"));
    Serial.print(nInterrupts*2);
    Serial.print(F(" RPM:"));
    Serial.println(engineRPM);    
  }
}

#endif
#endif