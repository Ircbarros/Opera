
//-----------VARIÁVEIS DE DIMMERIZAÇÃO---------------//
int LED = 16; //pino do LED Touch
int SSR = 17; //pino do SSR
int Touch = T3; //pino com sensor touch
int Threshold = 40; //valor máximo da capacitância via calibragem, quanto maior mais sensível

//-------------------DEEPSLEEP----------------------//

RTC_DATA_ATTR int bootCount = 0;
touch_pad_t touchPin;

//Informa o motivo do Wakeup
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("Wakeup caused by timer"); break;
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
}

//Informa qual pino de Touch efetuou o wakeup
void print_wakeup_touchpad() {
  touch_pad_t pin;

  touchPin = esp_sleep_get_touchpad_wakeup_status();

  switch (touchPin)
  {
    case 0  : Serial.println("Touch detected on GPIO 4"); break;
    case 1  : Serial.println("Touch detected on GPIO 0"); break;
    case 2  : Serial.println("Touch detected on GPIO 2"); break;
    case 3  : Serial.println("Touch detected on GPIO 15"); break;
    case 4  : Serial.println("Touch detected on GPIO 13"); break;
    case 5  : Serial.println("Touch detected on GPIO 12"); break;
    case 6  : Serial.println("Touch detected on GPIO 14"); break;
    case 7  : Serial.println("Touch detected on GPIO 27"); break;
    case 8  : Serial.println("Touch detected on GPIO 33"); break;
    case 9  : Serial.println("Touch detected on GPIO 32"); break;
    default : Serial.println("Wakeup not by touchpad"); break;
  }
}

void callback() {
  Serial.println(touchRead(Touch));
  int mediaT3 = 0;
  //faz 100 leituras do sensor touch e calcula a média do valor lido
  for (int i = 0; i < 100; i++)
  {
    mediaT3 += touchRead(Touch);
  }

  mediaT3 = mediaT3 / 100;

  if (mediaT3 < Threshold) {
    digitalWrite(LED, HIGH);
    digitalWrite(SSR, LOW);
  } else {
    digitalWrite(LED, LOW);
    digitalWrite(SSR, HIGH);
  }
  delay(10);
}


void setup()
{

  Serial.begin(115200);
  delay(1000);

  //Variáveis de Saída para Dimerização
  pinMode(LED, OUTPUT);
  pinMode(SSR, OUTPUT);

  //Define o número de boots que foram executados
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Informa o motivo de wakeup do ESP32
  print_wakeup_reason();
  print_wakeup_touchpad();

  //Coloca o Interrupt no Pino de Touch T3
  touchAttachInterrupt(T3, callback, Threshold);

  //Configura o Pino T3 como input de Wakeup
  esp_sleep_enable_touchpad_wakeup();

  //Inicia o deepSleep
  esp_deep_sleep_start();

}

void loop()
{

}

