#ifndef ENGINESENSORS_H
#define ENGINESENSORS_H

#include <Arduino.h>


// read frequencies
#define DEFAULT_FLYWHEEL_READ_PERIOD 2000

// Engine hours resolution
#define ENGINE_HOURS_PERIOD_MS 15000




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
                     {};
       bool begin();
       void read();
       bool isEngineRunning();
       void saveEngineHours();
       void setEngineSeconds(double seconds);
       double getEngineRPM();

       double getEngineSeconds();
       double getFuelLevel(uint8_t adc);
       double getFuelCapacity();
       double getCoolantTemperatureK(uint8_t coolantAdc, uint8_t batteryAdc);
       double getTemperatureK(uint8_t adc);
       double getVoltage(uint8_t adc);
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


        unsigned long lastFlywheelReadTime = 0;
        unsigned long lastEngineHoursTick = 0; 
};

#endif