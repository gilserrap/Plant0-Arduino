#include <Bridge.h>
#include <Console.h>
#include <Process.h>

#include "DHT.h"

#define DHT_D_PIN 2        //Ambient Mositure and Temperature sensor's pin
#define LIGHT_A_PIN 0      //Light sensor's pin
#define MOISTURE_A_PIN 1   //Moisture sensor's pin

#define DHTTYPE DHT11      //Mositure and Temperature sensor model.

DHT dht(DHT_D_PIN, DHTTYPE);

void setup() {
  //Yun setup
  Bridge.begin();
  Console.begin();
  
  //Initialise humidity & temperature sensor
  dht.begin();
  
  while (!Console);// Wait for Console port to connect.
  
  Console.println("Connection to console established");
}

void loop() {
  collectDataAndSend();
  delay(10000);
}

float MAX_ANALOG_VALUE = 720.0;

void collectDataAndSend() {
  
  //Ambient readings.
  float ambientHumidity = dht.readHumidity();
  float ambientTemperature = dht.readTemperature();
  int lightSensorValue = analogRead(LIGHT_A_PIN);
  float ambientLight = (lightSensorValue/MAX_ANALOG_VALUE)*100.0;
  
  //Plant readings.
  int plantMoistureValue = analogRead(MOISTURE_A_PIN);
  float plantMositure = (plantMoistureValue/MAX_ANALOG_VALUE)*100.0;
  
  float plantTemperature = ambientTemperature;
  
  float plantMoistureValues[1] = {plantMoistureValue};
  float plantTemperatureValues[1] = {plantTemperature};
  
  String data = buildJSON(ambientHumidity,
                          ambientTemperature, 
                          ambientLight, 
                          plantMoistureValues, 
                          plantTemperatureValues, 
                          1);
                          
   //Send data to Carriots.
   sendRequestWithBody(data);
}

String deviceName = "";

String buildJSON( float ambientHumidity, 
                  float ambientTemperature, 
                  float ambientLight, 
                  float *moistureValues, 
                  float *temperatureValues, 
                  int   valuesCount) {
                    
  //Create carriots required fields
  String JSON = "{\"protocol\":\"v2\",\"at\":\"now\",\"checksum\":\"\",\"persist\":\"true\",\"device\":";
  JSON += "\""; JSON += deviceName; JSON += "\",";
  
  //Create data dict
  JSON += "\"data\":{";
  
    //Add ambient data
    JSON += "\"ambient\":{"; 
      JSON += "\"humidity\":"; JSON += ambientHumidity; JSON += ",";
      JSON += "\"temperature\":"; JSON += ambientTemperature; JSON += ",";
      JSON += "\"light\":"; JSON += ambientLight;
    JSON += "},";
    
    //Add plants data
    JSON += "\"plants\":[";
    for (int i = 0; i < valuesCount; i++) {
      JSON += "{\"id\":"; JSON += i; JSON += ",";
      JSON += "\"moisture\":"; JSON += moistureValues[i]; JSON += ",";
      JSON += "\"temperature\":"; JSON += temperatureValues[i];
      JSON += "}";
      if (i != valuesCount-1) {
        JSON += ",";
      }
    }
  
  JSON += "]}}";
  
  //Display result
  Console.println("Request built.");
  
  return JSON;
}

String API_KEY = "";

void sendRequestWithBody(String requestBody) {
  
  String apiKey = "carriots.apiKey: "; apiKey += API_KEY;

  String processableBody = "'";
  processableBody += requestBody;
  processableBody += "'";
  
  //Build curl command to send POST request.
  Console.println("Sending data... ");
  
  Process curl;
  curl.begin("curl");
  curl.addParameter("-k");
  curl.addParameter("--request");
  curl.addParameter("POST");
  curl.addParameter("--data");
  curl.addParameter(processableBody);
  curl.addParameter("--header");
  curl.addParameter(apiKey);
  curl.addParameter("http://api.carriots.com/streams/");
  curl.run();

  Console.println("Request sent.");

  // If there's incoming data from the net connection,
  // send it out the Console:
  while (curl.available() > 0) {
    char c = curl.read();
    Console.print(c);
  }
}

