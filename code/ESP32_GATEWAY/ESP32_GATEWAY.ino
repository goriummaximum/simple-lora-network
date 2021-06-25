#include <SPI.h>
#include <LoRa.h>
#include <SSD1306.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

SSD1306 display(0x3c, 4, 15);

#define SS 18
#define RST 14
#define DI0 26
#define BAND 433E6

char auth[] = "ImBlJWW87CZQom_rkcumDzupAMYjlRR5";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "TAOLAGO";
char pass[] = "0097780694";
BlynkTimer BlyTimer;
WidgetLED led0(V0);

struct info
{
  int ledVal = 0;
};

struct package
{
  String src;
  String dst;
  info packageInfo;
};

package dataPackage;
int packetSend = 0;
int packetReceive = 0;
int rssi = 0;
float snr = 0;
long freqErr = 0;

int nodes = 3;
String registeredNodes[] = {"NODE0", "NODE1"};

StaticJsonDocument<300> receivedPackage;

void setup() {
  Serial.begin(9600);
  WiFi_setup();
  BLYNK_setup();
  OLED_setup();
  LORA_NODE_setup();

  display.drawString(5, 25, "LoRa OK!");
  display.display();

  dataPackage.src = "GATEWAY0";
  dataPackage.dst = "NODE1";

  BlyTimer.setInterval(1000L, BlynkTimerOperation);
}

void loop()
{
  BlynkRun();
  //receive
  receivedPackage["src"] = "NA";
  receivedPackage["dst"] = "NA";
  onReceive(LoRa.parsePacket(), receivedPackage);
  printOLED(receivedPackage);

  //send
  if (runEvery(900))
  {
    sendMessage(dataPackage);
    delay(100);
    Serial.println(String(packetReceive) + "," + String(packetSend) + "," + String(rssi) + "," + String(snr) + "," + String(freqErr));
  }
}

void WiFi_setup()
{
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {}
}

void BLYNK_setup()
{
  Blynk.begin(auth, ssid, pass);
}

void LORA_NODE_setup()
{
  LoRa.setPins(SS, RST, DI0);

  while (!LoRa.begin(BAND))
  {
    delay(1000);
  }
  LoRa.setTxPower(20, 20);
  LoRa.setSyncWord(0x29);
  //LoRa.setSpreadingFactor(8);
}

void OLED_setup()
{
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW); // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(16, HIGH);

  display.init();
  display.flipScreenVertically();

  display.drawString(5, 5, "LoRa Gateway");
  display.display();
}

void sendMessage(package dataPackage) {
  String message = "";
  JsonSerialize(message, dataPackage);
  LoRa.beginPacket();                   // start packet
  LoRa.write(message.length());
  LoRa.print(message);
  LoRa.endPacket();                     // finish packet and send it
}

void onReceive(int packetSize, StaticJsonDocument<300> &receivedPackage) {
  if (packetSize == 0)
    return;

  byte incomingMsglength;
  String incomingMsg = "";

  while (LoRa.available()) {
    packetSend = LoRa.read();
    incomingMsglength = LoRa.read();
    incomingMsg = LoRa.readString();
  }
  if (packetSend == 1)
  {
    packetReceive = 0;
  }
  
  // check length for error
  
  if (incomingMsglength != incomingMsg.length())
  {
    return;
  }
 
  //deserialize message
  StaticJsonDocument<300> incomingPackage;
  if (JsonDeserialize(incomingMsg, incomingPackage) == false)
    return;

  receivedPackage = incomingPackage;
  packetReceive++;
  rssi = LoRa.packetRssi();
  snr = LoRa.packetSnr();
  freqErr = LoRa.packetFrequencyError();
}

void JsonSerialize(String &message, package dataPackage)
{
  StaticJsonDocument<300> jsonPackage;
  jsonPackage["src"] = dataPackage.src;
  jsonPackage["dst"] = dataPackage.dst;
  jsonPackage["packageInfo"]["ledVal"] = dataPackage.packageInfo.ledVal;
  serializeJson(jsonPackage, message);
}

bool JsonDeserialize(String message, StaticJsonDocument<300> &receivedPackage)
{
  StaticJsonDocument<300> jsonPackage;
  DeserializationError error = deserializeJson(jsonPackage, message);
  if (error)
  {
    return false;
  }

  if (jsonPackage["dst"] != dataPackage.src && isRegisteredNode(jsonPackage["src"]) == false)
  {
    return false;
  }

  receivedPackage = jsonPackage;
  return true;
}

void printOLED(StaticJsonDocument<300> receivedPackage)
{
  display.clear();
  if (receivedPackage["src"] == "NODE0")
  {
    float tempVal = receivedPackage["packageInfo"]["tempVal"];
    float humidVal = receivedPackage["packageInfo"]["humidVal"];
    float pressVal = receivedPackage["packageInfo"]["pressVal"];
    float altVal = receivedPackage["packageInfo"]["altVal"];
    float lightVal = receivedPackage["packageInfo"]["lightVal"];
    display.drawString(5, 0, "T: " + String(tempVal) + " *C");
    display.drawString(5, 10, "H: " + String(humidVal) + " %");
    display.drawString(5, 20, "P: " + String(pressVal / 1000.0) + " kPa");
    display.drawString(5, 30, "A: " + String(altVal) + " m");
    display.drawString(5, 40, "L: " + String(lightVal));
    display.drawString(5, 50, "LED: " + String(dataPackage.packageInfo.ledVal));
    //display.drawString(80, 0, String(packetReceive) + "/" + String(packetSend));
    //display.drawString(80, 10, String(rssi));
    display.display();
  }
}

bool isRegisteredNode(String node)
{
  for (int i = 0; i < 3; i++)
  {
    if (node == registeredNodes[i])
    {
      return true;
    }
  }

  return false;
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

void BlynkRun()
{
  Blynk.run();
  BlyTimer.run();
}

void BlynkTimerOperation()
{
  if (led0.getValue())
  {
     led0.off();
  }

  else
  {
    led0.on();
  }
  
  if (receivedPackage["src"] == "NODE0")
  {
    float tempVal = receivedPackage["packageInfo"]["tempVal"];
    float humidVal = receivedPackage["packageInfo"]["humidVal"];
    float pressVal = receivedPackage["packageInfo"]["pressVal"];
    float altVal = receivedPackage["packageInfo"]["altVal"];
    float lightVal = receivedPackage["packageInfo"]["lightVal"];
    Blynk.virtualWrite(V1, tempVal);
    Blynk.virtualWrite(V2, humidVal);
    Blynk.virtualWrite(V3, pressVal / 1000.0);
    Blynk.virtualWrite(V4, altVal);
    Blynk.virtualWrite(V5, lightVal);

    //Blynk.virtualWrite(V7, packetSend);
    //Blynk.virtualWrite(V8, packetReceive);
    //Blynk.virtualWrite(V9, rssi);
  }
 }

BLYNK_WRITE(V6)
{
  dataPackage.packageInfo.ledVal = param.asInt();
}
