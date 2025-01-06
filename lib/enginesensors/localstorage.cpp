#include <Arduino.h>
#include "enginesensors.h"
#include <util/crc16.h>
#include <EEPROM.h>

/*
 EEPROM structure, all Little endian encoded
  unit16_t crc16
  unit32_t engineHoursPeriods
  uint16_t adcScaleFactor  

  adcScale =  adcScaleFactor/40000.0;

  adcScale is the factor by which an adc reading is multiplied by to correct for a supply VDD supply 
  voltage not == 5v.  If the supply voltage is < 5v, this is > 1.0 (and visa versa).

  adcScale is only required where reading an absolute voltage. For voltages powered by vdd, or references to
  and alternative supply measured at the same time, the correction cancels out.
*/


#define EEPROM_CRC 0
#define EEPROM_ENGINE_HOURS 2
#define EEPROM_VDD_SCALE 6
#define EEPROM_LEN 8




void LocalStorage::loadVdd() {
  uint16_t storedVdd = 46700;
  if ( eepromValid() ) {
    storedVdd = EEPROM.read(EEPROM_VDD_SCALE) | EEPROM.read(EEPROM_VDD_SCALE+1)<<8; 
  }
  vdd = (double)(storedVdd)/10000.0;
  Serial.print(F("Loaded VDD: "));
  Serial.print(storedVdd);
  Serial.print(F(" V: "));
  Serial.println(vdd, 6);
}




void LocalStorage::setVdd(double vdd) {
  uint16_t storedVdd = (vdd*10000);
  EEPROM.write(EEPROM_VDD_SCALE, storedVdd&0xff);
  EEPROM.write(EEPROM_VDD_SCALE+1, (storedVdd>>8)&0xff);
  updateCRC();
  this->vdd = vdd;
  Serial.print(F("Set VDD: "));
  Serial.print(storedVdd);
  Serial.print(F(" V: "));
  Serial.println(vdd, 5);
}





void LocalStorage::loadEngineHours() {
  if ( eepromValid() ) {
    engineHoursPeriods = EEPROM.read(EEPROM_ENGINE_HOURS+3);
    engineHoursPeriods = engineHoursPeriods<<8;
    engineHoursPeriods = engineHoursPeriods | EEPROM.read(EEPROM_ENGINE_HOURS+2);
    engineHoursPeriods = engineHoursPeriods<<8;
    engineHoursPeriods = engineHoursPeriods | EEPROM.read(EEPROM_ENGINE_HOURS+1);
    engineHoursPeriods = engineHoursPeriods<<8;
    engineHoursPeriods = engineHoursPeriods | EEPROM.read(EEPROM_ENGINE_HOURS);
  } else {
    engineHoursPeriods = 0;
  }
  Serial.print(F("Hours: "));
  Serial.println(0.004166666667*engineHoursPeriods);
}

void LocalStorage::saveEngineHours() {
  EEPROM.write(EEPROM_ENGINE_HOURS, engineHoursPeriods&0xff);
  EEPROM.write(EEPROM_ENGINE_HOURS+1, (engineHoursPeriods>>8)&0xff);
  EEPROM.write(EEPROM_ENGINE_HOURS+2, (engineHoursPeriods>>16)&0xff);
  EEPROM.write(EEPROM_ENGINE_HOURS+3, (engineHoursPeriods>>24)&0xff);
  updateCRC();
}


void LocalStorage::updateCRC() {
  uint16_t crc = 0;
  for (int i = EEPROM_CRC+2; i < EEPROM_LEN; i++) {    
    crc = _crc16_update(crc, EEPROM.read(i));
  }
  EEPROM.write(EEPROM_CRC, crc&0xff);
  EEPROM.write(EEPROM_CRC+1, (crc>>8)&0xff);
}

bool LocalStorage::eepromValid() {
  uint16_t crc = 0;
  for (int i = 2; i < EEPROM_LEN; i++) {    
    crc = _crc16_update(crc, EEPROM.read(i));
  }
  uint16_t storedCrc = EEPROM.read(0) | (EEPROM.read(1)<<8);
  return (storedCrc == crc);
}

