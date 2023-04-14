#include <Arduino.h>
/*
 * in file secrets.h you have to define:
 *
 * #define WIFI_SSID "your wifi ssid";   // your network SSID (name)
 * #define WIFI_PSK "your wifi pass";   // your network password
 *
 */

#include "secrets.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define I2C_SDA 21
#define I2C_SCL 22

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C

unsigned long delayTime = 10000;

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 900 /* Time ESP32 will go to sleep (in seconds) */

WiFiClient espClient;

float temperatureC;
float pressure;
float humidity;
float altitude;

const char* ssid = WIFI_SSID;   // your network SSID (name)
const char* password = WIFI_PSK;   // your network password

const char* mqtt_server = "192.168.1.135"; // ip of rpi with mqtt broker running
PubSubClient client(espClient);
const int lamp = LED_BUILTIN;

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

void callback(String topic, byte* message, unsigned int length);

void setup() {

  Serial.begin(115200);
  delay(100);
  Serial.println(F("BME280 test"));

  bool status;

  pinMode(lamp,OUTPUT);
  digitalWrite(lamp,HIGH);

  WiFi.mode(WIFI_STA);

  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin(0x76);
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  Serial.println("-- Default Test --");
  delayTime = 1000;

  Serial.println();

  // Connect or reconnect to WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect");
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password);
      delay(5000);
    }
    Serial.println("\nConnected.");
  }

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic fab/lamp, you check if the message is either on or off. Turns the lamp GPIO according to the message
  if(topic=="fab/lamp"){
      Serial.print("Changing Room lamp to ");
      if(messageTemp == "on"){
        digitalWrite(lamp, LOW);
        Serial.print("On");
      }
      else if(messageTemp == "off"){
        digitalWrite(lamp, HIGH);
        Serial.print("Off");
      }
  }
  Serial.println();
}

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    /*
     YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a new name to the ESP8266.
     Here's how it looks:
       if (client.connect("ESP8266Client")) {
     You can do it like this:
       if (client.connect("ESP1_Office")) {
     Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("fab/lamp");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void getValues() {
  temperatureC = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
}

void printValues() {
  Serial.print("Temperature = ");
  Serial.print(temperatureC);
  Serial.println(" °C");

  // Convert temperature to Fahrenheit
  /*Serial.print("Temperature = ");
    Serial.print(1.8 * bme.readTemperature() + 32);
    Serial.println(" *F");*/

  Serial.print("Pressure = ");
  Serial.print(pressure);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(altitude);
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.println();
}


void loop() {

  delay(delayTime);

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP8266Client");

  now = millis();
  // Publishes new temperature and humidity every 30 seconds
  if (now - lastMeasure > 300000) {
    lastMeasure = now;

    getValues();
    printValues();

    static char temperatureTemp[7];
    dtostrf(temperatureC, 6, 2, temperatureTemp);

    static char humidityTemp[7];
    dtostrf(humidity, 6, 2, humidityTemp);

    // Publishes Temperature and Humidity values
    client.publish("temperatura", temperatureTemp);
    client.publish("humedad", humidityTemp);

  }

}
