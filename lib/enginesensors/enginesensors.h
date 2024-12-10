#ifndef ENGINESENSORS_H
#define ENGINESENSORS_H

#include <Arduino.h>


// read frequencies
#define DEFAULT_FLYWHEEL_READ_PERIOD 2000

// Engine hours resolution
#define ENGINE_HOURS_PERIOD_MS 15000


// ADC assignments
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

// stored as voltage
// regulator output at 4.92v
// calibration
// voltage scale 0.01546609357, reading 14.77, actual, 14.45 new scale = (0.01546609357*14.45/14.77) = 0.01513101233
// 0.01513101233, 14.46/14.50=0.9972413793, 13.71/13.74=0.9978165939, 12.88/12.91=0.99767622,  12.37/12.39=0.99838
// (0.9972413793+0.9978165939+0.99767622+0.99838)/4=0.9977785483  0.01513101233*0.9977785483 = 0.01509739952
#define VOLTAGE_SCALE  0.01509739952 

// Coolant Sensor calibration.
#define COOLANT_SUPPLY_ADC_12V 780 // calibrated   12V supply Through a 47/100K divider 
                                   // 12*47/147=3.8367, calibrated at 3.8085 = ADC reading of 1024*3.8085/5=780
#define MIN_SUPPLY_VOLTAGE 261


// Fuel sensor calibration.
#define FUEL_RESISTOR 1000
#define ADC_READING_EMPTY 263 // measured for 190R
#define FUEL_SENSOR_MAX 190
#define FUEL_CAPACITY 60


// pulses  Perms -> RPM conversion.
// 415Hz at idle below needs adjusting.//
// 415Hz == 850 rpm
// 
// pulses*850*1000/(duration)*415
//  2.054
// 1879/1930=0.9735751295    1.9997233161  
#define RPM_FACTOR 1.9997233161 // 2048.1927711 // 850000/415




union EngineHoursStorage {
  uint8_t engineHoursBytes[4];
  uint32_t engineHoursPeriods;
};
union CRCStorage {
  uint8_t crcBytes[2];
  uint16_t crc;
};

class EngineSensors {
    public:

       EngineSensors(uint8_t flywheelPin=2, unsigned long flywheelReadPeriod=DEFAULT_FLYWHEEL_READ_PERIOD
                        ):
                     flywheelPin{flywheelPin},
                     flywheelReadPeriod{flywheelReadPeriod}
                     {
                     };
       bool begin();
       void read();
       bool isEngineRunning();
       void saveEngineHours();
       void setEngineSeconds(double seconds);
       double getEngineRPM();

       double getEngineSeconds();
       double getFuelLevel(uint8_t adc);
       double getOilPressure(uint8_t adc);
       double getFuelCapacity();
       double getCoolantTemperatureK(uint8_t coolantAdc, uint8_t batteryAdc);
       double getTemperatureK(uint8_t adc);
       double getVoltage(uint8_t adc);

       uint16_t getEngineStatus1();
       uint16_t getEngineStatus2();
       bool debug = false;

    private:
        void loadEngineHours();
        void writeEnginHours();
        void readEngineRPM();

        int16_t interpolate(
            int16_t reading, 
            int16_t minCurveValue, 
            int16_t maxCurveValue,
            int step, 
            const int16_t *curve, 
            int curveLength
            );
      
        uint8_t flywheelPin = 2;
        unsigned long flywheelReadPeriod = DEFAULT_FLYWHEEL_READ_PERIOD;
        EngineHoursStorage engineHours;
        double engineRPM = 0;
        bool engineRunning = false;
        uint16_t status1 = ENGINE_STATUS1_LOW_OIL_PRES | ENGINE_STATUS1_LOW_SYSTEM_VOLTAGE | ENGINE_STATUS1_WATER_FLOW ;
        uint16_t status2 = 0;


        unsigned long lastFlywheelReadTime = 0;
        unsigned long lastEngineHoursTick = 0; 
};

#endif