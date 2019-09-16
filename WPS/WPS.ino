#include <ESP8266WiFi.h>

void setup() {
  Serial.begin(9600);
  delay(1000);
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

void loop() {
  // put your main code here, to run repeatedly:

}
