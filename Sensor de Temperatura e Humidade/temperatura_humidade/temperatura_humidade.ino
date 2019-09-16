#include "DHT.h"
#define DHTPIN A1
#define DHTTYPE DHT11   // DHT 11

// Inicialização do Sensor
DHT dht(DHTPIN, DHT11);

void setup() {
  Serial.begin(9600);
  Serial.println("ÓPERA AUTOMAÇÃO");
  Serial.println();
  Serial.println("Leituras de Humidade e Temperatura");
  dht.begin();
}

void loop() {
  delay(1000);
  float h = dht.readHumidity();
  // Leutura da Temperatura em graus Celsius
  float t = dht.readTemperature();
//  // Read temperature as Fahrenheit (isFahrenheit = true)
//  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Falha na Leitura do Sensor!");
    return;
  }

//  // Compute heat index in Fahrenheit (the default)
//  float hif = dht.computeHeatIndex(f, h);
//  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Humidade: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperatura: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(" %\t");
  Serial.print("Sensação Térmica: ");
  Serial.print(hic);
  Serial.print(" *C ");
  Serial.println();
}
