


extern "C" {
#include <espnow.h>
}
#include "DHT.h"
#include <ESP8266WiFi.h>

//Definição dos Pinos do Sensor, Contador e Watchdog
#define DHTPIN 2
#define DHTTYPE DHT11   // DHT 11
#define LWD_TIMEOUT 10*1000 //Reinicia se o timer watchdog alcançar este valor
#define MAX_HUMIDITY 100
#define MIN_HUMIDITY 0
#define MAX_TEMPERATURE 50
#define ESPNOW_CHANNEL 1
#define INTERVAL 3000


//***ESTRUCTURA DE LOS DATOS TRANSMITIDOS MAESTRO/ESCLAVO***//
//Se de establecer IGUAL en el par esclavo
struct ESTRUCTURA_DATOS {
  float t;
  float h;
  float hic;
};

byte COUNTER = 0;

// Inicialização do Sensor
DHT dht(DHTPIN, DHT11);

void setup() {

  //***INICIALIZACIÓN DEL PUERTO SERIE***//
  Serial.begin(9600); Serial.println(); Serial.println();

  //***INICIALIZACIÓN DEL PROTOCOLO ESP-NOW***//
  if (esp_now_init() != 0) {
    Serial.println("*** ESP_Now init failed");
    ESP.restart();
    delay(1);
  }

  //***DATOS DE LAS MAC (Access Point y Station) del ESP***//
  Serial.print("Access Point MAC de este ESP: "); Serial.println(WiFi.softAPmacAddress());
  Serial.print("Station MAC de este ESP: "); Serial.println(WiFi.macAddress());

  //***DECLARACIÓN DEL PAPEL DEL DISPOSITIVO ESP EN LA COMUNICACIÓN***//
  //0=OCIOSO, 1=MAESTRO, 2=ESCLAVO y 3=MAESTRO+ESCLAVO
  esp_now_set_self_role(3);

  //***EMPAREJAMIENTO CON EL ESCLAVO***//
  // Dirección MAC del ESP con el que se empareja (esclavo)
  // Se debe introducir la STA MAC correspondiente
  /*
    ESP 8266 TEMPERATURA MAC
    AP MAC: 86:F3:EB:5A:4C:34
    STA MAC: 84:F3:EB:5A:4C:34

    ESP 32 MAC
    AP MAC: 30:AE:A4:87:E0:35
    STA MAC: 30:AE:A4:87:E0:34

  */
  uint8_t mac_addr[6] = {0x30, 0xAE, 0xA4, 0x87, 0xE0, 0x34};
  uint8_t role = 3;
  uint8_t channel = ESPNOW_CHANNEL;
  uint8_t key[0] = {}; //no hay clave
  //uint8_t key[16] = {0,255,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
  uint8_t key_len = sizeof(key);
  Serial.print("Tamaño de *key:  "); Serial.println(key_len);
  esp_now_add_peer(mac_addr, role, channel, key, key_len);
  dht.begin(); //Inicia o sensor
}

void loop() {

  //***DATOS A ENVIAR***//
  ESTRUCTURA_DATOS ED;

  ED.t = dht.readTemperature();
  ED.h = dht.readHumidity();
  if (isnan(ED.h) || isnan(ED.t)) {
    Serial.println("Falha na Leitura do Sensor!");
    return;
  }
  ED.hic = dht.computeHeatIndex(ED.t, ED.h, false);

  if (((ED.h >= MIN_HUMIDITY) && (ED.h <= MAX_HUMIDITY)) && (ED.t <= MAX_TEMPERATURE) && ((ED.hic >= MIN_HUMIDITY) && (ED.hic <= MAX_HUMIDITY))) {

    //***ENVÍO DE LOS DATOS***//
    //uint8_t *da=NULL;   //NULL envía los datos a todos los ESP con los que está emparejado
    uint8_t da[6] = {0x30, 0xAE, 0xA4, 0x87, 0xE0, 0x34};
    uint8_t data[sizeof(ED)]; memcpy(data, &ED, sizeof(ED));
    uint8_t len = sizeof(data);
    esp_now_send(da, data, len);

    delay(1); //Si se pierden datos en la recepción se debe subir este valor

    //***VERIFICACIÓN DE LA RECEPCIÓN CORRECTA DE LOS DATOS POR EL ESCLAVO***//
    esp_now_register_send_cb([](uint8_t* mac, uint8_t status) {
      char MACesclavo[6];
      sprintf(MACesclavo, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      Serial.print(". Enviado a ESP MAC: "); Serial.print(MACesclavo);
      Serial.print(". Recepcion (0=0K - 1=ERROR): "); Serial.println(status);
    });
  } else {
    COUNTER++;
    if (COUNTER > 5) {
      COUNTER = 0;
      ESP.restart();
    } else {
      delay(INTERVAL);
    }
  }
}
