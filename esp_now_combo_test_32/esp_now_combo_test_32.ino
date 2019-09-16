/*
    Compiled for the NODEMCU-32S
*/

#include <esp_now.h>
#include <WiFi.h>

#define WIFI_CHANNEL 1

//***ESTRUCTURA DE LOS DATOS TRANSMITIDOS MAESTRO/ESCLAVO***//
//Se de establecer IGUAL en el par maestro
struct ESTRUCTURA_DATOS {
  float t = 0;
  float h = 0;
  float hic = 0;
};

void setup() {

  //***INICIALIZACIÓN DEL PUERTO SERIE***//
  Serial.begin(115200); Serial.println();
  delay(20);
  //Colocamos o ESP em modo station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  //***DATOS DE LAS MAC (Access Point y Station) del ESP***//
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
  //***INICIALIZACIÓN DEL PROTOCOLO ESP-NOW***//
  if (esp_now_init() != 0) {
    Serial.println("Protocolo ESP-NOW no inicializado...");
    ESP.restart();
    delay(1);
  }

  esp_now_register_recv_cb(OnDataRecv);

}

void OnDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  char MACmaestro[6];
  //Copiamos o Mac Address origem para uma string
  snprintf(MACmaestro, sizeof(MACmaestro), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  //Mostramos o Mac Address que foi a origem da mensagem
  Serial.print("Received from: ");
  Serial.println(MACmaestro);
  Serial.println("");

  ESTRUCTURA_DATOS ED;
  memcpy(&ED, data, sizeof(ED));

  Serial.print("Temperatura: "); Serial.print(ED.t);
  Serial.print("Humidade: "); Serial.println(ED.h);
  Serial.print("Sensação Térmica: "); Serial.println(ED.hic);
}

void loop() {
  //Registra o callback que nos informará quando o Master enviou algo
  //A função que será executada é OnDataRecv e está declarada mais abaixo

}
