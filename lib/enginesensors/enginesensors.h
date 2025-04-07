#ifndef ENGINESENSORS_H
#define ENGINESENSORS_H

#include <Arduino.h>


// read frequencies
#define DEFAULT_FLYWHEEL_READ_PERIOD 500

// Engine hours resolution
#define ENGINE_HOURS_PERIOD_MS 15000
// dont emit any errors or warnings for measurements that require the engine to be running.
// while it spins up.
#define  ENGINE_START_GRACE_PERIOD 15000


// PGN 127489, status 1 and status 2 fields.

#define ENGINE_STATUS1_CHECK_ENGINE        0x0001
#define ENGINE_STATUS1_OVERTEMP            0x0002
#define ENGINE_STATUS1_LOW_OIL_PRES        0x0004
#define ENGINE_STATUS1_LOW_OIL_LEVEL       0x0008
#define ENGINE_STATUS1_LOW_FUEL_PRESS      0x0010
#define ENGINE_STATUS1_LOW_SYSTEM_VOLTAGE  0x0020
#define ENGINE_STATUS1_LOW_COOLANT_LEVEL   0x0040
#define ENGINE_STATUS1_WATER_FLOW          0x0080
#define ENGINE_STATUS1_WATER_IN_FUEL       0x0100
#define ENGINE_STATUS1_CHARGE_INDICATOR    0x0200
#define ENGINE_STATUS1_PREHEAT_INDICATOR   0x0400
#define ENGINE_STATUS1_HIGH_BOOST_PRESSURE 0x0800
#define ENGINE_STATUS1_REV_LIMIT_EXCEEDED  0x1000
#define ENGINE_STATUS1_EGR_SYSTEM          0x2000
#define ENGINE_STATUS1_THROTTLE_POS_SENSOR 0x4000
#define ENGINE_STATUS1_EMERGENCY_STOP      0x8000

#define ENGINE_STATUS2_WARN_1              0x0001
#define ENGINE_STATUS2_WARN_2              0x0002
#define ENGINE_STATUS2_POWER_REDUCTION     0x0004
#define ENGINE_STATUS2_MAINTANENCE_NEEDED  0x0008
#define ENGINE_STATUS2_ENGINE_COMM_ERROR   0x0010
#define ENGINE_STATUS2_SUB_OR_SECONDARY_THROTTLE  0x0020
#define ENGINE_STATUS2_NEUTRAL_START_PROTECT   0x0040
#define ENGINE_STATUS2_ENGINE_SUTTING_DOWN 0x0080


#define SET_BIT(x,y) x=(x) | (y)
#define CLEAR_BIT(x,y) x=(x) & (~(y))




// Fuel sensor calibration.
#define FUEL_RESISTOR 1000
#define FUEL_SENSOR_MAX 190
#define FUEL_CAPACITY 60


// pulses  Perms -> RPM conversion.
// 415Hz at idle below needs adjusting.//
// 415Hz == 830 rpm
// There are 30 teeth on the flywheel to count RPM.  (see screenshots/flywheel.png, part number 3840880)
// Hz to RPM == 60/30 == 2.0  ( not so odd 30 was chosen << 1 on Hz == RPM)
// There could be a factor to apply to account for any clock frequency errors.
#define RPM_FACTOR 2.0 


class LocalStorage {
public:
    LocalStorage() {};
    void loadVdd();
    void setVdd(double vdd);
    void loadEngineHours();
    void saveEngineHours();

    uint32_t engineHoursPeriods = 0;
    double vdd = 5.0;
private:
    void updateCRC();
    bool eepromValid();
};

class EngineSensors {
    public:

       EngineSensors(uint8_t flywheelPin, 
                    uint8_t adcAlternatorVoltage,
                    uint8_t adcEngineBattery,
                    uint8_t adcExhaustNTC1,
                    uint8_t adcAlternatorNTC2,
                    uint8_t adcEngineRoomNTC3,
                    unsigned long flywheelReadPeriod=DEFAULT_FLYWHEEL_READ_PERIOD
                    ){
                        this->flywheelReadPeriod = flywheelReadPeriod;
                        this->flywheelPin = flywheelPin;
                        this->adcAlternatorVoltage = adcAlternatorVoltage;
                        this->adcEngineBattery = adcEngineBattery;
                        this->adcExhaustNTC1 = adcExhaustNTC1;
                        this->adcAlternatorNTC2 = adcAlternatorNTC2;
                        this->adcEngineRoomNTC3 = adcEngineRoomNTC3;
                     };
       bool begin();
       void read(bool outoutDebug=false);
       bool isEngineRunning();
       void saveEngineHours();
       void setEngineSeconds(double seconds);
       void toggleFakeEngineRunning();

       double getEngineRPM();

       double getEngineSeconds();
       double getFuelLevel(uint8_t adc, bool outoutDebug=false);
       double getOilPressure(uint8_t adc, bool outoutDebug=false);
       double getFuelCapacity();
       double getCoolantTemperatureK(uint8_t coolantAdc, uint8_t batteryAdc, bool outoutDebug=false);
       double getTemperatureK(uint8_t adc, bool outoutDebug=false);
       double getVoltage(uint8_t adc, bool outoutDebug=false);
       void dumpADC(uint8_t adc);
       void dumpADCVDD();

       void setStoredVddVoltage(double measuredVddVoltage);
       double getStoredVddVoltage();
        void readEngineRPM(bool outoutDebug=false);


       uint16_t getEngineStatus1();
       uint16_t getEngineStatus2();

    private:
        void loadEngineHours();
        void writeEnginHours();

        int16_t interpolate(
            int16_t reading, 
            int16_t minCurveValue, 
            int16_t maxCurveValue,
            int step, 
            const int16_t *curve, 
            int curveLength
            );

      
        uint8_t flywheelPin;
        uint8_t adcAlternatorVoltage;
        uint8_t adcEngineBattery;
        uint8_t adcExhaustNTC1;
        uint8_t adcAlternatorNTC2;
        uint8_t adcEngineRoomNTC3;
        unsigned long flywheelReadPeriod = DEFAULT_FLYWHEEL_READ_PERIOD;
        LocalStorage localStorage;
        double engineRPM = 0;
        bool engineRunning = false;
        uint16_t status1 = 0;
        uint16_t status2 = 0;
        bool fakeEngineRunning = false;


#ifdef __AVR_TINY_2__
#ifdef FREQENCY_METHOD_1
        uint16_t lastInteruptCount = 0;
        uint16_t smoothedRPM[4];
        uint8_t slot = 0;
#endif
#ifdef FREQENCY_METHOD_2
        uint16_t edgeInterrupts[2] = {0,0};
        uint16_t overflowInterrupts[2] = {0,0};
#endif

#endif
        bool eepromWritten = false;
        bool canEmitAlarms = false;
        unsigned long lastFlywheelReadTime = 0;
        unsigned long lastEngineHoursTick = 0; 
        unsigned long engineStarted = 0;
};

#endif