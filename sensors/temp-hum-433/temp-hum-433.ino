/**************************************************************************/
/*!
    @file     Adafruit_MPL3115A2.cpp
    @author   K.Townsend (Adafruit Industries)
    @license  BSD (see license.txt)

    Example for the MPL3115A2 barometric pressure sensor

    This is a library for the Adafruit MPL3115A2 breakout
    ----> https://www.adafruit.com/products/1893

    Adafruit invests time and resources providing this open source code,
    please support Adafruit and open-source hardware by purchasing
    products from Adafruit!

    @section  HISTORY

    v1.0 - First release
*/
/**************************************************************************/

#include <Wire.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <RCSwitch.h>
#include <SparkFunHTU21D.h>

#include "wifi.secret.h"

// Power by connecting Vin to 3-5V, GND to GND
// Uses I2C - connect SCL to the SCL pin, SDA to SDA pin
// See the Wire tutorial for pinouts for each Arduino
// http://arduino.cc/en/reference/wire
WiFiClient espClient;
PubSubClient client(espClient);

IPAddress ip(192, 168, 2, 27);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);

typedef enum
{
  eRC_CHANNEL_1 = 0,
  eRC_CHANNEL_2,
  eRC_CHANNEL_3,
  eRC_CHANNEL_4,
  eRC_CHANNEL_ALL,
  eRC_CHANNEL_NR_OFF
}EN_RC_CHANNEL;

typedef enum
{
  eSTATE_ON = 0,
  eSTATE_OFF,
  eSTATE_NR_OFF
}EN_STATE;

const PROGMEM char* g_aRcChannels[eRC_CHANNEL_NR_OFF][eSTATE_NR_OFF] =
{
  [eRC_CHANNEL_1] = 
  {
    "111101010001111000001111",
    "111101010001111000001110"
  },
  [eRC_CHANNEL_2] = 
  {
    "111101010001111000001101",
    "111101010001111000001100"
  },
  [eRC_CHANNEL_3] = 
  {
    "111101010001111000001011",
    "111101010001111000001010"
  },
  [eRC_CHANNEL_4] = 
  {
    "111101010001111000000111",
    "111101010001111000000110"
  },
  [eRC_CHANNEL_ALL] = 
  {
    "111101010001111000000010",
    "111101010001111000000001"
  },
};

const PROGMEM char* g_wifiSsid      = WIFI_SSID;
const PROGMEM char* g_wifiPassword  = WIFI_PASSWORD;

const PROGMEM char* g_mqttGateway   = MQTT_GATEWAY;
const PROGMEM char* g_mqttUser      = MQTT_USER;
const PROGMEM char* g_mqttPassword  = MQTT_PASSWORD;

const PROGMEM char* MQTT_TOPIC_KITCHEN_CHRISTMAS_LIGHT_STATE      = "/kitchen/light/christmas/status";
const PROGMEM char* MQTT_TOPIC_KITCHEN_CHRISTMAS_LIGHT_COMMAND    = "/kitchen/light/christmas/switch";

const PROGMEM char* MQTT_TOPIC_HALL_SENS_CLIMATE_TEMPERATURE      = "/hall/sens/climate/temperature";
const PROGMEM char* MQTT_TOPIC_HALL_SENS_CLIMATE_HUMIDITY         = "/hall/sens/climate/humidity";

const PROGMEM char* MQTT_TOPIC_HALL_CHRISTMAS_LIGHT_STATE         = "/hall/light/christmas/status";
const PROGMEM char* MQTT_TOPIC_HALL_CHRISTMAS_LIGHT_COMMAND       = "/hall/light/christmas/switch";


const PROGMEM char* LIGHT_ON  = "ON";
const PROGMEM char* LIGHT_OFF = "OFF";

static bool g_bKitchenLightState = false;
static bool g_bHallLightState    = false;
unsigned long oldTime = 0;
RCSwitch mySwitch = RCSwitch();
HTU21D myHTU;

void callback(char* p_topic, byte* p_payload, unsigned int p_length) 
{
  Serial.print("Message arrived [");
  Serial.print(p_topic);
  Serial.print("] ");
  String payload;
  
  for (uint8_t i = 0; i < p_length; i++) 
  {
    payload.concat((char)p_payload[i]);
  }
  Serial.println(payload);

  if (String(MQTT_TOPIC_KITCHEN_CHRISTMAS_LIGHT_COMMAND).equals(p_topic)) 
  {
    // test if the payload is equal to "ON" or "OFF"
    if (payload.equals(String(LIGHT_ON))) 
    {
      mySwitch.send((char*)g_aRcChannels[eRC_CHANNEL_2][eSTATE_ON]);
      g_bKitchenLightState = true;
      publishState();
    } 
    else if (payload.equals(String(LIGHT_OFF))) 
    {
      mySwitch.send((char*)g_aRcChannels[eRC_CHANNEL_2][eSTATE_OFF]);
      g_bKitchenLightState = false;
      publishState();
    }
  }
  else if (String(MQTT_TOPIC_HALL_CHRISTMAS_LIGHT_COMMAND).equals(p_topic))
  {
    if (payload.equals(String(LIGHT_ON))) 
    {
      mySwitch.send((char*)g_aRcChannels[eRC_CHANNEL_1][eSTATE_ON]);
      g_bHallLightState = true;
      publishState();
    } 
    else if (payload.equals(String(LIGHT_OFF))) 
    {
      mySwitch.send((char*)g_aRcChannels[eRC_CHANNEL_1][eSTATE_OFF]);
      g_bHallLightState = false;
      publishState();
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.config(ip, gateway, subnet);
  WiFi.mode(WIFI_AP);
  WiFi.begin(g_wifiSsid, g_wifiPassword);
  // Connect to wifi and print the IP address over serial
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
  client.setServer((char*)g_mqttGateway, 1883);
  client.setCallback(callback);
  
  pinMode(D3,OUTPUT);
  mySwitch.enableTransmit(D3);
  mySwitch.setRepeatTransmit(10);
  myHTU.begin();
}

void loop() {
  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();
  
  if(TIMER_TickTime(oldTime) >= 10000){
    float h = myHTU.readHumidity();
    float t = myHTU.readTemperature();
    if (isnan(h) || isnan(t)) 
    {
      Serial.println("Failed to read from DHT sensor!");
    }
    else
    {
      client.publish(MQTT_TOPIC_HALL_SENS_CLIMATE_TEMPERATURE, String(t).c_str());
      client.publish(MQTT_TOPIC_HALL_SENS_CLIMATE_HUMIDITY,    String(h).c_str());
    }
    oldTime = millis();
  } 
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    String clientName = "Esp8266" + String(random(10000));
    Serial.println(clientName);
    if (client.connect(clientName.c_str(), (char*)g_mqttUser, (char*)g_mqttPassword))
    {
      client.subscribe(MQTT_TOPIC_KITCHEN_CHRISTMAS_LIGHT_COMMAND);
      client.subscribe(MQTT_TOPIC_HALL_CHRISTMAS_LIGHT_COMMAND);
      
      Serial.println("Connected");
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publishState() 
{
  client.publish(MQTT_TOPIC_KITCHEN_CHRISTMAS_LIGHT_STATE, (g_bKitchenLightState == true) ? LIGHT_ON : LIGHT_OFF, true);
  client.publish(MQTT_TOPIC_HALL_CHRISTMAS_LIGHT_STATE,    (g_bHallLightState == true)    ? LIGHT_ON : LIGHT_OFF, true);
}

uint32_t TIMER_TickTime(const uint32_t culStart) 
{
  uint32_t g_ulMsecTickCount = millis();
  return (g_ulMsecTickCount >= culStart) ? (g_ulMsecTickCount - culStart) : ((0xFFFFFFFF - g_ulMsecTickCount) + culStart + 1);
}
