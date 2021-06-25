#include <LoRa.h>
#include <ArduinoJson.h>

#define SS 10
#define RST 9
#define DI0 2
#define BAND 433E6

struct info
{
  float tempVal = 0;
  float humidVal = 0;
  float pressVal = 0;
  float altVal = 0;
  int lightVal = 0;
};

struct package
{
  String src;
  String dst;
  info packageInfo;
};

package dataPackage;
int packetSend = 0;

void setup()
{
  Serial.begin(9600);
  LORA_NODE_setup();
  BME280_setup();
  LIGHT_SENSOR_setup();

  dataPackage.src = "NODE0";
  dataPackage.dst = "GATEWAY0";
}

void loop() {
  if (runEvery(700))
  {
    read_environmentInfo(dataPackage.packageInfo);
    readLight(dataPackage.packageInfo.lightVal);
    sendMessage(dataPackage);
  }
}

void LORA_NODE_setup()
{
  LoRa.setPins(SS,RST,DI0);

  while (!LoRa.begin(BAND))
  {
    delay(1000);
  }

  LoRa.setTxPower(20, 20);
  LoRa.setSyncWord(0x29);
  //LoRa.setSpreadingFactor(8);
}

void sendMessage(package dataPackage) {
  String message = "";
  JsonSerialize(message, dataPackage);
  LoRa.beginPacket();
  LoRa.write(++packetSend);
  LoRa.write(message.length());
  LoRa.print(message);
  LoRa.endPacket();
}

void onReceive(int packetSize, StaticJsonDocument<200> &receivedPackage) {
  if (packetSize == 0)
    return;

  byte incomingMsglength;
  String incomingMsg = "";

  while (LoRa.available()) {
    incomingMsglength = LoRa.read();
    incomingMsg = LoRa.readString();
  }

  // check length for error
  if (incomingMsglength != incomingMsg.length())
  {
    return;
  }

  //deserialize message
  StaticJsonDocument<200> incomingPackage;
  if (JsonDeserialize(incomingMsg, incomingPackage) == false)
    return;
    
  receivedPackage = incomingPackage;
}

void JsonSerialize(String &message, package dataPackage)
{
  StaticJsonDocument<200> jsonPackage;
  jsonPackage["src"] = dataPackage.src;
  jsonPackage["dst"] = dataPackage.dst;
  jsonPackage["packageInfo"]["tempVal"] = dataPackage.packageInfo.tempVal;
  jsonPackage["packageInfo"]["humidVal"] = dataPackage.packageInfo.humidVal;
  jsonPackage["packageInfo"]["pressVal"] = dataPackage.packageInfo.pressVal;
  jsonPackage["packageInfo"]["altVal"] = dataPackage.packageInfo.altVal;
  jsonPackage["packageInfo"]["lightVal"] = dataPackage.packageInfo.lightVal;
  serializeJson(jsonPackage, message);
}

bool JsonDeserialize(String message, StaticJsonDocument<200> &receivedPackage)
{
   StaticJsonDocument<200> jsonPackage;
   DeserializationError error = deserializeJson(jsonPackage, message);
   if (error)
   {
      return false;
   }

   if (jsonPackage["dst"] != dataPackage.src)
   {
      return false;
   }

   receivedPackage = jsonPackage;
   return true;
}

boolean runEvery(unsigned long interval)
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}
