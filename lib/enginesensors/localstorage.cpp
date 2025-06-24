#include <Arduino.h>
#include "enginesensors.h"
#include <util/crc16.h>
#include <EEPROM.h>

/*
 EEPROM structure, all Little endian encoded, organised into blocks each with its own CRC to allow 
 each block to be reset independently

 Block 1 config and long term settings
 8 bytes, starting at offset 0

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

/**
 * 
 * block events
 * starts eprom offset 8
 * 
 *  engineHoursPeriods is a uint32_t, which has a maximum range of  17M hours (0.004166666667*2^32=17M), obviously
 *  this was being stored this way for simplicity, 2^16 would not have been enough 0.004166666667*2^16=276h, but 2^24 
 *  is plenty 0.004166666667*2^ = 69000h for saving the engine hours it only saved 1 byte, for the events we should
 *  pack the time and events better. We have 128 bytes of eeprom in an attny3226, 120 for events. 2 byte for crc
 *  so 118 for events, each event needs 4 bytes 118/4 allows us to store the last 29 events seen. 
 *  the event counteers are always sequential so we can always find the next free slot by taking the lowest value
 *  the store should be treated as n unordered list.
 * 
 * each event is 
 *   uint24_t eventPeriods
 *   uint24_t eventType
 * 
 */


#define EVENTS_CRC 8
#define EVENTS_START 10
#define EVENTS_LEN 126




void LocalStorage::loadVdd() {
  uint16_t storedVdd = 46700;
  if ( eepromBlockValid(EEPROM_CRC, EEPROM_LEN) ) {
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
  EEPROM.update(EEPROM_VDD_SCALE, storedVdd&0xff);
  EEPROM.update(EEPROM_VDD_SCALE+1, (storedVdd>>8)&0xff);
  updateBlockCRC(EEPROM_CRC, EEPROM_LEN);
  this->vdd = vdd;
  Serial.print(F("Set VDD: "));
  Serial.print(storedVdd);
  Serial.print(F(" V: "));
  Serial.println(vdd, 5);
}





void LocalStorage::loadEngineHours() {
  if ( eepromBlockValid(EEPROM_CRC, EEPROM_LEN) ) {
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
  EEPROM.update(EEPROM_ENGINE_HOURS, engineHoursPeriods&0xff);
  EEPROM.update(EEPROM_ENGINE_HOURS+1, (engineHoursPeriods>>8)&0xff);
  EEPROM.update(EEPROM_ENGINE_HOURS+2, (engineHoursPeriods>>16)&0xff);
  EEPROM.update(EEPROM_ENGINE_HOURS+3, (engineHoursPeriods>>24)&0xff);
  updateBlockCRC(EEPROM_CRC, EEPROM_LEN);
}


/**
 * clear event memory
 */ 
void LocalStorage::clearEvents() {
  for (int i = EVENTS_START; i < EVENTS_LEN; ++i) {
      EEPROM.update(i, 0x00);
  }
  updateBlockCRC(EVENTS_CRC, EVENTS_LEN);
}


/**
 * Record the event identifid by the event ID, overwritting the oldest event if no free space.
 */
void LocalStorage::saveEvent(uint8_t eventId) {
  // scan through the events block, saving at the first slot with a 0 entry, failing that the lowest entry
  uint32_t minEventTime = 0xffffff;
  uint8_t useSlot = EVENTS_START-1;
  for (int i = EVENTS_START; i < EVENTS_LEN; i=i+4) {
    uint32_t eventTime = EEPROM.read(i+2);
    eventTime = eventTime<<8;
    eventTime = eventTime | EEPROM.read(i+1);
    eventTime = eventTime<<8;
    eventTime = eventTime | EEPROM.read(i);
    if ( eventTime == 0) {
      useSlot = i;
      break;
    } else if ( eventTime < minEventTime ) {
      minEventTime = eventTime;
      useSlot = i;
    }
  }
  if ( useSlot >= EVENTS_START ) {
    // unused slot save here
    EEPROM.update(useSlot, engineHoursPeriods&0xff);
    EEPROM.update(useSlot+1, (engineHoursPeriods>>8)&0xff);
    EEPROM.update(useSlot+2, (engineHoursPeriods>>16)&0xff);
    EEPROM.update(useSlot+3, eventId);
    updateBlockCRC(EVENTS_CRC, EVENTS_LEN);    
  }
}

/**
 * Find the next event after the last event, updating the last event
 * The last event will become 0xffffff when there are no more events availabl
 * Should be called with lastEvent set to 0x00 to start with.
 */
uint8_t LocalStorage::nextEvent(uint32_t *lastEvent) {
  // find the next event, updating lastEvent with the new event
  uint8_t eventId = EVENTS_NO_EVENT;
  uint32_t minEventTime = 0xffffff;
  for (int i = EVENTS_START; i < EVENTS_LEN; i=i+4) {
    uint8_t e = EEPROM.read(i+3);
    if ( e != EVENTS_NO_EVENT ) {
      uint32_t eventTime = EEPROM.read(i+2);
      eventTime = eventTime<<8;
      eventTime = eventTime | EEPROM.read(i+1);
      eventTime = eventTime<<8;
      eventTime = eventTime | EEPROM.read(i); 
      if ( eventTime > *lastEvent && eventTime < minEventTime) {
        minEventTime = eventTime;
        eventId = e;
      }     
    }
  }
  *lastEvent = minEventTime;
  return eventId;
}

uint8_t LocalStorage::countEvents() {
  // find the next event, updating lastEvent with the new event
  uint8_t nevents = 0;
  for (int i = EVENTS_START; i < EVENTS_LEN; i=i+4) {
    uint8_t eventId = EEPROM.read(i+3);
    if ( eventId != EVENTS_NO_EVENT ) {
      uint32_t eventTime = EEPROM.read(i+2);
      eventTime = eventTime<<8;
      eventTime = eventTime | EEPROM.read(i+1);
      eventTime = eventTime<<8;
      eventTime = eventTime | EEPROM.read(i); 
      if ( eventTime > 0) {
        nevents++;
      }
    }
  }
  return nevents;
}



void LocalStorage::updateBlockCRC(uint8_t crc_offset, uint8_t block_len) {
  uint16_t crc = 0;
  for (int i = crc_offset+2; i < block_len; i++) {    
    crc = _crc16_update(crc, EEPROM.read(i));
  }
  EEPROM.update(crc_offset, crc&0xff);
  EEPROM.update(crc_offset+1, (crc>>8)&0xff);
}

bool LocalStorage::eepromBlockValid(uint8_t crc_offset, uint8_t block_len) {
  uint16_t crc = 0;
  for (int i = crc_offset+2; i < block_len; i++) {    
    crc = _crc16_update(crc, EEPROM.read(i));
  }
  uint16_t storedCrc = EEPROM.read(crc_offset) | (EEPROM.read(crc_offset+1)<<8);
  return (storedCrc == crc);
}

