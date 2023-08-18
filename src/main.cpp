#include <Arduino.h>
#include "config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <TimeLib.h>

// setting clients
WiFiClient espClient;
PubSubClient client(espClient);

// setting config from config.h
const char* ssid = WIFI_SSID;
const char* password = WIFI_PSK;
const char* mqtt_server = BROKER_IP;
const int mqtt_port = BROKER_PORT;
const char* esp_name = ESP_NAME;
const char* door_topic = DOOR_TOPIC;

// callback to be called when posting to topic
void callback(String topic, byte* message, unsigned int length);

const int led = LED_BUILTIN;
const int relePin 14;

bool ledState = HIGH;

void setup() {

  // start serial
  Serial.begin(9600);
  delay(100);
  Serial.println();

  // setting pinmode for led
  pinMode(led, OUTPUT);
  pinMode(relePin, OUTPUT);
  delay(10);
  digitalWrite(led, ledState);
  digitalWrite(relePin, !ledState);
  delay(100);
 
  // set wifi to station mode (join a network that already exists)
  WiFi.mode(WIFI_STA);

  // try to connect to network
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println("...");
 
  // waiting for wifi connection
  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
 
  Serial.println(); 
    
  // connected to wifi
  if(WiFi.status()==WL_CONNECTED)
  {
    Serial.print("Successfully connected to ");
    Serial.println(ssid);
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }
  
  // set mqtt broker and callback
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

}

// callback for message on suscribed topic
void callback(String topic, byte* message, unsigned int length) {

  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTopic;

  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTopic += (char)message[i];
  }
  Serial.println();

  // message received on door topic
  if(topic == door_topic){
      
      // open door
      if(messageTopic == "open"){
	ledState = LOW;
    	digitalWrite(ledPin, ledState);
    	digitalWrite(relePin, !ledState);
    	delay(500);
    	ledState = HIGH;
    	digitalWrite(ledPin, ledState);
    	digitalWrite(relePin, !ledState);
        Serial.print("Opening door");
      }
  }

  Serial.println();
}

void reconnect() {

  // Loop until we're reconnected
  while (!client.connected()) {
    
    Serial.print("Attempting MQTT connection...");

    if (client.connect(esp_name)) {
      Serial.println("connected");
      client.subscribe(door_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

}

void loop() {

  delay(5000);

  if (!client.connected()) {
    reconnect();
  }

  if(!client.loop())
    client.connect(esp_name);

}
