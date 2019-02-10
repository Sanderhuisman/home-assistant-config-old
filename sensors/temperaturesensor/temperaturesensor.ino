/*

  This example connects to a encrypted WiFi network (WPA/WPA2).
  Then it prints the  MAC address of the WiFi shield,
  the IP address obtained, and other network details.
  Then it continuously pings given host specified by IP Address or name.

  Circuit:
   WiFi shield attached / MKR1000

  created 13 July 2010
  by dlf (Metodo2 srl)
  modified 09 June 2016
  by Petar Georgiev
*/
#include <Wire.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "SHT21.h"

#include "wifi.secret.h"

#define MQTT_SENSOR_TOPIC "/sander/temperature"

int status = WL_IDLE_STATUS; // the WiFi radio's status
const PROGMEM char* g_wifiSsid      = WIFI_SSID;
const PROGMEM char* g_wifiPassword  = WIFI_PASSWORD;

const PROGMEM char* g_mqttGateway   = MQTT_GATEWAY;
const PROGMEM char* g_mqttUser      = MQTT_USER;
const PROGMEM char* g_mqttPassword  = MQTT_PASSWORD;

WiFiClient espClient;
PubSubClient client(espClient);

SHT21 SHT21;
unsigned long oldTime = 0;
unsigned long lastReconnectAttempt = 0;

IPAddress ip(192, 168, 2, 35);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);

void callback(char *topic, byte *payload, unsigned int length)
{
    // handle message arrived
}

void setup()
{
    Serial.begin(115200);
    WiFi.config(ip, gateway, subnet);
    WiFi.mode(WIFI_AP);
    // Connect to wifi and print the IP address over serial
    while (WiFi.status() != WL_CONNECTED)
    {
        WiFi.begin(g_wifiSsid, g_wifiPassword);
        delay(5000);
    }
    client.setServer((char*)g_mqttGateway, 1883);
    client.setCallback(callback);

    SHT21.begin();
}

boolean reconnect()
{
    //if (client.connect("temp-sensor", "sander_temperature", "V3B7^f9E!7l2"))
    if (client.connect("temp-sensor", MQTT_USER, MQTT_PASSWORD))
    {
        // Once connected, publish an announcement...
        //client.publish("/sander/tempereture","hello world");
        // ... and resubscribe
        //client.subscribe("inTopic");
    }
    return client.connected();
}

uint32_t TIMER_TickTime(const uint32_t culStart) 
{
  uint32_t g_ulMsecTickCount = millis();
  return (g_ulMsecTickCount >= culStart) ? (g_ulMsecTickCount - culStart) : ((0xFFFFFFFF - g_ulMsecTickCount) + culStart + 1);
}

void loop()
{
    if (!client.connected())
    {
        if(TIMER_TickTime(lastReconnectAttempt) >= 10000)
        {
            lastReconnectAttempt = millis();
            // Attempt to reconnect
            if (reconnect())
            {
                lastReconnectAttempt = 0;
            }
        }
    }
    else
    {
        if(TIMER_TickTime(oldTime) >= 10000)
        {
            StaticJsonDocument<200> doc;
            JsonObject root = doc.to<JsonObject>();

            float temp = SHT21.getTemperature();
            float humidity = SHT21.getHumidity();

            root["temperature"] = String(temp);
            root["humidity"] = String(humidity);

            serializeJsonPretty(root, Serial);
            //Serial.println("");

            char data[200];
            serializeJson(doc, data);
            client.publish(MQTT_SENSOR_TOPIC, data, true);

            oldTime = millis();
        }

        client.loop();
    }
}
