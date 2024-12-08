
#include <OneWire.h>
#include <DallasTemperature.h>


#define MAX_ONE_WIRE_SENSORS 4

class OneWireSensors {
    public:
      OneWireSensors(OneWire * oneWire) {
        tempSensors.setOneWire(oneWire);
      }
      void begin();
      uint8_t getMaxActiveDevice();
      double getTemperature(uint8_t  n);
      void readOneWire();

    private:
      float temperature[MAX_ONE_WIRE_SENSORS];
      DeviceAddress tempDevices[MAX_ONE_WIRE_SENSORS];
      DallasTemperature tempSensors;
      uint8_t maxActiveDevice;
      unsigned long lastReadTime;
      bool requestTemperaturesRequired = false;
};