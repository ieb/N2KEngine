#ifndef ENGINESENSORS_H
#define ENGINESENSORS_H

#include <Arduino.h>


// read frequencies
#define DEFAULT_FLYWHEEL_READ_PERIOD 2000

// Engine hours resolution
#define ENGINE_HOURS_PERIOD_MS 15000




// stored as voltage
// to scale the voltage V =  V*5*(100+47)/(47*1024);
// to keep the calculation integer split
// 11.94 = 11.87  11.94/11.87 = 1.0058972199 11.87*1.0061532186 = 11.943038705
// 12.00/11.94  1.0050251256 11.94*1.0061532186 = 12.01346943
// 13.00/12.94  1.0046367852 12.94*1.0061532186 = 13.019622649
// 14.0/13.93  1.0050251256 13.94*1.0061532186 = 14.025775867
// 7.91 = 7.85  7.91/7.85 = 1.0076433121 7.85*1.0061532186 = 7.898302766
// 12.00/11.91  1.0075566751  11.91*1.0061532186 = 11.983284834
// 13.00/12.91  1.00697134   12.91*1.0061532186 = 12.989438052
// 14.00/13.91  1.0064701653  13.91*1.0061532186 = 13.99
// (1.0058972199+1.0050251256+1.0046367852+1.0050251256+1.0076433121+1.0075566751+1.00697134+1.0064701653)/8
// 1.0061532186
// 0.01537150931 * 1.0061532186 = 0.01546609357
// 11.99/11.93
#define VOLTAGE_SCALE  0.01546609357 // 5*(100+47)/(47*1024), calibrated for resistors.

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
       double getEngineRPM();

       double getEngineSeconds();
       double getFuelLevel(uint8_t adc);
       double getFuelCapacity();
       double getCoolantTemperatureK(uint8_t coolantAdc, uint8_t batteryAdc);
       double getTemperatureK(uint8_t adc);
       double getVoltage(uint8_t adc);

    private:
        void loadEngineHours();
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