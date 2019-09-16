//Bibliotecas Utilizadas

#include <ESP8266WiFi.h>
#include "DHT.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

//Watchdog
extern "C" {
#include "user_interface.h"
}
#include <Ticker.h>
Ticker lwdTicker;

//Definição dos Pinos do Sensor e Watchdog
#define DHTPIN 2
#define DHTTYPE DHT11   // DHT 11
#define LWD_TIMEOUT 10*1000 //Reinicia se o timer watchdog alcançar este valor

//Definições do Watchdog
unsigned long lwdTime = 0; 
unsigned long lwdTimeout = LWD_TIMEOUT;

//--------ADAFRUIT CONFIGURAÇÕES--------//

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "Ircbarros"
#define AIO_KEY "5b0262d51b874df3b4fc3c1062543ad0"

//----------VARIÁVEIS GLOBAIS-----------//

WiFiClient client;
DHT dht(DHTPIN, DHT11);
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish Temperatura = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Temperatura");
Adafruit_MQTT_Publish Humidade = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Humidade");
Adafruit_MQTT_Publish Sensacao = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Sensacao");

//-------------------------------------//

//LWDTicker rotina de callback

void ICACHE_RAM_ATTR lwdtcb(void){
  if ((millis() - lwdTime > LWD_TIMEOUT) || (lwdTimeout - lwdTime != LWD_TIMEOUT)){
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

void setup() {
  Serial.begin(9600); //Aguarda a abertura da serial
  Serial.printf("\n\nReason for reboot: %s\n", ESP.getResetReason().c_str()); //Motivo do restart do ESP
  Serial.println("----------------------------------------------");
  
  while(! Serial);
  lwdtFeed();
  lwdTicker.attach_ms(LWD_TIMEOUT, lwdtcb); // adiciona a rotina de callback do lwdt ao objeto Ticker
  dht.begin(); //Inicia o sensor

  //Inicia o WPS
  Serial.printf("\nIniciando a conexão... '%s'\n", WiFi.SSID().c_str());

  WiFi.mode(WIFI_STA);
  WiFi.begin(WiFi.SSID().c_str(),WiFi.psk().c_str()); // reading data from EPROM, last saved credentials
  while (WiFi.status() == WL_DISCONNECTED) {
    yield();
    Serial.print(".");
  }

  wl_status_t status = WiFi.status();
  if(status == WL_CONNECTED) {
    Serial.printf("\nConectado ao SSID:  '%s'\n", WiFi.SSID().c_str());
  } else {
    Serial.printf("\nNão foi possível conectar ao WI-FI. state='%d'", status); 
    Serial.println("Aperte o Botão WPS do seu roteador.\n Aperte alguma tecla para continuar...");
    while(!Serial.available()) { ; }
    if(!startWPSPBC()) {
       Serial.println("Falha ao conectar via WPS :-(");  
    }
  }

  //Conecta ao Adafruit IO
  Serial.print("\nIniciando a conexão ao Adafruit... '%s'\n");
  connect();

}


void loop() {
    lwdtFeed(); // inicia o watchdog ao loop
    if(! mqtt.ping()) {
    // reconnect to adafruit io
    if(! mqtt.connected())
      connect();
        }
        
  startTemp();

//  ESP.deepSleep(sleepTime * 60000000);//Dorme por X Minutos (Deep-Sleep em Micro segundos).
}

bool startWPSPBC() {
  Serial.println("Iniciando a conexão via WPS");
  bool wpsSuccess = WiFi.beginWPSConfig();
  if(wpsSuccess) {
      // Well this means not always success :-/ in case of a timeout we have an empty ssid
      String newSSID = WiFi.SSID();
      if(newSSID.length() > 0) {
        // WPSConfig has already connected in STA mode successfully to the new station. 
        Serial.printf("Conexão ao WPS Concluída. Conectado ao SSID:  '%s'\n", newSSID.c_str());
      } else {
        wpsSuccess = false;
      }
  }
  return wpsSuccess; 
}

void startTemp(){
  
  int h = (int)dht.readHumidity();
  int t = (int)dht.readTemperature();
  int hic = (int)dht.computeHeatIndex(t, h, false);
  Temperatura.publish(t);
  Humidade.publish(h);
  Sensacao.publish(hic);
  Serial.println(t);
  Serial.println(h);
  Serial.println(hic);
  
  //Envio dos dados para o feed
  if (! Temperatura.publish(t)){

   Serial.println(F("Falha ao publicar a temperatura"));
  }else{
    Serial.println(F("Temperatura publicada!"));
 }
  if (! Humidade.publish(h)){
   Serial.println(F("Falha ao publicar a temperatura"));
 }else{
  Serial.println(F("Temperatura publicada!"));
  }
  if (! Sensacao.publish(hic)){
    Serial.println(F("Falha ao publicar a Sensação Térmica"));
 }else{
    Serial.println(F("Sensação Térmica publicada!"));
  }
}

void connect() {

  Serial.print(F("Conectando ao Adafruit IO... "));

  int8_t ret;

  while ((ret = mqtt.connect()) != 0) {

    switch (ret) {
      case 1: Serial.println(F("Erro de Protocolo")); break;
      case 2: Serial.println(F("ID Rejeitado")); break;
      case 3: Serial.println(F("Servidor Indisponível")); break;
      case 4: Serial.println(F("Erro de usuário/senha")); break;
      case 5: Serial.println(F("Não autorizado")); break;
      case 6: Serial.println(F("Falha de subscribe")); break;
      default: Serial.println(F("Falha de conexão")); break;
    }

    if(ret >= 0)
      mqtt.disconnect();

    Serial.println(F("Retrying connection..."));
    delay(3000);

  }

  Serial.println(F("Adafruit IO Connected!"));

}

