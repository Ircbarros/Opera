#include <CayenneMQTTSerial.h>
#include "DHT.h"

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "1ad28970-5b65-11e8-9b75-e32a21a125ca";
char password[] = "695c56e6c41844636a03ce7228ffb5a45962e11b";
char clientID[] = "cd0cf500-7f80-11e8-8ba1-bbce60414d6e";

#define DHTPIN A1
#define VIRTUAL_TEMP V1
#define VIRTUAL_HUM V2
#define VIRTUAL_SM V3
#define VIRTUAL_GRAPH V4
#define DHTTYPE DHT11   // DHT 11


// Inicialização do Sensor
DHT dht(DHTPIN, DHT11);

void setup()
{
  Serial.begin(9600);
  Cayenne.begin(username, password, clientID);
  dht.begin();
  
}

void loop() {
  Cayenne.loop();
}


CAYENNE_OUT(VIRTUAL_TEMP)
{
    float t = dht.readTemperature();    
    Cayenne.virtualWrite(V1, t, "analog_sensor", "c");
}

CAYENNE_OUT(VIRTUAL_HUM)
{
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    Cayenne.virtualWrite(V2, h, "rel_hum", "p");
}

CAYENNE_OUT(VIRTUAL_SM)
{
    float t = dht.readTemperature(); 
    float h = dht.readHumidity();
    float hic = dht.computeHeatIndex(t, h, false);
    Cayenne.virtualWrite(V3, hic, "analog_sensor", "c");

}

CAYENNE_OUT(VIRTUAL_GRAPH)
{
    float t = dht.readTemperature(); 
    Cayenne.virtualWrite(V4, t, "analog_sensor", "c");

}
