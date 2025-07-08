
#include "version.h"
#include <Arduino.h>
#include "enginesensors.h"
#include "SmallNMEA2000.h"
#include <MemoryFree.h>
#ifndef INSPECT_FLASH_USAGE
#include "oneWireSensors.h"
#endif

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

#ifdef __AVR_TINY_2__

#define ADC_ALTERNATOR_VOLTAGE PIN_PA5
#define ADC_FUEL_SENSOR PIN_PA6
#define ADC_EXHAUST_NTC1 PIN_PA7 // NTC1
#define ADC_ALTERNATOR_NTC2 PIN_PB5 // NTC2
#define ADC_ENGINEROOM_NTC3 PIN_PB4
#define ADC_OIL_SENSOR PIN_PB1
#define ADC_COOLANT_TEMPERATURE PIN_PA4
#define ADC_ENGINEBATTERY PIN_PB0


#define PIN_FLYWHEEL PIN_PC2
#define PIN_ONE_WIRE ONE_WIRE_PIN 

#define SNMEA_SPI_M0SI PIN_PA1 // Default pin
#define SNMEA_SPI_MISO PIN_PA2 // Default Pin
#define SNMEA_SPI_SCK  PIN_PA3 // Default pin
#define SNMEA_SPI_CS_PIN  PIN_PC3 // not default, set in constructor
#define LED_PIN PIN_PC0

#else

#define ADC_ALTERNATOR_VOLTAGE 0
#define ADC_FUEL_SENSOR 1
#define ADC_EXHAUST_NTC1 2 // NTC1
#define ADC_ALTERNATOR_NTC2 3 // NTC2
#define ADC_ENGINEROOM_NTC3 4
#define ADC_OIL_SENSOR 5
#define ADC_COOLANT_TEMPERATURE 6
#define ADC_ENGINEBATTERY 7
// Digital pin assignments
// Flywheel pulse pin
#define PIN_FLYWHEEL 2
#define PIN_ONE_WIRE 3 // not used at present


// defaults
#define SNMEA_SPI_M0SI 11 
#define SNMEA_SPI_MISO 12 
#define SNMEA_SPI_SCK  13 
#define SNMEA_SPI_CS_PIN 10
#endif






#define DEVICE_ADDRESS 24

#define ENGINE_PROPRIETARY_PGN  65305L
#define ENGINE_PROPRIETARY_FP_PGN  130817L
#define ENGINE_PROPRIETARY_CODE 0x9ffe // 2046 & 0x7FE | 0x3<<11 | 0x04<<13
#define FN_DUMP_EVENTS 11
#define FN_DUMP_EVENTS_RESP 12
#define FN_CLEAR_EVENTS 13
#define FN_CLEAR_EVENTS_RESP 14


bool sensorDebug = false;
bool monitorEnabled = false;

EngineSensors sensors(PIN_FLYWHEEL, 
    ADC_ALTERNATOR_VOLTAGE,
    ADC_ENGINEBATTERY,
    ADC_EXHAUST_NTC1,
    ADC_ALTERNATOR_NTC2,
    ADC_ENGINEROOM_NTC3);


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


const unsigned long txPGN[] = { 
    127488L, // Rapid engine ideally 0.1s
    127489L, // Dynamic engine 0.5s
    127505L, // Tank Level 2.5s
    130316L, // Extended Temperature 2.5s
    127508L,
  SNMEA200_DEFAULT_TX_PGN
};

const unsigned long rxPGN[] = { 
  ENGINE_PROPRIETARY_PGN,
  ENGINE_PROPRIETARY_FP_PGN,
  SNMEA200_DEFAULT_RX_PGN
};

SNMEA2000DeviceInfo devInfo = SNMEA2000DeviceInfo(
  223,   // device serial number
  140,  // Engine
  50 // Propulsion
);



EngineMonitor engineMonitor = EngineMonitor(DEVICE_ADDRESS,
  &devInfo,
  &productInfomation, 
  &configInfo, 
  &txPGN[0],
  SNMEA200_DEFAULT_TX_PGN_LEN+5,
  &rxPGN[0],
  SNMEA200_DEFAULT_RX_PGN_LEN+2,
  SNMEA_SPI_CS_PIN);

#ifndef INSPECT_FLASH_USAGE
OneWire oneWire;
OneWireSensors oneWireSensor(oneWire);
#endif

#ifdef LED_PIN
void toggleLed() {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}
void asyncBlink() {
  static unsigned long lastToggle=0;
  unsigned long now = millis();
  if ( now-lastToggle > 5000 ) {
    lastToggle = now;
    toggleLed();
  }
}

void setupLed() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}
void blinkLed(int n) {
  for (int i = 0; i < n; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);    
  }
}


#else
void setupLed(){};
void asyncBlink(){};
void toggleLed(){};
void blinkLed(int n){};
#endif



/**
 * Send engine rapid updates while the engine is running.
 */ 
void sendRapidEngineData() {
  static unsigned long lastRapidEngineUpdate=0;
  if ( sensors.isEngineRunning() ) {
    unsigned long now = millis();
    if ( now-lastRapidEngineUpdate > RAPID_ENGINE_UPDATE_PERIOD ) {
      lastRapidEngineUpdate = now;
      toggleLed();
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
      toggleLed();
      double engineSeconds = sensors.getEngineSeconds();
      double coolantTemperature = sensors.getCoolantTemperatureK(ADC_COOLANT_TEMPERATURE, ADC_ENGINEBATTERY);
      double alternatorVoltage = sensors.getVoltage(ADC_ALTERNATOR_VOLTAGE);
      double oilPressure = sensors.getOilPressure(ADC_OIL_SENSOR);
      double alternatorTemperature = sensors.getTemperatureK(ADC_ALTERNATOR_NTC2);
      uint16_t status1 = sensors.getEngineStatus1();
      uint16_t status2 = sensors.getEngineStatus2();
      if (status1 != 0) {
        sensors.dumpEngineStatus1();
      }
      if (status2 != 0) {
        sensors.dumpEngineStatus2();
      }
      engineMonitor.sendEngineDynamicParamMessage(ENGINE_INSTANCE,
          engineSeconds,
          coolantTemperature,
          alternatorVoltage,
          status1, // status1
          status2, // status2
          oilPressure, // engineOilPressure
          alternatorTemperature // alterator temperature as engineOil temperature, more important with LiFeP04
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
      toggleLed();
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
      toggleLed();
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
      toggleLed();
    // this may need adjusting depending on what the instruments can display
    engineMonitor.sendTemperatureMessage(sid, 0, 14, sensors.getTemperatureK(ADC_EXHAUST_NTC1));
    engineMonitor.sendTemperatureMessage(sid, 0, 3, sensors.getTemperatureK(ADC_ENGINEROOM_NTC3));
    // custom temperatures
    // temperature source can be 0-255, 0-15 are defined.
    engineMonitor.sendTemperatureMessage(sid, 0, 30, sensors.getTemperatureK(ADC_ALTERNATOR_NTC2));
#ifndef INSPECT_FLASH_USAGE
    uint8_t maxActiveDevices = oneWireSensor.getMaxActiveDevice();
    for (int i = 0; i < maxActiveDevices; i++) {
      engineMonitor.sendTemperatureMessage(sid, 0, 31+i, oneWireSensor.getTemperatureK(i));
    }
#endif
    sid++;
  }
}


void printN2K(double v, double fact, double offset, const char * term="\n") {
  if ( v == SNMEA2000::n2kDoubleNA) {
    Serial.print(F("--"));
  } else {
    Serial.print((v*fact)-offset);
  }
  Serial.print(term);
}


// uses 1.1KB
void showStatus() {
  Serial.print(F("CPU Vdd   : "));Serial.println(sensors.getStoredVddVoltage());
  Serial.print(F("Free mem  : "));Serial.println(freeMemory());
  Serial.print(F("Exhaust T : "));printN2K(sensors.getTemperatureK(ADC_EXHAUST_NTC1, sensorDebug), 1.0,273.15);
  Serial.print(F("Alt T     : "));printN2K(sensors.getTemperatureK(ADC_ALTERNATOR_NTC2, sensorDebug),1.0, 273.15);
  Serial.print(F("Room T    : "));printN2K(sensors.getTemperatureK(ADC_ENGINEROOM_NTC3, sensorDebug),1.0,273.15);
  Serial.print(F("Fuel      : "));printN2K(sensors.getFuelLevel(ADC_FUEL_SENSOR, sensorDebug),1.0,0);
  Serial.print(F("Engine V  : "));printN2K(sensors.getVoltage(ADC_ENGINEBATTERY, sensorDebug),1.0,0);
  Serial.print(F("Alt V     : "));printN2K(sensors.getVoltage(ADC_ALTERNATOR_VOLTAGE, sensorDebug),1.0,0);
  Serial.print(F("Engine h  : "));Serial.println(sensors.getEngineSeconds()/3600.0);
  Serial.print(F("Coolant T : "));printN2K(sensors.getCoolantTemperatureK(ADC_COOLANT_TEMPERATURE, ADC_ENGINEBATTERY, sensorDebug),1.0,273.15);
  Serial.print(F("Oil Psi   : "));printN2K(sensors.getOilPressure(ADC_OIL_SENSOR, sensorDebug),1.0/6894.76,0.0);
  sensors.read(sensorDebug);
  Serial.print(F("Engine On : "));Serial.println(sensors.isEngineRunning()?"Y":"N");
  Serial.print(F("Engine RPM: "));printN2K(sensors.getEngineRPM(),1.0,0);
#ifndef INSPECT_FLASH_USAGE
  uint8_t maxActiveDevices = oneWireSensor.getMaxActiveDevice();
  Serial.print(F("Onewire N : "));Serial.println(maxActiveDevices);
  for (int i = 0; i < maxActiveDevices; i++) {
    Serial.print(F("    "));
    Serial.print(i);
    Serial.print(F(" : "));
    printN2K(oneWireSensor.getTemperatureK(i),1.0,273.15);
  }
#endif
  engineMonitor.dumpStatus();


  Serial.print(F("ADC_EXH  : "));sensors.dumpADC(ADC_EXHAUST_NTC1);
  Serial.print(F("ADC_ALT  : "));sensors.dumpADC(ADC_ALTERNATOR_NTC2);
  Serial.print(F("ADC_ROOM : "));sensors.dumpADC(ADC_ENGINEROOM_NTC3);
  Serial.print(F("ADC_ALT_V: "));sensors.dumpADC(ADC_ALTERNATOR_VOLTAGE);
  Serial.print(F("ADC_FUEL : "));sensors.dumpADC(ADC_FUEL_SENSOR);
  Serial.print(F("ADC_OIL  : "));sensors.dumpADC(ADC_OIL_SENSOR);
  Serial.print(F("ADC_COOL : "));sensors.dumpADC(ADC_COOLANT_TEMPERATURE);
  Serial.print(F("ADC_BAT  : "));sensors.dumpADC(ADC_ENGINEBATTERY);

  sensors.dumpEngineStatus1();
  sensors.dumpEngineStatus2();

  // dump the stored events
  uint32_t lastEvent = 0;
  Serial.print(F("Stored Events:"));
  Serial.println(sensors.localStorage.countEvents());
  for (int i = 0; i < 30; ++i) {
    uint8_t eventId = sensors.localStorage.nextEvent(lastEvent);
    if ( eventId == EVENTS_NO_EVENT) {
      break;
    }
    Serial.print(F("evt:"));
    Serial.print(eventId);
    Serial.print(F(" h:"));
    Serial.println(0.004166666667*lastEvent);
  }


}

void monitor() {
  static unsigned long lastMonitorOutput = 0;
  if ( monitorEnabled) {
    unsigned long now = millis();
    if ( now-lastMonitorOutput > 1000 ) {
      lastMonitorOutput = now;
      Serial.print("rpm=");
      printN2K(sensors.getEngineRPM(),1.0,0,",");
      Serial.print(" coolant=");
      printN2K(sensors.getCoolantTemperatureK(ADC_COOLANT_TEMPERATURE, ADC_ENGINEBATTERY, false),1.0, 273.15, ",");
      Serial.print(" oil=");
      printN2K(sensors.getOilPressure(ADC_OIL_SENSOR, false),1.0/6894.76,0.0, ",");
      Serial.print(" fuel=");
      printN2K(sensors.getFuelLevel(ADC_FUEL_SENSOR, false),1.0, 0.0, ",");
      Serial.print(" batV=");
      printN2K(sensors.getVoltage(ADC_ENGINEBATTERY, false), 1.0, 0.0, ",");
      Serial.print(" altV=");
      printN2K(sensors.getVoltage(ADC_ALTERNATOR_VOLTAGE, false),1.0, 0.0, ",");
      Serial.print(" exT=");
      printN2K(sensors.getTemperatureK(ADC_EXHAUST_NTC1, false), 1.0, 273.15, ",");
      Serial.print(" altT=");
      printN2K(sensors.getTemperatureK(ADC_ALTERNATOR_NTC2, false),1.0, 273.15, ",");
      Serial.print(" erT=");
      printN2K(sensors.getTemperatureK(ADC_ENGINEROOM_NTC3, false),1.0, 273.15);
    }
  }
}


void(* resetDevice) (void) = 0; //declare reset function @ address 0

void setEngineHours() {
  Serial.setTimeout(10000);
  Serial.print(F("Engine Hours ?>"));
  char buffer[10];
  size_t l = Serial.readBytesUntil('\n', buffer, 9);
  if ( l > 0) {
    buffer[l] = '\0';
    double hours = atof(buffer);
    Serial.print(F("Engine Hours:"));
    Serial.println(hours);
    sensors.setEngineSeconds(hours*3600.0);
  } else {
    Serial.println("");
    Serial.println(F("canceled"));
  }
  Serial.setTimeout(100);
}

void setStoredVddVoltage() {
  Serial.setTimeout(10000);
  Serial.print(F("Vdd Voltage ?>"));
  char buffer[10];
  size_t l = Serial.readBytesUntil('\n', buffer, 9);
  if ( l > 0) {
    buffer[l] = '\0';
    double measuredVoltagee = atof(buffer);
    Serial.print(F("Vdd Voltage:"));
    Serial.println(measuredVoltagee,5);
    sensors.setStoredVddVoltage(measuredVoltagee);
  } else {
    Serial.println("");
    Serial.println(F("canceled"));
  }
  Serial.setTimeout(100);
}

void toggleDiagnostics() {
  static bool diagnostics = false;
  diagnostics = !diagnostics;
  engineMonitor.setDiagnostics(diagnostics);
}

void showHelp() {
  Serial.println(F("N2K Engine monitor."));
  Serial.print(F("Git Version: "));Serial.println(F(GIT_SHA1_VERSION));
  Serial.println(F("  - Send 'h' help"));
  Serial.println(F("  - Send 'E' set hours"));
  Serial.println(F("  - Send 'V' set Vdd "));
  Serial.println(F("  - Send 'F' fake Engine RPM at 1K"));
  Serial.println(F("  - Send 's' status"));
  Serial.println(F("  - Send 'C' clear events"));
  Serial.println(F("  - Send 'd' toggle N2k debug"));
  Serial.println(F("  - Send 'D' toggle Sensor debug"));
  Serial.println(F("  - Send 'M' toggle Engine debug"));
  Serial.println(F("  - Send 'R' restart"));
}




void checkCommand() {
  // Remove after testing
  static uint8_t eventId = 1;
  if (Serial.available()) {
    char chr = Serial.read();
    switch ( chr ) {
      case 'h': showHelp(); break;
      case 's':
        showStatus();
        break;
      case 'E':
        setEngineHours();
        break;
      case 'e':
        // NOT in the menu, only here for testing
        sensors.localStorage.engineHoursPeriods += 10;
        sensors.localStorage.saveEvent(eventId++);
        break;
      case 'F':
        sensors.toggleFakeEngineRunning();
        break;
      case 'd':
        toggleDiagnostics();
        break;
      case 'D':
        sensorDebug = !sensorDebug;
        break;
      case 'M':
        monitorEnabled = !monitorEnabled;
        break;
      case 'C':
        sensors.localStorage.clearEvents();
        break;
      case 'V':
        setStoredVddVoltage();
        break;
      case 'R':
        Serial.println(F("Restart"));
        delay(100);
        resetDevice();
        break;
    }
  }
}




void messageHandler(MessageHeader *requestMessageHeader, byte * buffer, int len) {
  if ( requestMessageHeader->pgn == ENGINE_PROPRIETARY_PGN) { // single packet pro[prietary]

    uint16_t id = (((unsigned long )buffer[2])<<16)|(((unsigned long )buffer[1])<<8)|(buffer[0]);
    if ( id == ENGINE_PROPRIETARY_CODE ) { // matches us 2046 + marine industry
      uint8_t function = buffer[2];
      if ( function == FN_DUMP_EVENTS) { // a function we recognise, dump the stored events
        // requested error history
        uint8_t nevents = sensors.localStorage.countEvents();
        MessageHeader messageHeader(ENGINE_PROPRIETARY_FP_PGN, 6, engineMonitor.getAddress(), requestMessageHeader->source);
        engineMonitor.startFastPacket(&messageHeader, 2+2+4+nevents*4);
        engineMonitor.output2ByteUInt(ENGINE_PROPRIETARY_CODE);
        engineMonitor.outputByte(FN_DUMP_EVENTS_RESP);
        engineMonitor.outputByte(nevents+1);
        engineMonitor.outputByte(0);
        double h = 0.004166666667*sensors.localStorage.engineHoursPeriods;
        engineMonitor.output3ByteUDouble(h, 0.001);
        uint32_t lastEvent = 0;
        for(int i = 0; i < nevents; i++) {
          uint8_t eventId = sensors.localStorage.nextEvent(lastEvent);
          engineMonitor.outputByte(eventId);
          h = 0.004166666667*lastEvent;
          engineMonitor.output3ByteUDouble(h, 0.001);
        }
        engineMonitor.finishFastPacket();
      } else if ( function == FN_CLEAR_EVENTS) { // clear stored events
        sensors.localStorage.clearEvents();
        MessageHeader messageHeader(ENGINE_PROPRIETARY_PGN, 6, engineMonitor.getAddress(), requestMessageHeader->source);
        engineMonitor.startPacket(&messageHeader);
        engineMonitor.output2ByteUInt(ENGINE_PROPRIETARY_CODE);
        engineMonitor.outputByte(FN_CLEAR_EVENTS_RESP);
        engineMonitor.finishPacket();
      } 
      // TODO, consider replacing serial functions with CAN proprietary messages 
      // which are easier to send without direct access to the serial port.
      // setEngineHours - no real point in exposing this over can
      // setStoredVddVoltage -- the reading for this is only available with the box open and should never change. 

    }
  }
}


void setup() {
  Serial.begin(19200);
  Serial.println(F("Luna engine monitor start"));
  setupLed();
  engineMonitor.setMessageHandler(messageHandler);
  Serial.println(F("Opening CAN"));
#ifdef CANCONTROLLER_SPEED_8MHZ
  while (!engineMonitor.open(MCP_8MHz) ) {
#else
  while (!engineMonitor.open(MCP_16MHz) ) {
#endif
    Serial.println(F("CAN Failed"));
    delay(5000);
    blinkLed(2);
  }
  Serial.println(F("Opened, MCP2515 Operational"));

  while(!sensors.begin() ) {
    Serial.println(F("Engine Sensors failed"));
    delay(5000);
    blinkLed(3);
  }

#ifndef INSPECT_FLASH_USAGE
  Serial.print(F("Starting one wire on:"));
  Serial.println(PIN_ONE_WIRE);
  oneWire.begin();
  Serial.println(F("Sarting sensor"));
  oneWireSensor.begin();
#endif
  blinkLed(4);

  Serial.println(F("Running..."));;
}

void loop() {
  sensors.read(sensorDebug);
  asyncBlink();
  monitor();
#ifndef INSPECT_FLASH_USAGE  
  oneWireSensor.readOneWire();
#endif
  sendRapidEngineData();
  sendEngineData();
  sendVoltages();
  sendTemperatures();
  sendFuel();
  engineMonitor.processMessages();
  checkCommand();
}


