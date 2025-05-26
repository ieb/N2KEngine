#include "enginesensors.h"
#include <util/crc16.h>
#include <SmallNMEA2000.h>



#ifdef __AVR_TINY_2__

#define READ_ADC(x) analogReadEnh((x), 12)
#define CHECK_ADC(x) ((x) < 0)

#define ADC_RESOLUTION_SCALE 1
#define RESOLUTION_BITS 4096

#else

#define READ_ADC(x) analogRead((x))
#define CHECK_ADC(x) false
#define ADC_RESOLUTION_SCALE 4
#define RESOLUTION_BITS 1024
#endif

// common settings.
// Oil pressure.
    // 0psi == 0.5v
    // 100psi = 4.5v
    // 100ps == 4.5-0.5=4
    // scale == 100/4 = 25psi/V   
    // 1psi = 6894.76Pa 
    // scal in PA == 25*6894.76 = 172369
    // linear, no divider
    // 
#define PSIV_POWEROFF 0.2
#define PSIV_0 0.5
#define PSIV_100 4.5
#define SCALE_TO_PA 172369

    // Rtop = 1000
    // Rempty = 0
    // Rfull = 190
    // diode drop 0.63v
    // empty = 0v
    // full = (5-0.63)*190/(190+1000) =  0.697731092436975V
    // scale  100/0.697731092436975=143.3216909551

#define SCALE_FUEL_TO_PERCENT 143.3216909551

    //(147/47) = 3.1276595745
#define VOLTAGE_SCALE  3.1276595745




// using 

// same for 



/*

https://docs.google.com/spreadsheets/d/1cH6MjYFLKQYQMPswFqU3-U8cdn9rH_bujdkhxfRw2cY/edit#gid=1843834819

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


From https://docs.google.com/spreadsheets/d/1cH6MjYFLKQYQMPswFqU3-U8cdn9rH_bujdkhxfRw2cY/edit#gid=1843834819
Sheet VP Sensor                    
                    
Model   Volvo Penta Standard Coolant Sensor                 
R top   1000                    
Vsupply 12                  
                        
C   R   V   ADC ADC 4096    Current (mA)    Power (mW)
0   1743    7.625227853 1562    6247    4.374772147 33.35863443
10  1076    6.219653179 1274    5095    5.780346821 35.95175248
20  677 4.844364937 992 3969    7.155635063 34.6645076
30  439 3.660875608 750 2999    8.339124392 30.52849708
40  291 2.704879938 554 2216    9.295120062 25.14218378
50  197 1.974937343 404 1618    10.02506266 19.79887061
60  134 1.417989418 290 1162    10.58201058 15.00517903
70  97  1.061075661 217 869 10.93892434 11.60702637
80  70  0.785046729 161 643 11.21495327 8.804262381
90  51  0.582302569 119 477 11.41769743 6.648554546
100 38  0.4393063584    90  360 11.56069364 5.078686224
110 29  0.3381924198    69  277 11.66180758 3.943934925
120 22  0.2583170254    53  212 11.74168297 3.03307662
*/

// Thermistor lookups at 4096 resolution, multiply ADC by 4x for 1024 resolution.

const int16_t coolantTable[] PROGMEM= {
    5095,
    3969,
    2999,
    2216,
    1618,
    1162,
    869,
    643,
    477,
    360,
    277,
    212
};
#define COOLANT_TABLE_LENGTH 13
// in 0.1C as interpolation uses ints.
#define COOLANT_MIN_TEMPERATURE 100
#define COOLANT_MAX_TEMPERATURE 1200
#define COOLANT_STEP 100
#define COOLANT_SUPPLY_ADC_12V 3143 // (12*47/147)*4096/5= 3,143.05306122449
#define COOLANT_SUPPLY_ADC_5V 1310 // (5*47/147)*4096/5=1,309.6054421769


/*
https://docs.google.com/spreadsheets/d/1cH6MjYFLKQYQMPswFqU3-U8cdn9rH_bujdkhxfRw2cY/edit#gid=1843834819
Using https://en.wikipedia.org/wiki/Thermistor

Model   MT52-10K        Voltage 5           
B   3950        Top R   4700            
To  298.15      Max Current 1.063829787     mA  
Ro  10000       Max allowed power   50          
rinf    0.01763226979                       
C   K   R   V   ADC 1024    ADC 4096    current (mA)    Power (mW)
-20 253.15  105384.6902 4.786527991 980 3921    0.04541957642   0.2174020739
-15 258.15  77898.10758 4.71548985  966 3863    0.06053407453   0.285447814
-10 263.15  58245.71279 4.626662421 948 3790    0.07943352738   0.3675121161
-5  268.15  44026.04669 4.517711746 925 3701    0.1026145222    0.4635828322
0   273.15  33620.60372 4.386752877 898 3594    0.1304781114    0.5723752304
5   278.15  25924.56242 4.232642097 867 3467    0.1632676389    0.6910534817
10  283.15  20174.57702 4.055260317 831 3322    0.2010084431    0.8151415627
15  288.15  15837.14766 3.855732043 790 3159    0.2434612674    0.9387214101
20  293.15  12535.32581 3.636521279 745 2979    0.2901018556    1.054961571
25  298.15  10000   3.401360544 697 2786    0.3401360544    1.156925355
30  303.15  8037.140687 3.155001929 646 2585    0.3925527811    1.238504782
35  308.15  6505.531126 2.902821407 594 2378    0.4462082113    1.295262748
40  313.15  5301.466859 2.650344661 543 2171    0.4999266678    1.324977975
45  318.15  4348.13685  2.40278022  492 1968    0.5525999532    1.327776237
50  323.15  3588.182582 2.164637752 443 1773    0.6032685635    1.305857907
55  328.15  2978.435896 1.939480863 397 1589    0.6511742844    1.262940063
60  333.15  2486.164751 1.72982727  354 1417    0.6957814318    1.203581695
65  338.15  2086.372143 1.537177817 315 1259    0.7367706772    1.132547541
70  343.15  1759.837009 1.362137316 279 1116    0.7740133371    1.054312449
75  348.15  1491.682246 1.204585593 247 987 0.8075349802    0.9727450031
80  353.15  1270.320235 1.063862728 218 872 0.8374760152    0.8909595185
85  358.15  1086.670769 0.9389429714    192 769 0.8640546869    0.8112980752
90  363.15  933.5770578 0.828582842 170 679 0.8875355655    0.7353967412
95  368.15  805.3667326 0.7314378603    150 599 0.9082047106    0.6642953102
100 373.15  697.519773  0.6461484185    132 529 0.9263514003    0.5985604923
105 378.15  606.4157635 0.5713986526    117 468 0.9422556058    0.5384035836
110 383.15  529.1404013 0.5059535226    104 414 0.9561801016    0.4837826907
115 388.15  463.3365226 0.4486793767    92  368 0.9683660901    0.4344858937
120 393.15  407.0887766 0.398552673 82  326 0.9790313462    0.39019556
125 398.15  358.8338772 0.3546606648    73  291 0.9883700713    0.3505359866
130 403.15  317.290402  0.3161969675    65  259 0.9965538367    0.3151073011
135 408.15  281.403613  0.2824541383    58  231 1.003733162 0.2835085854
140 413.15  250.301877  0.2528147608    52  207 1.010039413 0.2553528725
145 418.15  223.2620904 0.2267420323    46  186 1.015586802 0.2302762154
*/

const int16_t tcurveNMF5210K[] PROGMEM= {
3921,
3863,
3790,
3701,
3594,
3467,
3322,
3159,
2979,
2786,
2585,
2378,
2171,
1968,
1773,
1589,
1417,
1259,
1116,
987,
872,
769,
679,
599,
529,
468,
414,
368,
326,
291,
259,
231,
207,
186
};
// in 0.1C steps.
#define NMF5210K_MIN -200
#define NMF5210K_MAX 1450
#define NMF5210K_STEP 50
#define NMF5210K_LENGTH 33
// ADC value that indicates a NTC is not connected.
#define DISCONNECTED_NTC 4090





extern void setupTimerFrequencyMeasurement(uint8_t flywheelPin);
extern void setupAdc();

bool EngineSensors::begin() {
  localStorage.loadEngineHours();
  localStorage.loadVdd();
  eepromWritten = true;

  setupTimerFrequencyMeasurement(flywheelPin);

  setupAdc();

  return true;
}

void EngineSensors::read(bool outputDebug) {
  unsigned long now = millis();
  if ( now-lastFlywheelReadTime > flywheelReadPeriod) {
    lastFlywheelReadTime = now;
    readEngineRPM(outputDebug);
    updateEngineStatus();
  }
  saveEngineHours();
}

void EngineSensors::setStoredVddVoltage(double measuredVddVoltage) {
  localStorage.setVdd(measuredVddVoltage);
  eepromWritten = true;
}

double EngineSensors::getStoredVddVoltage() {
  return localStorage.vdd;
}


void EngineSensors::setEngineSeconds(double seconds) {
  localStorage.engineHoursPeriods = seconds/15L;
  localStorage.saveEngineHours();
  eepromWritten = true;
}

double EngineSensors::getEngineSeconds() {  
  return 15L*localStorage.engineHoursPeriods; 
};


bool EngineSensors::isEngineRunning() { 
  return engineRunning; 
};

void EngineSensors::toggleFakeEngineRunning() {
  fakeEngineRunning = !fakeEngineRunning;
}

void EngineSensors::updateEngineStatus() {
  unsigned long now = millis();
  if ( engineRunning ) {
    if ( engineRPM < ENGINE_SHUTDOWN_RPM ) {
      engineRunning = false;
      canEmitAlarms = false;
      Serial.print(F("EngineStop"));
      SET_BIT(status2, ENGINE_STATUS2_ENGINE_SUTTING_DOWN);
    } else if ( !canEmitAlarms && now-engineStarted > ENGINE_START_GRACE_PERIOD ) {
      dumpEngineStatus1();
      dumpEngineStatus2();
      status1 = 0;
      status2 = 0;
      Serial.println(F("EngineStart: Grace period over"));
      canEmitAlarms = true;
    }
  } else {
    if ( engineRPM > 0 ) {
      canEmitAlarms = false;
      engineRunning = true;
      engineStarted = now;
      lastEngineHoursTick = now;
      Serial.println(F("EngineStartup...."));
    } else {
      CLEAR_BIT(status1, ENGINE_STATUS1_EMERGENCY_STOP);
      CLEAR_BIT(status2, ENGINE_STATUS2_ENGINE_SUTTING_DOWN);
    }
  }
}

void EngineSensors::saveEngineHours() {
  // Only count engine hours while the engine is running.
  // when the engine is running, increment the engineHoursPeriod by 1 every 15s
  if ( engineRunning ) {
    unsigned long now = millis();
    if ( now-lastEngineHoursTick > ENGINE_HOURS_PERIOD_MS ) {
      lastEngineHoursTick = now;
      localStorage.engineHoursPeriods++;

      localStorage.saveEngineHours();
      eepromWritten = true;
    }
  }
}






double EngineSensors::getEngineRPM() {
  return engineRPM;
}


       


double EngineSensors::getFuelCapacity() {
  return FUEL_CAPACITY;
}
 
// European Fuel sensor goes 0-190, 190 being full, 0 being empty.

double EngineSensors::getFuelLevel(uint8_t adc, bool outputDebug) { 
    // probably need a long to do this calc


    // powered by VDD so no can read relative to VDD.
    int16_t adcReading =  READ_ADC(adc);
    if ( CHECK_ADC((adcReading))) {
        if (outputDebug) {
          Serial.println(F("adc error"));
        }
        return SNMEA2000::n2kDoubleNA;
    } 

    double measuredVoltage = 5.0*((double)adcReading/(double)RESOLUTION_BITS);
    double fuelReading  = SCALE_FUEL_TO_PERCENT*(double)measuredVoltage;



    if (outputDebug) {
      Serial.print(F("Fuel tank adc:"));
      Serial.print(adcReading);
      Serial.print(F(" v:"));Serial.print(measuredVoltage);
      Serial.print(F(" %:"));Serial.print(fuelReading);
      Serial.println("");
    }    



    // the restances may be out of spec so deal with > 100 or < 0.
    if ( fuelReading > 200 ) {
      // sensor disconnected.
      return SNMEA2000::n2kDoubleNA;
    } else if (fuelReading > 100 ) {
      return 100.0;
    } else if ( fuelReading < 0) {
      return 0.0;
    } 
    return (double)fuelReading;
}

uint16_t EngineSensors::getEngineStatus1() {
  if ( canEmitAlarms ) {
    return status1;
  }
  return 0;
}

uint16_t EngineSensors::getEngineStatus2() {
  if ( canEmitAlarms ) {
    return status2;
  }
  return 0;
}

#define checkStatus(s,mask,msg)  if ( ((s)&(mask)) == (mask) ) Serial.print(msg)

void EngineSensors::dumpEngineStatus1() {
  Serial.print(F("Engine Status1: 0x"));
  Serial.print(status1,HEX);
  checkStatus(status1, ENGINE_STATUS1_CHECK_ENGINE,        F(" engineCheck"));
  checkStatus(status1, ENGINE_STATUS1_OVERTEMP,            F(" overTemp"));
  checkStatus(status1, ENGINE_STATUS1_LOW_OIL_PRES,        F(" lowOilPressure"));
  checkStatus(status1, ENGINE_STATUS1_LOW_OIL_LEVEL,       F(" lowOilLevel"));
  checkStatus(status1, ENGINE_STATUS1_LOW_SYSTEM_VOLTAGE,  F(" lowSystemVoltage"));
  checkStatus(status1, ENGINE_STATUS1_LOW_COOLANT_LEVEL,   F(" lowCoolantLevel"));
  checkStatus(status1, ENGINE_STATUS1_WATER_FLOW,          F(" lowWaterFlow"));
  checkStatus(status1, ENGINE_STATUS1_WATER_IN_FUEL,       F(" waterInFuel"));
  checkStatus(status1, ENGINE_STATUS1_CHARGE_INDICATOR,    F(" chargeIndicator"));
  checkStatus(status1, ENGINE_STATUS1_PREHEAT_INDICATOR,   F(" preheatIndicator"));
  checkStatus(status1, ENGINE_STATUS1_HIGH_BOOST_PRESSURE, F(" higBoostPressure"));
  checkStatus(status1, ENGINE_STATUS1_REV_LIMIT_EXCEEDED,  F(" revsExceeded"));
  checkStatus(status1, ENGINE_STATUS1_EGR_SYSTEM,          F(" egr"));
  checkStatus(status1, ENGINE_STATUS1_THROTTLE_POS_SENSOR, F(" throttlePos"));
  checkStatus(status1, ENGINE_STATUS1_EMERGENCY_STOP,      F(" emergnecyStop"));
  Serial.println("");
}
void EngineSensors::dumpEngineStatus2() {
  Serial.print(F("Engine Status2: 0x"));
  Serial.print(status2,HEX);
  checkStatus(status2, ENGINE_STATUS2_WARN_1,                    F(" warn1"));
  checkStatus(status2, ENGINE_STATUS2_WARN_2,                    F(" warn2"));
  checkStatus(status2, ENGINE_STATUS2_POWER_REDUCTION,           F(" powerDown"));
  checkStatus(status2, ENGINE_STATUS2_MAINTANENCE_NEEDED,        F(" maintNeeded"));
  checkStatus(status2, ENGINE_STATUS2_ENGINE_COMM_ERROR,         F(" comError"));
  checkStatus(status2, ENGINE_STATUS2_SUB_OR_SECONDARY_THROTTLE, F(" secondarThrottle"));
  checkStatus(status2, ENGINE_STATUS2_NEUTRAL_START_PROTECT,     F(" neutralStart"));
  checkStatus(status2, ENGINE_STATUS2_ENGINE_SUTTING_DOWN,       F(" shuttingDown"));
  Serial.println("");
}



// in Pascal
double EngineSensors::getOilPressure(uint8_t adc, bool outputDebug) {
    int16_t adcReading =  READ_ADC(adc);
    if ( CHECK_ADC((adcReading))) {
        if (outputDebug) {
          Serial.println(F("adc error"));
        }
        return SNMEA2000::n2kDoubleNA;
    } 
    double measuredVoltage = localStorage.vdd*((double)adcReading/(double)RESOLUTION_BITS);
    double oilPressureReading = (measuredVoltage-PSIV_0)*SCALE_TO_PA;

    if (outputDebug) {
      Serial.print(F("Oil Pressure  "));
      Serial.print(adcReading);
      Serial.print(F(" v:"));Serial.print(measuredVoltage);
      Serial.print(F(" pa:"));Serial.print(oilPressureReading);
      Serial.print(F(" psi:"));Serial.print(oilPressureReading/6894.76);
      Serial.println(F(" pa:"));Serial.print(oilPressureReading);      
    }
    
    if ( oilPressureReading < PSIV_POWEROFF ) {
      // disconnected or not powered up
      CLEAR_BIT(status1, ENGINE_STATUS1_LOW_OIL_PRES);
      if ( engineRPM > MIN_ENGINE_RUNNING_RPM ) {
        SET_BIT(status2, ENGINE_STATUS2_ENGINE_COMM_ERROR);
      } else {
        CLEAR_BIT(status2, ENGINE_STATUS2_ENGINE_COMM_ERROR);
      }
      return SNMEA2000::n2kDoubleNA;
    }
    CLEAR_BIT(status1, ENGINE_STATUS1_LOW_OIL_PRES);
    CLEAR_BIT(status2, ENGINE_STATUS2_ENGINE_COMM_ERROR);
    // need the real voltage so must scale to take account of VDD.
    if ( oilPressureReading < 0 ) {
      oilPressureReading = 0;
    }
    if (oilPressureReading < MIN_OIL_PRESSURE && engineRPM > MIN_ENGINE_RUNNING_RPM ) { // 10psi
      SET_BIT(status1, ENGINE_STATUS1_LOW_OIL_PRES | ENGINE_STATUS1_CHECK_ENGINE);
      SET_BIT(status2, ENGINE_STATUS2_MAINTANENCE_NEEDED );
    } else {
      CLEAR_BIT(status1, ENGINE_STATUS1_LOW_OIL_PRES);
    }
    return oilPressureReading;

}


double EngineSensors::getCoolantTemperatureK(uint8_t coolantAdc, uint8_t batteryAdc, bool outputDebug) {
  // probably need a long to do this calc
    // need 32 bits, 1024*1024 = 1M
    if (outputDebug) {
      Serial.print(F("Coolant:"));
    }
    // TODO 

    // powered by 12v so need to read 12v and scale
    int16_t coolantSupply =  READ_ADC(batteryAdc);
    if ( CHECK_ADC((coolantSupply))) { 
        if (outputDebug) {
          Serial.println(F("adc error"));
        } 
        return SNMEA2000::n2kDoubleNA; 
    } 

    int16_t coolantReading =  READ_ADC(coolantAdc);
    if ( CHECK_ADC((coolantReading))) {
        if (outputDebug) {
          Serial.println(F("adc error"));
        } 
        return SNMEA2000::n2kDoubleNA;
    } 

    // scale to 4096 bits if not already scaled.
    coolantReading = coolantReading * ADC_RESOLUTION_SCALE;
    coolantSupply = coolantSupply * ADC_RESOLUTION_SCALE;




    if ( coolantSupply < COOLANT_SUPPLY_ADC_5V) {
      if (outputDebug) {
        Serial.println(F("no power"));
      } 
      return SNMEA2000::n2kDoubleNA;
    }
    // both will be scaled by VDD errors and so those errors cancel out
    // however any difference in the supply (12v) needs to be takne into account.
    coolantReading = coolantReading *  ((double)COOLANT_SUPPLY_ADC_12V/(double)coolantSupply);

    int16_t coolantTemperature = interpolate(coolantReading,COOLANT_MIN_TEMPERATURE, COOLANT_MAX_TEMPERATURE, COOLANT_STEP, coolantTable, COOLANT_TABLE_LENGTH);

    if (outputDebug) {
      Serial.print(F("Coolant Temperature adc:"));
      Serial.print(F(" supply scaling:"));Serial.print(((double)COOLANT_SUPPLY_ADC_12V/(double)coolantSupply));
      Serial.print(F(" cooltantReading:"));Serial.print(coolantReading);
      Serial.print(F(" 0.1C:"));Serial.print(coolantTemperature);
      Serial.print(F(" K:"));Serial.println((0.1*coolantTemperature)+273.15);      
    }

    // 98C, may want to make this a setting ?
    if ( coolantTemperature > MAX_COOLANT_TEMP) {
      SET_BIT(status1, ENGINE_STATUS1_OVERTEMP | ENGINE_STATUS1_CHECK_ENGINE);
      SET_BIT(status2, ENGINE_STATUS2_MAINTANENCE_NEEDED);
    } else {
      CLEAR_BIT(status1, ENGINE_STATUS1_OVERTEMP);
    }
    return (0.1*coolantTemperature)+273.15; 

}

void EngineSensors::dumpADC(uint8_t adc) { 
  int16_t adcReading =  READ_ADC(adc);
  if ( CHECK_ADC((adcReading))) {
      Serial.print(F("adc:"));
      Serial.print(adc);
      Serial.print(F(" error:"));
      Serial.println(adcReading);
      return;
  } 

  Serial.print(F("adc:"));
  Serial.print(adc);
  Serial.print(F(" raw:"));
  Serial.print(adcReading);
  Serial.print(F(" Vdd:"));
  Serial.print(localStorage.vdd, 5);
  double fraction = (double)adcReading/RESOLUTION_BITS;
  double voltage = localStorage.vdd*fraction;
  Serial.print(F(" fraction:"));
  Serial.print(fraction, 5);
  Serial.print(F(" V:"));
  Serial.println(voltage, 5);
}



double EngineSensors::getVoltage(uint8_t adc, bool outputDebug) { 

  int16_t adcReading =  READ_ADC(adc);
  if ( CHECK_ADC((adcReading))) {
      if (outputDebug) {
        Serial.println(F("adc error"));
      } 
      return SNMEA2000::n2kDoubleNA;
  } 

  double voltage = (double)VOLTAGE_SCALE*localStorage.vdd*((double)adcReading/(double)RESOLUTION_BITS);
  if (outputDebug) {
    Serial.print(F("Voltage n:"));
    Serial.print(adc);
    Serial.print(F(" adc:"));Serial.print(adcReading);
    Serial.print(F(" V:"));Serial.println(voltage);    
  }
  if ( adc == adcAlternatorVoltage ) {
    if ( voltage < LOW_ALTERNATOR_VOLTAGE && engineRPM > MIN_ENGINE_RUNNING_RPM ) {
      SET_BIT(status1, ENGINE_STATUS1_CHARGE_INDICATOR);
    } else {
      CLEAR_BIT(status1, ENGINE_STATUS1_CHARGE_INDICATOR);
    }
  } else if ( adc == adcEngineBattery) {
    if ( voltage < LOW_BATTERY_VOLTAGE && engineRPM > MIN_ENGINE_RUNNING_RPM) {
      SET_BIT(status1, ENGINE_STATUS1_LOW_SYSTEM_VOLTAGE);
    } else {
      CLEAR_BIT(status1, ENGINE_STATUS1_LOW_SYSTEM_VOLTAGE);
    }
  }
  return voltage; 
};


// tested ok 20210909
double EngineSensors::getTemperatureK(uint8_t adc, bool outputDebug) {
  // probably need a long to do this calc


  // The ntcReading is relative to VDD which also supplies the NTC, so no scaling required.
  int16_t ntcReading =  READ_ADC(adc);
  if ( CHECK_ADC((ntcReading))) {
      if (outputDebug) {
        Serial.println(F("adc error"));
      } 
      return SNMEA2000::n2kDoubleNA;
  } 

  // scale to 4096 bits if not already scaled.
  ntcReading = ntcReading * ADC_RESOLUTION_SCALE;
  

  if ( ntcReading > DISCONNECTED_NTC ) {
    if (outputDebug) {
      Serial.println("NTC disconnected");
    }
    return SNMEA2000::n2kDoubleNA;
  } 

  int16_t temperature = interpolate(ntcReading, NMF5210K_MIN, NMF5210K_MAX, NMF5210K_STEP, tcurveNMF5210K, NMF5210K_LENGTH);

  if (outputDebug) {
    Serial.print(F("Temperature adcn:"));
    Serial.print(adc);
    Serial.print(F(" adc:"));Serial.print(ntcReading);
    Serial.print(F(" 0.1C:"));Serial.print(temperature);
    Serial.print(F(" K:"));Serial.println((0.1*temperature)+273.15);    
  }

  // temperatures are in 0.1C
  // 80C
  if ( adc == adcExhaustNTC1 ) {
    if ( temperature > MAX_EXHAUST_TEMP) {
      SET_BIT(status1, ENGINE_STATUS1_WATER_FLOW | ENGINE_STATUS1_CHECK_ENGINE | ENGINE_STATUS1_EMERGENCY_STOP);
      SET_BIT(status2, ENGINE_STATUS2_MAINTANENCE_NEEDED);
    } else if ( temperature < CLEAR_EXHAUST_TEMP) {
      CLEAR_BIT(status1, ENGINE_STATUS1_WATER_FLOW);
    }
  } else if ( adcAlternatorNTC2) {
    // 100C
    if ( temperature > MAX_ALTERNATOR_TEMP) {
      SET_BIT(status1, ENGINE_STATUS1_OVERTEMP | ENGINE_STATUS1_CHECK_ENGINE | ENGINE_STATUS1_EMERGENCY_STOP);
      SET_BIT(status1, ENGINE_STATUS2_WARN_2);
    } else if ( temperature < CLEAR_ALTERLATOR_TEMP ) {
      CLEAR_BIT(status1, ENGINE_STATUS2_WARN_2);
    }
  } else if ( adcEngineRoomNTC3 ) {
    // 70C
    if ( temperature > MAX_ENGINE_ROOM_TEMP) {
      SET_BIT(status1, ENGINE_STATUS1_OVERTEMP | ENGINE_STATUS1_CHECK_ENGINE | ENGINE_STATUS1_EMERGENCY_STOP);
      SET_BIT(status1, ENGINE_STATUS2_WARN_2);
    } else if ( temperature < CLEAR_ENGINE_ROOM_TEMP ) {
      CLEAR_BIT(status1, ENGINE_STATUS2_WARN_2);
    }
  }
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
    return minCurveValue;
  }
  for (int i = 1; i < curveLength; i++) {
    int16_t cv = ((int16_t)pgm_read_dword(&curve[i]));
    if ( reading > cv ) {
      return minCurveValue+((i-1)*step)+((cvp-reading)*step)/(cvp-cv);
    }
    cvp = cv;
  }
  return maxCurveValue;
}





