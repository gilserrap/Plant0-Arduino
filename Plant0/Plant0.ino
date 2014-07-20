
#include <Bridge.h>
#include <Console.h>
#include <Process.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "DHT.h"

//Ambient Mositure and Temperature sensor pin
#define DHT_D_PIN 2 

//Ground temperature sensor pin
#define TMPSOIL_D_PIN 3

//Ambient light sensor pin
#define LIGHT_A_PIN 4  

//Moisture sensor pins
#define MSTR_0A_PIN 0
#define MSTR_1A_PIN 1
#define MSTR_2A_PIN 2
#define MSTR_3A_PIN 3

#define DHTTYPE DHT11      //Mositure and Temperature sensor model.

DHT dht(DHT_D_PIN, DHTTYPE);

//Setup soil temperature sensor
OneWire oneWire(TMPSOIL_D_PIN);
DallasTemperature tmpSensor(&oneWire);
DeviceAddress soilTmpSensorAddress;

void setup() {

  Bridge.begin();
  
  Console.begin();
  while(!Console);
  Console.println("Console connected!");
  
  //Initialize humidity & temperature sensor
  dht.begin();
  
  //Initialize temperature sensor
  tmpSensor.begin();
  
  Console.println("Searching sensor");
  if (!tmpSensor.getAddress(soilTmpSensorAddress, 0)) Console.println("Unable to find address for soil tmp sensor"); 
  printAddress(soilTmpSensorAddress);
  Console.println("Sensor available?");
  Console.print(tmpSensor.isConnected(soilTmpSensorAddress));
  
  tmpSensor.setResolution(soilTmpSensorAddress, 9);
}

void loop() {
  collectDataAndSend();
  delay(300000);
}

float MAX_ANALOG_VALUE = 800.0;

void collectDataAndSend() {
  
  //Ambient readings.
  float ambientHumidity = dht.readHumidity();

  float ambientTemperature = dht.readTemperature();
  
  int lightSensorValue = analogRead(LIGHT_A_PIN);
  float ambientLight = (lightSensorValue/MAX_ANALOG_VALUE)*100.0;
  
  //Plant readings.

  //Plant 0
  int plant0MSTRValue = analogRead(MSTR_0A_PIN);
  float plant0MSTR = (plant0MSTRValue/MAX_ANALOG_VALUE)*100.0;
  
  //Plant 1
  int plant1MSTRValue = analogRead(MSTR_1A_PIN);
  float plant1MSTR = (plant1MSTRValue/MAX_ANALOG_VALUE)*100.0;
  
  //Plant 2
  int plant2MSTRValue = analogRead(MSTR_2A_PIN);
  float plant2MSTR = (plant2MSTRValue/MAX_ANALOG_VALUE)*100.0;
  
  //Plant 3
  int plant3MSTRValue = analogRead(MSTR_3A_PIN);
  float plant3MSTR = (plant3MSTRValue/MAX_ANALOG_VALUE)*100.0;
  
  int plantMoistureValues[4] = {(int)plant0MSTR, 
                                (int)plant1MSTR, 
                                (int)plant2MSTR, 
                                (int)plant3MSTR};

  //Since I only have one soil temperature sensor...
  tmpSensor.requestTemperatures();
  //Take one measurement from a random plant and hope the others are at the same temp :D
  float randomPlantSoilTemperature = tmpSensor.getTempC(soilTmpSensorAddress);
  
  int plantTemperatureValues[4] = {(int)randomPlantSoilTemperature, 
                                   (int)randomPlantSoilTemperature, 
                                   (int)randomPlantSoilTemperature, 
                                   (int)randomPlantSoilTemperature};
  
  Console.println("Building request.");
  String data = buildJSON( (int)ambientHumidity,
                           (int)ambientTemperature, 
                           (int)ambientLight, 
                           plantMoistureValues,
                           plantTemperatureValues, 
                           4);
  Console.println(data);
  Console.println("Request built.");
  
  //Send data to Carriots.
  sendRequestWithBody(data);
}

String deviceName = "";

String buildJSON( int ambientHumidity, 
                  int ambientTemperature, 
                  int ambientLight, 
                  int *moistureValues, 
                  int *temperatureValues, 
                  int valuesCount) 
{  
  //Create carriots required fields
  String JSON = "{\"protocol\":\"v2\",\"at\":\"now\",\"checksum\":\"\",\"persist\":\"true\",\"device\":";
  JSON += "\""; JSON += deviceName; JSON += "\",";
  
  //Create data dict
  JSON += "\"data\":{\"v\":[";
  
  //Add ambient data
  JSON += ambientHumidity; JSON += ",";
  JSON += ambientTemperature; JSON += ","; 
  JSON += ambientLight; JSON += ",";
  
  //Add plant moisture data
  for(int i = 0; i < valuesCount; i++) {
    JSON += moistureValues[i]; JSON += ",";
  }
  
  //Add plant temperature data
  for(int i = 0; i < valuesCount; i++) {
    JSON += temperatureValues[i];
    if(i < valuesCount-1){
      JSON += ",";
    }
  }
  
  JSON += "]}}";
  
  return JSON;
}

String API_KEY = "";

void sendRequestWithBody(String requestBody) {
  
  String apiKeyHeader = "carriots.apiKey:"; apiKeyHeader += API_KEY;
  
  //Build curl command to send POST request.
  Console.println("Sending data... ");
  
  Process curl;

  curl.begin("curl");
  curl.addParameter("-k");
  curl.addParameter("-X");
  curl.addParameter("POST");
  curl.addParameter("-H");
  curl.addParameter(apiKeyHeader);
  curl.addParameter("-d");
  curl.addParameter(requestBody);
  curl.addParameter("--verbose");
  curl.addParameter("http://api.carriots.com/streams/");
  curl.run();
  
  if(curl.exitValue() == 0) {
    Console.println("Request sent.");
  } else {
    Console.println("Exit value:");
    Console.println(curl.exitValue());
    Console.println("Something not supposed to happen happened.\nWho will unhappen what has happened?");
  }
  while (curl.available() > 0) {
    char c = curl.read();
    Console.print(c);
  }
}

void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Console.print("0");
    Console.print(deviceAddress[i], HEX);
  }
}

