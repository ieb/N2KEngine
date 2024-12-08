#include "enginesensors.h"
#include <util/crc16.h>
#include <EEPROM.h>
#include <SmallNMEA2000.h>


#ifdef DEBUGON
#define DEBUG(x) if (debug) Serial.print(x)
#define DEBUGLN(x) if (debug) Serial.println(x)
#define INFO(x)  Serial.print(x)
#define INFOLN(x) Serial.println(x)
#else
#define DEBUG(x)
#define DEBUGLN(x)
#define INFO(x)
#define INFOLN(x)
#endif

// ISR for frequency measurements.
volatile int8_t pulseCount = 0;
volatile unsigned long lastPulse=0;
volatile unsigned long thisPulse=0;
// record the time take for 10 pulses in us using micros, Should be fine 
// for < 50KHz which would read 200uS.
// 50Hz will be 200000.
void flywheelPuseHandler() {
  pulseCount++;
  if ( pulseCount == 10 ) {
    lastPulse = thisPulse;
    thisPulse = micros();
    pulseCount = 0;
  }
}

bool EngineSensors::begin() {
  loadEngineHours();
  // PULLUP is required with the LMV393 which is open collector.
  pinMode(flywheelPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flywheelPin), flywheelPuseHandler, FALLING);
  DEBUG(F("ISR enabled"));
  return true;
}

void EngineSensors::read() {
  unsigned long now = millis();
  if ( now-lastFlywheelReadTime > flywheelReadPeriod) {
    lastFlywheelReadTime = now;
    readEngineRPM();
  }
  saveEngineHours();
}


void EngineSensors::loadEngineHours() {
  uint16_t crc = 0;
  for (int i = 0; i < 4; i++) {    
    crc = _crc16_update(crc, EEPROM.read(i+2));
  }
  CRCStorage crcStore;
  crcStore.crcBytes[0] = EEPROM.read(0);
  crcStore.crcBytes[1] = EEPROM.read(1);
  if ( crcStore.crc == crc) {
    // data is valid
    for (int i = 0; i < 4; i++) {    
      engineHours.engineHoursBytes[i] = EEPROM.read(i+2);
    }
  }
  Serial.print(F("Hours: "));
  Serial.println(0.004166666667*engineHours.engineHoursPeriods);
}

void EngineSensors::writeEnginHours() {
  CRCStorage crcStore;
  crcStore.crc = 0;
  for (int i = 0; i < 4; i++) {    
    crcStore.crc = _crc16_update(crcStore.crc, engineHours.engineHoursBytes[i]);
  }
  EEPROM.write(0, crcStore.crcBytes[0]);
  EEPROM.write(1, crcStore.crcBytes[1]);
  for (int i = 0; i < 4; i++) {
    EEPROM.write(i+2, engineHours.engineHoursBytes[i]);
  }
}


void EngineSensors::setEngineSeconds(double seconds) {
  engineHours.engineHoursPeriods = seconds/15L;
  writeEnginHours();
}

double EngineSensors::getEngineSeconds() {  
  return 15L*engineHours.engineHoursPeriods; 
};


bool EngineSensors::isEngineRunning() { 
  return engineRunning; 
};


void EngineSensors::saveEngineHours() {
  // Only count engine hours while the engine is running.
  // when the engine is running, increment the engineHoursPeriod by 1 every 15s
  unsigned long now = millis();
  if ( engineRunning ) {
    if ( engineRPM == 0 ) {
      engineRunning = false;
    } else if ( now-lastEngineHoursTick > ENGINE_HOURS_PERIOD_MS ) {
      lastEngineHoursTick = now;
      engineHours.engineHoursPeriods++;
      writeEnginHours();
    }
  } else if ( engineRPM > 0 ) {
      engineRunning = true;
      lastEngineHoursTick = now;
  }
}






void EngineSensors::readEngineRPM() {
  noInterrupts();
  unsigned long lastP = lastPulse;
  unsigned long thisP = thisPulse;
  interrupts();
  unsigned long period = thisP - lastP;
  if ( (period == 0) || (micros() - thisP) > 1000000 ) {
    // no pulses, > 10MHz or < 10Hz, assume not running.
    engineRPM = 0;
  } else {
     engineRPM =  (RPM_FACTOR*10000000.0)/(double)period;
    DEBUG("Period ");DEBUGLN(period);
    DEBUG("RPM ");DEBUGLN(engineRPM);
  }
}

double EngineSensors::getEngineRPM() {
  return engineRPM;
}





       


double EngineSensors::getFuelCapacity() {
  return FUEL_CAPACITY;
}
 
// European Fuel sensor goes 0-190, 190 being full, 0 being empty.
double EngineSensors::getFuelLevel(uint8_t adc) { 
    // probably need a long to do this calc
    DEBUG(F("Fuel:"));
    double fuelReading = analogRead(adc);
    DEBUG(fuelReading);
    DEBUG(",");
    // Rtop = 1000
    // Rempty = 0
    // Rfull = 190
    // diode drop 0.63v
    // ADCempty = (5-0.63)*190/(190+1000) =  0.697731092436975 = 1024*0.697731092436975/5 = 142
    // TODO measure and check.

    // ADCfull = 0 = 0v 0

    fuelReading  = 100.0*(fuelReading/142.0);
    // the restances may be out of spec so deal with > 100 or < 0.
    DEBUGLN(fuelReading);
    if (fuelReading > 100 ) {
      return 100.0;
    } else if ( fuelReading < 0) {
      return 0.0;
    } 
    return (double)fuelReading;
}


/*
Thermistor temperature curve from Volvo Penta workshop manual is below.


Assume a 12V supply, but also measure it and adjust based on the current reading by scaling the reading.

12*700/(700+1000)=4.9, so min temperature that can be read is about 18C for a 12V supply.
12/(22+1000)=11mA max current through temperature sensor.

Coolant supply voltage uses a 100K/470K divider so 12V == 12*470/(1470) = 3.836V
ADC reading at 12V = 1024*(12*470/(1470))/5 = 786
ADC reading at 4v = 1024*(4*470/(1470))/5 = 261

Scaling of coolant reading to 12V = CoolantReading*786/SupplyVoltage.

The reason we are doing this is so that the relay board alarms can operate with no 
regulated 8V supply from the NMEA200 board, which may fail. If the coolant supply was
8v regulated it would depend on the MCU supply voltage (and the 8V regulator)

Model   Volvo Penta Standard Coolant Sensor             
R top   1000                
Vsupply 12              
                    
                    
12*1076/(1076+1000) = 6.2196531792                  
                    
C   R   V   ADC Current (mA)    Power (mW)
0   1743    7.625227853 1562    4.374772147 33.35863443
10  1076    6.219653179 1274    5.780346821 35.95175248
20  677 4.844364937 992 7.155635063 34.6645076
30  439 3.660875608 750 8.339124392 30.52849708
40  291 2.704879938 554 9.295120062 25.14218378
50  197 1.974937343 404 10.02506266 19.79887061
60  134 1.417989418 290 10.58201058 15.00517903
70  97  1.061075661 217 10.93892434 11.60702637
80  70  0.785046729 161 11.21495327 8.804262381
90  51  0.582302569 119 11.41769743 6.648554546
100 38  0.4393063584    90  11.56069364 5.078686224
110 29  0.3381924198    69  11.66180758 3.943934925
120 22  0.2583170254    53  11.74168297 3.03307662*/



const int16_t coolantTable[] PROGMEM= {
  1274,
  992,
  750,
  554,
  404,
  290,
  217,
  161,
  119,
  90,
  69,
  53
  };
#define COOLANT_TABLE_LENGTH 12
#define COOLANT_MIN_TEMPERATURE 10
#define COOLANT_MAX_TEMPERATURE 120
#define COOLANT_STEP 10



double EngineSensors::getCoolantTemperatureK(uint8_t coolantAdc, uint8_t batteryAdc) {
  // probably need a long to do this calc
    // need 32 bits, 1024*1024 = 1M
    DEBUG(F("Coolant:"));
    int32_t coolantSupply = (int32_t) analogRead(batteryAdc);
    int32_t coolantReading = (int32_t)analogRead(coolantAdc);

//    // fake up 12v supply to testing purposes
//    coolantSupply = COOLANT_SUPPLY_ADC_12V;
    
    
    
    if ( coolantSupply < MIN_SUPPLY_VOLTAGE) {
      DEBUGLN(F("no power"));
      return SNMEA2000::n2kDoubleNA;
    }
    DEBUG(coolantSupply);
    DEBUG(",");
    DEBUG(coolantReading);
    DEBUG(",");
    coolantReading = (coolantReading * COOLANT_SUPPLY_ADC_12V)/coolantSupply;
    DEBUG(coolantReading);
    DEBUG(",");
    int16_t coolantTemperature = interpolate(coolantReading,COOLANT_MIN_TEMPERATURE, COOLANT_MAX_TEMPERATURE, COOLANT_STEP, coolantTable, COOLANT_TABLE_LENGTH);
    DEBUG("CCC");DEBUGLN(coolantTemperature);
    return ((double)coolantTemperature)+273.15; 

}


double EngineSensors::getVoltage(uint8_t adc) { 
  double voltage = VOLTAGE_SCALE*analogRead(adc);
  DEBUG(F(" Adc:"));
  DEBUG(adc);
  DEBUG(F(" V:"));
  DEBUGLN(voltage);
  return voltage; 
};



/*
https://docs.google.com/spreadsheets/d/1cH6MjYFLKQYQMPswFqU3-U8cdn9rH_bujdkhxfRw2cY/edit#gid=1843834819
Using https://en.wikipedia.org/wiki/Thermistor
Model   MT52-10K        Voltage 5       
B   3950        Top R   4700        
To  298.15      Max Current 1.063829787 mA  
Ro  10000       Max allowed power   50      
rinf    0.01763226979                   
C   K   R   V   ADC current (mA)    Power (mW)
-20 253.15  105384.6902 4.786527991 980 0.04541957642   0.2174020739
-15 258.15  77898.10758 4.71548985  966 0.06053407453   0.285447814
-10 263.15  58245.71279 4.626662421 948 0.07943352738   0.3675121161
-5  268.15  44026.04669 4.517711746 925 0.1026145222    0.4635828322
0   273.15  33620.60372 4.386752877 898 0.1304781114    0.5723752304
5   278.15  25924.56242 4.232642097 867 0.1632676389    0.6910534817
10  283.15  20174.57702 4.055260317 831 0.2010084431    0.8151415627
15  288.15  15837.14766 3.855732043 790 0.2434612674    0.9387214101
20  293.15  12535.32581 3.636521279 745 0.2901018556    1.054961571
25  298.15  10000   3.401360544 697 0.3401360544    1.156925355
30  303.15  8037.140687 3.155001929 646 0.3925527811    1.238504782
35  308.15  6505.531126 2.902821407 594 0.4462082113    1.295262748
40  313.15  5301.466859 2.650344661 543 0.4999266678    1.324977975
45  318.15  4348.13685  2.40278022  492 0.5525999532    1.327776237
50  323.15  3588.182582 2.164637752 443 0.6032685635    1.305857907
55  328.15  2978.435896 1.939480863 397 0.6511742844    1.262940063
60  333.15  2486.164751 1.72982727  354 0.6957814318    1.203581695
65  338.15  2086.372143 1.537177817 315 0.7367706772    1.132547541
70  343.15  1759.837009 1.362137316 279 0.7740133371    1.054312449
75  348.15  1491.682246 1.204585593 247 0.8075349802    0.9727450031
80  353.15  1270.320235 1.063862728 218 0.8374760152    0.8909595185
85  358.15  1086.670769 0.9389429714    192 0.8640546869    0.8112980752
90  363.15  933.5770578 0.828582842 170 0.8875355655    0.7353967412
95  368.15  805.3667326 0.7314378603    150 0.9082047106    0.6642953102
100 373.15  697.519773  0.6461484185    132 0.9263514003    0.5985604923
105 378.15  606.4157635 0.5713986526    117 0.9422556058    0.5384035836
110 383.15  529.1404013 0.5059535226    104 0.9561801016    0.4837826907
115 388.15  463.3365226 0.4486793767    92  0.9683660901    0.4344858937
120 393.15  407.0887766 0.398552673 82  0.9790313462    0.39019556
125 398.15  358.8338772 0.3546606648    73  0.9883700713    0.3505359866
130 403.15  317.290402  0.3161969675    65  0.9965538367    0.3151073011
135 408.15  281.403613  0.2824541383    58  1.003733162 0.2835085854
140 413.15  250.301877  0.2528147608    52  1.010039413 0.2553528725
145 418.15  223.2620904 0.2267420323    46  1.015586802 0.2302762154
*/

const int16_t tcurveNMF5210K[] PROGMEM= {
    980,
    966,
    948,
    925,
    898,
    867,
    831,
    790,
    745,
    697,
    646,
    594,
    543,
    492,
    443,
    397,
    354,
    315,
    279,
    247,
    218,
    192,
    170,
    150,
    132,
    117,
    104,
    92,
    82,
    73,
    65,
    58,
    52,
    46
};
// in 0.1C steps.
#define NMF5210K_MIN -200
#define NMF5210K_MAX 1450
#define NMF5210K_STEP 50
#define NMF5210K_LENGTH 33
// ADC value that indicates a NTC is not connected.
#define DISCONNECTED_NTC 1000


// tested ok 20210909
double EngineSensors::getTemperatureK(uint8_t adc) {
  // probably need a long to do this calc
  DEBUG(F("Temp "));
  int ntcReading = analogRead(adc);
  if ( ntcReading > DISCONNECTED_NTC ) {
    DEBUGLN("disconnected");
    return SNMEA2000::n2kDoubleNA;
  } 
  DEBUG(ntcReading);
  DEBUG(",");
  DEBUG(5.0*ntcReading/1024.0);
  DEBUG(",");
  int16_t temperature = interpolate(ntcReading, NMF5210K_MIN, NMF5210K_MAX, NMF5210K_STEP, tcurveNMF5210K, NMF5210K_LENGTH);
  DEBUGLN(temperature);
  return (0.1*temperature)+273.15;
}


/**
 *  convert a reading from a ntc into a temperature using a curve.
 */ 
// tested ok 20210909
int16_t EngineSensors::interpolate(
      int16_t reading, 
      int16_t minCurveValue, 
      int16_t maxCurveValue,
      int step, 
      const int16_t *curve, 
      int curveLength
      ) {
  int16_t cvp = ((int16_t)pgm_read_dword(&curve[0]));
  if ( reading > cvp ) {
    DEBUG(minCurveValue);
    DEBUG(",");
    return minCurveValue;
  }
  for (int i = 1; i < curveLength; i++) {
    int16_t cv = ((int16_t)pgm_read_dword(&curve[i]));
    if ( reading > cv ) {
      DEBUG(i);
      DEBUG(",");
      DEBUG(curve[i]);
      DEBUG(",");
      DEBUG(curve[i-1]);
      DEBUG(",");
      return minCurveValue+((i-1)*step)+((cvp-reading)*step)/(cvp-cv);
    }
    cvp = cv;
  }
  DEBUG(minCurveValue);
  DEBUG("^,");
  return maxCurveValue;
}





