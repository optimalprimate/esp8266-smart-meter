//this version is for electricity on A0, and for reading the gas meter rotation on 0 on D5 (pin GPIO14)

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#include "EmonLib.h"

EnergyMonitor emon1;

const char* ssid = "***";
const char* password =  "***";
const char* mqttServer = "192.168.1.**";
const int mqttPort = 1883;
int gas_count = 0;
int gas_delay_counter = 0; //this is to debounce the gas signal
int gas_time = 0;
int prev_gas_time = 0;
int gas_count_prev = 0;

unsigned long time_now = 0;
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
Serial.begin(9600); 


WiFi.mode(WIFI_STA);
WiFi.begin(ssid, password);


 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
 
  client.setServer(mqttServer, mqttPort);
//client.setCallback(callback);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
 // Create a random client ID
    String clientId = "PowerMet_";
    clientId += String(random(0xffff), HEX);
if (client.connect(clientId.c_str())) {  

 
      Serial.println("connected");  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  

}
OTA_setup();

emon1.current(0, 111.1);             // Current: input pin, calibration.
client.publish("esp/test", "Hello from PowerMeter");

pinMode(14,INPUT); //define optical sensor pin
}

void reconnect() {
  Serial.println("Reconnect activated");
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");


// Create a random client ID
    String clientId = "PowerMeter_";
    clientId += String(random(0xffff), HEX);
if (client.connect(clientId.c_str())) {  
      Serial.println("connected");
       delay(1000);
        client.publish("esp/test", "Hello from powermeter(recon)");
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
 ArduinoOTA.handle();
  if (!client.connected()) {
    reconnect();
  }

//%%%%%%%%%%%%%%%%%%%%%%%%LOOP%%%%%%%%%%%%%%%%%%%%%%%%
 if(millis() > time_now + 1500){
double Irms = emon1.calcIrms(1480);  // Calculate Irms only
client.publish("esp/power", String(Irms*230.0).c_str());
gas_count = digitalRead(14);
if(gas_count == 1 && gas_count_prev == 0){ //only if value shifts from 0 to 1
  if(gas_delay_counter >2){ //only 1 signal every 3s
     gas_time = millis()-prev_gas_time;
     prev_gas_time = millis();
     client.publish("esp/power_gas", String(gas_time).c_str()); //publish how many ms since last 1
     gas_delay_counter = 0;
  }
}
  time_now = millis(); 
  gas_delay_counter++;
  gas_count_prev = gas_count;

 }
//%%%%%%%%%%%%%%%%%%%%%%%%LOOP END%%%%%%%%%%%%%%%%%%%%%%%%
}
void OTA_setup(){
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println(WiFi.localIP());
}
