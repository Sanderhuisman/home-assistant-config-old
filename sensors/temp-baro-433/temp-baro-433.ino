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
#include <Adafruit_MPL3115A2.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <RCSwitch.h>


// Power by connecting Vin to 3-5V, GND to GND
// Uses I2C - connect SCL to the SCL pin, SDA to SDA pin
// See the Wire tutorial for pinouts for each Arduino
// http://arduino.cc/en/reference/wire
Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();
WiFiClient espClient;
PubSubClient client(espClient);

IPAddress ip(192, 168, 2, 26);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);

const char* ssid     = "KOTNET_2.4GHz";
const char* password = "2fts*AWP5Hn5";
const char* mqtt_server = "192.168.2.12";

const PROGMEM char* MQTT_LIGHT_STATE_TOPIC = "lights/433/status";
const PROGMEM char* MQTT_LIGHT_COMMAND_TOPIC = "lights/433/switch";

const PROGMEM char* LIGHT_ON = "ON";
const PROGMEM char* LIGHT_OFF = "OFF";

bool toggle = false;

RCSwitch mySwitch = RCSwitch();


void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
  Serial.print("Message arrived [");
  Serial.print(p_topic);
  Serial.print("] ");
  String payload;
  for (uint8_t i = 0; i < p_length; i++) {
    payload.concat((char)p_payload[i]);
  }
  Serial.println(payload);


  if (String(MQTT_LIGHT_COMMAND_TOPIC).equals(p_topic)) {
    // test if the payload is equal to "ON" or "OFF"
    if (payload.equals(String(LIGHT_ON))) {
      mySwitch.send("111101010001111000001111");
      toggle = true;
      publishState();
    } else if (payload.equals(String(LIGHT_OFF))) {
      mySwitch.send("111101010001111000001110");
      toggle = false;
      publishState();
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.config(ip, gateway, subnet);
  WiFi.mode(WIFI_AP);
  WiFi.begin(ssid, password);
  // Connect to wifi and print the IP address over serial
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  
  pinMode(D3,OUTPUT);
  mySwitch.enableTransmit(D3);
  mySwitch.setRepeatTransmit(10);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if (! baro.begin()) {
    Serial.println("Couldnt find sensor");
    return;
  }
  float pascals = baro.getPressure();
  Serial.print(pascals/100); Serial.println(" hPa");
  
  float tempC = baro.getTemperature();
  Serial.print(tempC); Serial.println("*C");
  client.publish("/steven/temperature", String(tempC).c_str());
  client.publish("/steven/pressure", String(pascals/100).c_str());
  delay(1000);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    String clientName = "Esp8266" + String(random(10000));
    Serial.println(clientName);
    if (client.connect(clientName.c_str())) {
      client.subscribe(MQTT_LIGHT_COMMAND_TOPIC);
      
      Serial.println("Connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publishState() {
  if (toggle) {
    client.publish(MQTT_LIGHT_STATE_TOPIC, LIGHT_ON, true);
  } else {
    client.publish(MQTT_LIGHT_STATE_TOPIC, LIGHT_OFF, true);
  }
}
