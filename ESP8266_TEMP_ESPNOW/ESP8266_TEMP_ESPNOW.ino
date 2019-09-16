//Bibliotecas Utilizadas
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <PubSubClient.h>
#include "DHT.h"

//Watchdog
extern "C" {
#include "user_interface.h"
}
#include <Ticker.h>
Ticker lwdTicker;


//----------VARIÁVEIS GLOBAIS-----------//

//Definição dos Pinos do Sensor, Contador e Watchdog
#define DHTPIN 2
#define DHTTYPE DHT11   // DHT 11
#define LWD_TIMEOUT 10*1000 //Reinicia se o timer watchdog alcançar este valor
#define MAX_HUMIDITY 100
#define MIN_HUMIDITY 0
#define MAX_TEMPERATURE 50
#define ESPNOW_CHANNEL 1
#define INTERVAL 3000

//Definição das Variáveis ESP_NOW
/*
   ESP 8266 TEMPERATURA MAC
   AP MAC: 86:F3:EB:5A:4C:34
   STA MAC: 84:F3:EB:5A:4C:34

   ESP 32 MAC
   AP MAC: 30:AE:A4:87:E0:35
   STA MAC: 30:AE:A4:87:E0:34

*/

uint8_t slaveMacAddress[6] = {0x30, 0xAE, 0xA4, 0x87, 0xE0, 0x34}; //MAC em modo STA para se comportar como Mestre/Escravo (espressif)

byte COUNTER = 0;
byte COUNTER_NOW = 0;

struct SENSOR_DATA {
  float t;
  float h;
  float hic;
};

volatile boolean readingSent;
// Inicialização do Sensor
DHT dht(DHTPIN, DHT11);

//Definições do Watchdog
unsigned long lwdTime = 0;
unsigned long lwdTimeout = LWD_TIMEOUT;
//-------------------------------------//

//LWDTicker rotina de callback

void ICACHE_RAM_ATTR lwdtcb(void) {
  if ((millis() - lwdTime > LWD_TIMEOUT) || (lwdTimeout - lwdTime != LWD_TIMEOUT)) {
    //Outras funções podem ser implementadas aqui antes do restart
    ESP.restart();
  }
}

void lwdtFeed(void) {
  lwdTime = millis();
  lwdTimeout = lwdTime + LWD_TIMEOUT;
}

int getBootDevice(void) {
  int bootmode;
  asm (
    "movi %0, 0x60000200\n\t"
    "l32i %0, %0, 0x118\n\t"
    : "+r" (bootmode) /* Output */
    : /* Inputs (none) */
    : "memory" /* Clobbered */
  );
  return ((bootmode >> 0x10) & 0x7);
}

esp_now_peer_info_t slave;

void setup() {
  Serial.begin(9600); //Aguarda a abertura da serial
  Serial.printf("\n\nReason for reboot: %s\n", ESP.getResetReason().c_str()); //Motivo do restart do ESP
  Serial.println("----------------------------------------------");
  while (!Serial);
  lwdtFeed();
  lwdTicker.attach_ms(LWD_TIMEOUT, lwdtcb); // adiciona a rotina de callback do lwdt ao objeto Ticker
  dht.begin(); //Inicia o sensor
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  setupESPNow();

}


void loop() {
  lwdtFeed(); // inicia o watchdog ao loop
  sensorData();
  setupSlave();
  delay(INTERVAL);
  //  ESP.deepSleep(sleepTime * 60000000);//Dorme por X Minutos (Deep-Sleep em Micro segundos).
}


void setupESPNow() {
  //Se a inicialização foi bem sucedida
  if (esp_now_init() == 0) {
    Serial.println("ESPNow Inicializado com Sucesso!");
  }
  //Se houve erro na inicialização
  else {
    Serial.println("Falha de inicialização do ESPNow, reiniciando...");
    COUNTER_NOW++;
    setupESPNow();
    if (COUNTER_NOW == 3) {
      COUNTER_NOW = 0;
      ESP.restart();
    }
  }
  delay(1);
  Serial.print("Access Point MAC: "); Serial.println(WiFi.softAPmacAddress());
  Serial.print("Station MAC: "); Serial.println(WiFi.macAddress());
}

void setupSlave() {
  //0=OCIOSO, 1=MESTRE, 2=ESCRAVO y 3=COMBO (MESTRE+ESCRAVO)
  esp_now_set_self_role(1);
  slave.channel = ESPNOW_CHANNEL;
  //0 para não usar criptografia ou 1 para usar
  slave.encrypt = 0;
  //Copia o endereço do array para a estrutura do slave
  memcpy(slave.peer_addr, slaveMacAddress, 6);
  //Adiciona o slave
  esp_now_add_peer(&slave);
}

void sensorData() {

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Falha na Leitura do Sensor!");
    return;
  }

  float hic = dht.computeHeatIndex(t, h, false);

  if (((h >= MIN_HUMIDITY) && (h <= MAX_HUMIDITY)) && (t <= MAX_TEMPERATURE) && ((hic >= MIN_HUMIDITY) && (hic <= MAX_HUMIDITY))) {
    Serial.println(t);
    Serial.println(h);
    Serial.println(hic);
    
    //Dados que serão enviados
    SENSOR_DATA sd;
    sd.t = t;
    sd.h = h;
    sd.hic = hic;

    //Envio de dados
    //uint8_t *da=NULL;   //NULL envía los datos a todos los ESP con los que está emparejado
    uint8_t da[6] = {0x30, 0xAE, 0xA4, 0x87, 0xE0, 0x34 };
    uint8_t data[sizeof(sd)]; memcpy(data, &sd, sizeof(sd));
    uint8_t len = sizeof(data);
    esp_now_send(slaveMacAddress, data, len);

    delay(1);


    //***VERIFICACIÓN DE LA RECEPCIÓN CORRECTA DE LOS DATOS POR EL ESCLAVO***//
    esp_now_register_send_cb([](uint8_t* mac, uint8_t status) {
      char MACescravo[6];
      sprintf(MACescravo, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      Serial.print(". Enviado a ESP MAC: "); Serial.print(MACescravo);
      Serial.print(". Recepcion (0=0K - 1=ERROR): "); Serial.println(status);
    });
  } else {
    COUNTER++;
    if (COUNTER > 5) {
      COUNTER = 0;
      ESP.restart();
    }
    delay(5000);
  }
}

