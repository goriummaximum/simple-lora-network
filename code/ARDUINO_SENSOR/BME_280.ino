#include "Seeed_BME280.h"
#include <Wire.h>

BME280 bme280;

void BME280_setup()
{
  while (!bme280.init());
}

void read_environmentInfo(info &packageInfo)
{
  packageInfo.tempVal = bme280.getTemperature();
  packageInfo.humidVal = bme280.getHumidity();
  packageInfo.pressVal = bme280.getPressure();
  packageInfo.altVal = bme280.calcAltitude(packageInfo.pressVal);
}
