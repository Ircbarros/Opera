
/* SENSOR DE TEMPERATURA DHT11 COM BLUEMIX - ALPHA 0.1
 * Organization ID: mv458f
   Device Type: ESP8266
   Device ID: 2C3AE827445E (MAC)
   Authentication Method: use-token-auth
   Authentication Token: &nBsdqhXqJ!eXf?jgd
 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 2
#define DHTTYPE DHT11   // DHT 11

//--------BLUEMIX CONFIGURAÇÕES--------//

#define ORG "quickstart"
#define DEVICE_TYPE "ESP8266"
#define DEVICE_ID "2C3AE827445E"
#define TOKEN "&nBsdqhXqJ!eXf?jgd"

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char topic[] = "iot-2/evt/status/fmt/json";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

WiFiClient wifiClient;
PubSubClient client(server, 1883, NULL, wifiClient);

//-------------------------------------//

DHT dht(DHTPIN, DHT11);


void setup() {

  Serial.begin(9600);
  Serial.println();
  dht.begin();
  
  //WPS
  Serial.printf("\nIniciando a conexão... '%s'\n", WiFi.SSID().c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(WiFi.SSID().c_str(),WiFi.psk().c_str()); // reading data from EPROM, last saved credentials
  while (WiFi.status() == WL_DISCONNECTED) {
    delay(500);
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
  //FIM DO WPS
  
}


void loop() {
  
  // Verifica se está conectada a cloud para envio dos dados
  if (!!!client.connected()) {
   // Caso não esteja conectada, tenta a conexão
    Serial.print("Reconectando-se ao servidor: ");
    Serial.println(server);
    while (!!!client.connect(clientId, authMethod, token)) {
      Serial.print(".");
      delay(500);
    }
    Serial.println();
  }

  // Função temperatura
  startTemp();
  delay(2000);

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
  delay(500);
  float h = dht.readHumidity();
  // Leitura da Temperatura em graus Celsius
  float t = dht.readTemperature();
  // Observa se houve falha de leitura no sensor
  if (isnan(h) || isnan(t)) {
    Serial.println("Falha na Leitura do Sensor!");
    return;
  }
  float hic = dht.computeHeatIndex(t, h, false);

  // Formata a string que será envia para a cloud (JSON)
  String temperatura = "{\"d\":{\"Temperatura\":\"18FE34D81E46\"";
  temperatura += t;
  temperatura += "}}";
 
  Serial.print("Enviando temperatura: ");
  Serial.println(temperatura);

 
  // Enviando o dado
  if (client.publish(topic, (char*) temperatura.c_str())) {
    Serial.println("Publish ok");
  } else {
    Serial.println("Publish failed");
  }
  
  
}


//  String humidade = "{\"d\":{\"Humidade\":\"18FE34D81E46\"";
//  humidade += h;
//  humidade += "}}";
// 
//  Serial.print("Enviando Humidade: ");
//  Serial.println(humidade);
//
//   
//  // Enviando o dado
//  if (client.publish(topic, (char*) humidade.c_str())) {
//    Serial.println("Publish ok");
//  } else {
//    Serial.println("Publish failed");
//  }
//
//  String sensacao = "{\"d\":{\"Sensacao\":\"18FE34D81E46\"";
//  sensacao += hic;
//  sensacao += "}}";
// 
//  Serial.print("Enviando Sensação Térmica: ");
//  Serial.println(sensacao);
//  
// 
//  // Enviando o dado
//  if (client.publish(topic, (char*) sensacao.c_str())) {
//    Serial.println("Publish ok");
//  } else {
//    Serial.println("Publish failed");
//  }
//
//  delay(100);
//}

