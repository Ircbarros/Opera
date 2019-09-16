

//--------------------VARIÁVEIS GLOBAIS-----------------------//
#include <FS.h>
#include <rom/rtc.h>
#include <esp_now.h>
#include "esp_system.h"
#include <WiFi.h>


struct SENSOR_DATA {
  float t = 0;
  float h = 0;
  float hic = 0;
};


//-----------VARIÁVEIS DE DIMMERIZAÇÃO E DADOS---------------//

#define LED             16 //pino do LED Touch
#define SSR             17 //pino do SSR
#define TOUCH           T3 //pino com sensor touch
#define THRESHOLD       30 //valor máximo da capacitância via calibragem, quanto maior mais sensível
#define TRIGGER_PIN     5 //Trigger do Portal de Wifi
#define RED_LED         18 //LED Vermelho
#define YELLOW_LED      19 //LED Amarelo 
#define GREEN_LED       21 //LED Verde
#define mS_TO_S_FACTOR  1000 /* Fator de conversão milisegundos para segundos */
#define ESPNOW_CHANNEL 1
#define INTERVAL 3000
RTC_DATA_ATTR int bootCount = 0; //variável de boot armazenado no RTC
byte COUNTER_NOW = 0;

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
  WiFi.mode(WIFI_STA);
  delay(3000);

  //Variáveis de Saída para Dimerização
  pinMode(LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(SSR, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  digitalWrite(SSR, HIGH);
  digitalWrite(RED_LED, LOW);
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
  setupESPNow();

}

void loop()
{

  timerWrite(timer0, 0); //reset timer (feed watchdog)
  long loopTime = millis();
  esp_now_register_recv_cb([](const uint8_t *mac, const uint8_t *data, int len) {
    //Variáveis de Recepção ESP-NOW

    char MACmestre[6];
    //Copiamos o Mac Address origem para uma string
    snprintf(MACmestre, sizeof(MACmestre), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print("Recepção do Mestre: "); Serial.print(MACmestre);

    SENSOR_DATA sd;
    memcpy(&sd, data, sizeof(sd));

    Serial.print(". Temperatura: "); Serial.print(sd.t);
    Serial.print(". Humidade: "); Serial.println(sd.h);
    Serial.print(". Sensação Térmica: "); Serial.println(sd.hic);
  });
  // touchSensor();
  loopTime = millis() - loopTime;

}

void setupESPNow() {
  //Se a inicialização foi bem sucedida
  if (esp_now_init() == 0) {
    Serial.println("ESPNow Inicializado com Sucesso!");
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
    Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
  }
  //Se houve erro na inicialização
  else {
    Serial.println("Falha de inicialização do ESPNow, reiniciando...");
    COUNTER_NOW++;

    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);

    setupESPNow();

    if (COUNTER_NOW == 3) {
      digitalWrite(YELLOW_LED, HIGH);
      COUNTER_NOW = 0;
      ESP.restart();
      delay(1);
    }
  }
  delay(1);
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





