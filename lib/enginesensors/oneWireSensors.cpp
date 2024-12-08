
#include "oneWireSensors.h"
#include <SmallNMEA2000.h>

//#define DEBUGON 1

#ifdef DEBUGON
#define DEBUG(x) Serial.print(x)
#define DEBUGLN(x) Serial.println(x)
#define DEBUGHEX(x) Serial.print(x,HEX)
#define INFO(x)  Serial.print(x)
#define INFOLN(x) Serial.println(x)
#else
#define DEBUG(x)
#define DEBUGLN(x)
#define DEBUGHEX(x)
#define INFO(x)
#define INFOLN(x)
#endif


#define REQUEST_TEMPERATURES 28000
#define READ_TEMPERATURES    30000


void OneWireSensors::begin() {
  tempSensors.begin();
  tempSensors.setWaitForConversion(false);
  for(int i = 0; i < MAX_ONE_WIRE_SENSORS; i++) {
    temperature[i] = SNMEA2000::n2kDoubleNA;
    if (tempSensors.getAddress(tempDevices[i], i)) {
      Serial.print(F("Temp Sensor "));
      Serial.print(i);
      Serial.print(" ");
      for (int j = 0; j < 8; j++) {
        Serial.print(tempDevices[i][j],HEX);
      }
      Serial.println(" ");
      maxActiveDevice = i+1;
    } else {
      Serial.print(F("No Temp sensor "));
      Serial.println(i);
    }
  }
  for (int i = 0; i < maxActiveDevice; i++) {
      tempSensors.requestTemperaturesByAddress(tempDevices[i]);
  }   
  requestTemperaturesRequired = false;
  lastReadTime = millis() - (READ_TEMPERATURES + 1000);
}




void OneWireSensors::readOneWire() {
  unsigned long now = millis();
  if ( requestTemperaturesRequired && now-lastReadTime > REQUEST_TEMPERATURES ) {
    for (int i = 0; i < maxActiveDevice; i++) {
      tempSensors.requestTemperaturesByAddress(tempDevices[i]);
    }   
    requestTemperaturesRequired = false;
  }
  if ( !requestTemperaturesRequired && now-lastReadTime > READ_TEMPERATURES ) {
    // 1 wire sensors
    for (int i = 0; i < maxActiveDevice; i++) {

      double t  = tempSensors.getTempC(tempDevices[i]);
      DEBUG(F("Temp i="));
      DEBUG(i);
      DEBUG(F(" t="));
      DEBUGLN(t);

      if ( temperature[i] == SNMEA2000::n2kDoubleNA ) {
        if ( t != 85.0) {
            temperature[i] = t;
        }
      } else {
        temperature[i] = t;
      }
    }
    requestTemperaturesRequired = true;
    lastReadTime = now;
  }
}

uint8_t OneWireSensors::getMaxActiveDevice() {
    return maxActiveDevice;
}


double OneWireSensors::getTemperature(uint8_t  n) {
    if ( n < maxActiveDevice ) {
        return temperature[n];
    }
    return SNMEA2000::n2kDoubleNA;
}
