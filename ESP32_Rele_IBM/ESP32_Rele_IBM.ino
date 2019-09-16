#include <FS.h>
#include <rom/rtc.h>
#include "esp_system.h"

//-----------------VARIÁVEIS WI-FI MANAGER--------------------//
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>

//------------------VARIÁVEIS BLUEMIX IOT--------------------//

const String ORG = "ewxuvw";
const String DEVICE_TYPE = "ESP32";
const String DEVICE_ID = "ESP32Dimmer";
#define DEVICE_TOKEN "k7DfObuOfXRRB1rLee"
const String CLIENT_ID = "d:" + ORG + ":" + DEVICE_TYPE + ":" + DEVICE_ID;
const String MQTT_SERVER = ORG + ".messaging.internetofthings.ibmcloud.com";
#define COMMAND_TOPIC_R1 "iot-2/cmd/commandR1/fmt/json"
WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER.c_str(), 1883, wifiClient);
WiFiManager wm;

//-----------VARIÁVEIS DE DIMMERIZAÇÃO E DADOS---------------//

#define LED             16 //pino do LED Touch
#define SSR             17 //pino do SSR
#define TOUCH           T3 //pino com sensor touch
#define THRESHOLD       30 //valor máximo da capacitância via calibragem, quanto maior mais sensível
#define TRIGGER_PIN     5 //Trigger do Portal de Wifi
#define RED_LED         18 //LED Verde
#define YELLOW_LED      19 //LED Amarelo 
#define GREEN_LED       21 //LED Vermelho
#define mS_TO_S_FACTOR  1000 /* Fator de conversão milisegundos para segundos */
RTC_DATA_ATTR int bootCount = 0; //variável de boot armazenado no RTC

//-----------------------WATCHDOG---------------------------//

const int wdtTimeout = 30 * mS_TO_S_FACTOR; //tempo em segundos para acionar o watchdog (30 seg)
hw_timer_t *timer0 = NULL;

void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart_noos();
}

//---------------------RESET REASON-------------------------//

void print_reset_reason(RESET_REASON reason)
{
  switch ( reason)
  {
    case 1 : Serial.println ("POWERON_RESET"); break;         /**<1,  Vbat power on reset*/
    case 3 : Serial.println ("SW_RESET"); break;              /**<3,  Software reset digital core*/
    case 4 : Serial.println ("OWDT_RESET"); break;            /**<4,  Legacy watch dog reset digital core*/
    case 5 : Serial.println ("DEEPSLEEP_RESET"); break;       /**<5,  Deep Sleep reset digital core*/
    case 6 : Serial.println ("SDIO_RESET"); break;            /**<6,  Reset by SLC module, reset digital core*/
    case 7 : Serial.println ("TG0WDT_SYS_RESET"); break;      /**<7,  Timer Group0 Watch dog reset digital core*/
    case 8 : Serial.println ("TG1WDT_SYS_RESET"); break;      /**<8,  Timer Group1 Watch dog reset digital core*/
    case 9 : Serial.println ("RTCWDT_SYS_RESET"); break;      /**<9,  RTC Watch dog Reset digital core*/
    case 10 : Serial.println ("INTRUSION_RESET"); break;      /**<10, Instrusion tested to reset CPU*/
    case 11 : Serial.println ("TGWDT_CPU_RESET"); break;      /**<11, Time Group reset CPU*/
    case 12 : Serial.println ("SW_CPU_RESET"); break;         /**<12, Software reset CPU*/
    case 13 : Serial.println ("RTCWDT_CPU_RESET"); break;     /**<13, RTC Watch dog Reset CPU*/
    case 14 : Serial.println ("EXT_CPU_RESET"); break;        /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : Serial.println ("RTCWDT_BROWN_OUT_RESET"); break; /**<15, Reset when the vdd voltage is not stable*/
    case 16 : Serial.println ("RTCWDT_RTC_RESET"); break;     /**<16, RTC Watch dog reset digital core and rtc module*/
    default : Serial.println ("NO_MEAN");
  }
}

void verbose_print_reset_reason(RESET_REASON reason)
{
  switch ( reason)
  {
    case 1  : Serial.println ("Vbat power on reset"); break;
    case 3  : Serial.println ("Software reset digital core"); break;
    case 4  : Serial.println ("Legacy watch dog reset digital core"); break;
    case 5  : Serial.println ("Deep Sleep reset digital core"); break;
    case 6  : Serial.println ("Reset by SLC module, reset digital core"); break;
    case 7  : Serial.println ("Timer Group0 Watch dog reset digital core"); break;
    case 8  : Serial.println ("Timer Group1 Watch dog reset digital core"); break;
    case 9  : Serial.println ("RTC Watch dog Reset digital core"); break;
    case 10 : Serial.println ("Instrusion tested to reset CPU"); break;
    case 11 : Serial.println ("Time Group reset CPU"); break;
    case 12 : Serial.println ("Software reset CPU"); break;
    case 13 : Serial.println ("RTC Watch dog Reset CPU"); break;
    case 14 : Serial.println ("for APP CPU, reseted by PRO CPU"); break;
    case 15 : Serial.println ("Reset when the vdd voltage is not stable"); break;
    case 16 : Serial.println ("RTC Watch dog reset digital core and rtc module"); break;
    default : Serial.println ("NO_MEAN");
  }
}

void setup()
{

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(3000);

  //Variáveis de Saída para Dimerização
  pinMode(LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(SSR, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  digitalWrite(SSR, HIGH);
  digitalWrite(RED_LED, HIGH);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  //Variáveis Timer do Watchdog
  timer0 = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer0, &resetModule, true);  //attach callback
  timerAlarmWrite(timer0, wdtTimeout * 1000, false); //set time in us
  timerAlarmEnable(timer0);                          //enable interrupt

  //Informações de Reset
  Serial.println("CPU0 reset reason:");
  print_reset_reason(rtc_get_reset_reason(0));
  verbose_print_reset_reason(rtc_get_reset_reason(0));

  Serial.println("CPU1 reset reason:");
  print_reset_reason(rtc_get_reset_reason(1));
  verbose_print_reset_reason(rtc_get_reset_reason(1));

  //Define o número de boots que foram executados
  ++bootCount;
  Serial.print("Número de boots executados:");
  Serial.println(bootCount);

  //Configurações do WIFI-Manager
  //  WiFiManager wm;
  int customFieldLength = 40;
  WiFiManagerParameter custom_field("customfield", "Custom Field", "Default Value", customFieldLength);
  wm.addParameter(&custom_field);
  std::vector<const char *> menu = {"wifi", "info", "param", "sep", "restart", "exit"};
  wm.setMenu(menu);
  wm.setClass("invert"); //Dark Theme
  wm.setRemoveDuplicateAPs(true);


  /*  CONFIGURAÇÕES DE IP ESTÁTICO
     ----------------
    wm.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); // set static ip,gw,sn
    wm.setShowStaticFields(true); // force show static ip fields
    wm.setShowDnsFields(true);    // force show dns field always

  */

  wm.setAPCallback(configModeCallback);
  wm.setSaveConfigCallback(saveConfigCallback);
  bool AP;
  AP = wm.autoConnect("ESP32 DIMMER", "password");
  /*
     Tenta se conectar a uma conexão salva, se não conseguir inicia um ponto de acesso
  */
  if (!AP) {
    Serial.println("failed to connect, we should reset as see if it connects");
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  } else {
    Serial.println("Conectado, endereço de IP:");
    Serial.println(WiFi.localIP());
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
  }
  connectMQTTServer();
}

void loop()
{

  timerWrite(timer0, 0); //reset timer (feed watchdog)
  long loopTime = millis();


  client.loop();
  touchSensor();
  WIFIReset();
  loopTime = millis() - loopTime;
}

void touchSensor() {

  //Serial.println(touchRead(Touch));
  int mediaT3 = 0;
  //faz 100 leituras do sensor touch e calcula a média do valor lido
  for (int i = 0; i < 100; i++)
  {
    mediaT3 += touchRead(TOUCH);
  }

  mediaT3 = mediaT3 / 100;

  if (mediaT3 < THRESHOLD) {
    digitalWrite(LED, HIGH);
    digitalWrite(SSR, LOW);
  } else {
    digitalWrite(LED, LOW);
    digitalWrite(SSR, HIGH);
  }
  delay(10);
}

void WIFIReset() {
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    delay(50); //debounce
    if ( digitalRead(TRIGGER_PIN) == LOW ) {
      delay(3000);
      if ( digitalRead(TRIGGER_PIN) == LOW ) {
        Serial.println("Erasing Config");
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(RED_LED, HIGH);
        wm.resetSettings();
        ESP.restart();
      }
      Serial.println("Starting config portal");
      wm.setConfigPortalTimeout(120);
      if (!wm.startConfigPortal("OnDemandAP")) {
        Serial.println("failed to connect or hit timeout");
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(RED_LED, HIGH);
        delay(3000);
      } else {
        digitalWrite(GREEN_LED, HIGH);
        digitalWrite(RED_LED, LOW);
        Serial.println("connected...yeey :)");
      }
    }
  }
}

//Função responsável pela conexão ao servidor MQTT
void connectMQTTServer() {
  Serial.println("Conectando ao servidor MQTT...");
  digitalWrite(RED_LED, LOW);

  if (client.connect(CLIENT_ID.c_str(), "use-token-auth", DEVICE_TOKEN)) {
    Serial.println("Conectado ao Broker MQTT...");
    digitalWrite(RED_LED, LOW);
    digitalWrite(YELLOW_LED, HIGH);
    client.setCallback(callback);
    client.subscribe(COMMAND_TOPIC_R1);
  } else {
    Serial.print("erro = ");
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    Serial.println(client.state());
    connectMQTTServer();
  }
}

void callback(char* topic, unsigned char* payload, unsigned int length) {
  Serial.print("topico ");
  Serial.println(topic);

  StaticJsonBuffer<30> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);

  if ( !root.success() ) {
    Serial.println("Erro no Json Parse");
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    return;
  }

  int value = root["value"];

  if (strcmp(topic, COMMAND_TOPIC_R1) == 0) {

    if ((value == 0)) {
      digitalWrite(SSR, LOW);
      delay(250);
      digitalWrite(SSR, HIGH);
    } else {
      digitalWrite(SSR, LOW);
      delay(150);
      digitalWrite(SSR, HIGH);
    }

  }
}

void configModeCallback( WiFiManager *myWiFiManager) {
  Serial.println("Entrou no modo de configuração");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void saveConfigCallback() {
  Serial.println("Configuração salva");
  Serial.println(WiFi.softAPIP());
}

