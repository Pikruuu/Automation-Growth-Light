//for help look at: https://github.com/Starmbi/hp_BH1750/wiki
#include <Arduino.h>
#include <hp_BH1750.h>  //  include the library
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
hp_BH1750 BH1750;       //  create the sensor

// ==== WiFi & MQTT setup ====
const char* WIFI_SSID = "Kelompok3";
const char* WIFI_PASSWORD = "333.333.333";

const char* MQTT_BROKER = "192.168.79.244";  // ganti dengan IP PC yang menjalankan mqtt/broker.py
const uint16_t MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "hydrolight-esp32";

const char* TOPIC_LUX = "hydrolight/device/lux";
const char* TOPIC_MOL = "hydrolight/device/mol";
const char* TOPIC_LED = "hydrolight/device/led";
const char* TOPIC_STATUS = "hydrolight/device/status";
const char* TOPIC_COMMAND = "hydrolight/device/command";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setupWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi connecting");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi failed, continuing offline");
  }
}

void setupMQTT()
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("MQTT skipped: WiFi not connected");
    return;
  }
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  for (int i = 0; i < 5 && !mqttClient.connected(); i++) {
    Serial.print("MQTT connecting...");
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected");
      mqttClient.publish(TOPIC_STATUS, "online", true);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retry 2s");
      delay(2000);
    }
  }
}

void onMqttMessage(char* topic, byte* payload, unsigned int length)
{
  Serial.print(topic);
  Serial.print(": ");
  for (unsigned int i = 0; i < length; i++) Serial.print((char)payload[i]);
  Serial.println();
}

void reconnectMQTT()
{
  static unsigned long lastTry = 0;
  if (millis() - lastTry < 5000) return;
  lastTry = millis();
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    return;
  }
  if (mqttClient.connect(MQTT_CLIENT_ID, TOPIC_STATUS, 0, true, "offline")) {
    mqttClient.publish(TOPIC_STATUS, "online", true);
    mqttClient.subscribe(TOPIC_COMMAND);
  }
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  bool avail = BH1750.begin(BH1750_TO_GROUND);// init the sensor with address pin connetcted to ground
  // result (bool) wil be be "false" if no sensor found
  if (!avail) {
    Serial.println("No BH1750 sensor found!");
    while (true) {};                                        
  }
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
	display.setTextSize(2);
	display.setTextColor(SSD1306_WHITE);

  setupWiFi();
  setupMQTT();
  mqttClient.setCallback(onMqttMessage);
  if (mqttClient.connected()) mqttClient.subscribe(TOPIC_COMMAND);
}

void loop()
{
  
  // put your main code here, to run repeatedly:
  BH1750.start();   //starts a measurement
  float mol=BH1750.getLux() * 0.145;  //  waits until a conversion finished
  
  display.clearDisplay();
  display.setCursor(0, 0);
	display.print("Mol: ");
	display.print(mol);

  if(mol < 50){
      Serial.print(mol);
      Serial.println(" TRUE");        
      display.setCursor(0, 18);
      display.print("led: TRUE");
    } else {
      Serial.print(mol);
      Serial.println(" FALSE");
      display.setCursor(0, 18);
      display.print("led: FALSE");
  }
  display.display();

  if (!mqttClient.connected()) reconnectMQTT();
  mqttClient.loop();
  static unsigned long lastPublish = 0;
  if (mqttClient.connected() && millis() - lastPublish >= 2000) {
    lastPublish = millis();
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f", mol / 0.145);
    mqttClient.publish(TOPIC_LUX, buf);
    snprintf(buf, sizeof(buf), "%.2f", mol);
    mqttClient.publish(TOPIC_MOL, buf);
    mqttClient.publish(TOPIC_LED, mol < 50 ? "TRUE" : "FALSE");
  }
  delay(333);
}