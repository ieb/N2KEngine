
#include <Arduino.h>
#include "enginesensors.h"
#include "SmallNMEA2000.h"


#define RAPID_ENGINE_UPDATE_PERIOD 500
#define ENGINE_UPDATE_PERIOD 1000
#define VOLTAGE_UPDATE_PERIOD 999
#define FUEL_UPDATE_PERIOD 4900
#define TEMPERATURE_UPDATE_PERIOD 4850

#define ENGINE_INSTANCE 0
#define ENGINE_BATTERY_INSTANCE 0
// treating the alternator as a battery its possible to monitor temperature.
#define ALTERNATOR_BATTERY_INSTANCE 2
// Now using a dedicated shunt for service battery #define SERVICE_BATTERY_INSTANCE 2
#define FUEL_LEVEL_INSTANCE 0
#define FUEL_TYPE 0   // diesel


// ADC assignments
#define ADC_ALTERNATOR_VOLTAGE 0
#define ADC_FUEL_SENSOR 1
#define ADC_EXHAUST_NTC1 2 // NTC1
#define ADC_ALTERNATOR_NTC2 3 // NTC2
#define ADC_ENGINEROOM_NTC3 4
#define ADC_A2B_NTC4 5
#define ADC_COOLANT_TEMPERATURE 6
#define ADC_ENGINEBATTERY 7



// Digital pin assignments
// Flywheel pulse pin
#define PIN_FLYWHEEL 2
#define PIN_ONE_WIRE 3 // not used at present



const SNMEA2000ProductInfo productInfomation PROGMEM={
                                       1300,                        // N2kVersion
                                       44,                         // Manufacturer's product code
                                       "EMS",    // Manufacturer's Model ID
                                       "1.2.3.4 (2023-05-16)",     // Manufacturer's Software version code
                                       "5.6.7.8 (2017-06-11)",      // Manufacturer's Model version
                                       "0000002",                  // Manufacturer's Model serial code
                                       0,                           // SertificationLevel
                                       1                            // LoadEquivalency
};
const SNMEA2000ConfigInfo configInfo PROGMEM={
      "Engine Monitor",
      "Luna Technical Area",
      "https://github.com/ieb/EngineManagement"
};


const unsigned long txPGN[] PROGMEM = { 
    127488L, // Rapid engine ideally 0.1s
    127489L, // Dynamic engine 0.5s
    127505L, // Tank Level 2.5s
    130316L, // Extended Temperature 2.5s
    127508L,
  SNMEA200_DEFAULT_TX_PGN
};
const unsigned long rxPGN[] PROGMEM = { 
  SNMEA200_DEFAULT_RX_PGN
};

SNMEA2000DeviceInfo devInfo = SNMEA2000DeviceInfo(
  223,   // device serial number
  140,  // Engine
  50 // Propulsion
);


#define DEVICE_ADDRESS 24
#define SNMEA_SPI_CS_PIN 10

EngineMonitor engineMonitor = EngineMonitor(DEVICE_ADDRESS,
  &devInfo,
  &productInfomation, 
  &configInfo, 
  &txPGN[0], 
  &rxPGN[0],
  SNMEA_SPI_CS_PIN);

EngineSensors sensors(PIN_FLYWHEEL);

/**
 * Send engine rapid updates while the engine is running.
 */ 
void sendRapidEngineData() {
  static unsigned long lastRapidEngineUpdate=0;
  if ( sensors.isEngineRunning() ) {
    unsigned long now = millis();
    if ( now-lastRapidEngineUpdate > RAPID_ENGINE_UPDATE_PERIOD ) {
      lastRapidEngineUpdate = now;
      engineMonitor.sendRapidEngineDataMessage(ENGINE_INSTANCE, sensors.getEngineRPM());
    }
  }
}


/**
 * Send engine status while the engine is running.
 */ 
void sendEngineData() {
  static unsigned long lastEngineUpdate=0;
  if ( sensors.isEngineRunning() ) {
    unsigned long now = millis();
    if ( now-lastEngineUpdate > ENGINE_UPDATE_PERIOD ) {
      lastEngineUpdate = now;
      engineMonitor.sendEngineDynamicParamMessage(ENGINE_INSTANCE,
          sensors.getEngineSeconds(),
          sensors.getCoolantTemperatureK(ADC_COOLANT_TEMPERATURE, ADC_ENGINEBATTERY),
          sensors.getVoltage(ADC_ALTERNATOR_VOLTAGE),
          0, // status1
          0, // status2
          SNMEA2000::n2kDoubleNA, // engineOilPressure
          sensors.getTemperatureK(ADC_ALTERNATOR_NTC2) // alterator temperature as nengineOil temperature
          );
    }
  }
}

/**
 * Send voltages all the time.
 */ 
void sendVoltages() {
  static unsigned long lastVoltageUpdate=0;
  static byte sid = 0;
  unsigned long now = millis();
  if ( now-lastVoltageUpdate > VOLTAGE_UPDATE_PERIOD ) {
    lastVoltageUpdate = now;
    // because the engine monitor is not on all the time, the sid and instance ids of these messages has been shifted
    // to make space for sensors that are on all the time, and would be used by default
    // engineMonitor.sendDCBatterStatusMessage(SERVICE_BATTERY_INSTANCE, sid, sensors.getServiceBatteryVoltage());
    engineMonitor.sendDCBatterStatusMessage(ENGINE_BATTERY_INSTANCE, sid, sensors.getVoltage(ADC_ENGINEBATTERY));
    engineMonitor.sendDCBatterStatusMessage(ALTERNATOR_BATTERY_INSTANCE, sid, 
        sensors.getVoltage(ADC_ALTERNATOR_VOLTAGE),
        sensors.getTemperatureK(ADC_ALTERNATOR_NTC2)
        );
    sid++;
  }
}

/**
 * send Fuel all the time.
 */ 
void sendFuel() {
  static unsigned long lastFuelUpdate=0;
  unsigned long now = millis();
  if ( now-lastFuelUpdate > FUEL_UPDATE_PERIOD ) {
    lastFuelUpdate = now;
    engineMonitor.sendFluidLevelMessage(FUEL_TYPE, FUEL_LEVEL_INSTANCE, sensors.getFuelLevel(ADC_FUEL_SENSOR), sensors.getFuelCapacity());

  }
}

/**
 * send temperatures all the time.
 */ 
void sendTemperatures() {
  static unsigned long lastTempUpdate=0;
  static byte sid = 0;
  unsigned long now = millis();
  if ( now-lastTempUpdate > TEMPERATURE_UPDATE_PERIOD ) {
    lastTempUpdate = now;
    engineMonitor.sendTemperatureMessage(sid, 0, 14, sensors.getTemperatureK(ADC_EXHAUST_NTC1));
    engineMonitor.sendTemperatureMessage(sid, 1, 3, sensors.getTemperatureK(ADC_ENGINEROOM_NTC3));
    engineMonitor.sendTemperatureMessage(sid, 2, 15, sensors.getTemperatureK(ADC_ALTERNATOR_NTC2));
    sid++;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Luna engine monitor start"));
  while(!sensors.begin() ) {
    Serial.println(F("Failed to start Engine Sensors, retry in 5s"));
    delay(5000);
  }
  Serial.println(F("Opening CAN"));
  while (!engineMonitor.open(MCP_16MHz) ) {
     Serial.println(F("Failed to start NMEA2000, retry in 5s"));
    delay(5000);
  }
  Serial.println(F("Opened, MCP2515 Operational"));
  Serial.println(F("Running..."));;
}

void loop() {
  sensors.read();
  sendRapidEngineData();
  sendEngineData();
  sendVoltages();
  sendTemperatures();
  sendFuel();
  engineMonitor.processMessages();
}


