#define LIGHT_SENSOR_PIN A0

void LIGHT_SENSOR_setup()
{
  pinMode(LIGHT_SENSOR_PIN, INPUT);
}

void readLight(int &value)
{
  value = analogRead(LIGHT_SENSOR_PIN);
}
