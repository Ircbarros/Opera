
//-----------VARIÁVEIS DE DIMMERIZAÇÃO---------------//

int LED     = 16; //pino do LED Touch
int SSR     = 17; //pino do SSR
int Touch        = T3; //pino com sensor touch
int CapacitanciaMaxima = 40; //valor máximo da capacitância via calibragem

//--------------------------------------------------//

void setup()
{

  Serial.begin(115200);
  delay(1000);
  pinMode(LED, OUTPUT);
  pinMode(SSR, OUTPUT);
  
}

void loop()
{
  
  Serial.println(touchRead(Touch));
  int mediaT3 = 0;
  //faz 100 leituras de cada sensor touch e calcula a média do valor lido
  for (int i = 0; i < 100; i++)
  {
    mediaT3 += touchRead(Touch);
  }

  mediaT3 = mediaT3 / 100;

  /*verifica se o valor médio lido no pinoTouchOn é menor que a capacitância máxima
     definida e maior que a mínima, acionando o relé SSR
  */
  if (mediaT3 < CapacitanciaMaxima)
  {
    digitalWrite(LED, HIGH);
    digitalWrite(SSR, LOW);
  }
  else {
    digitalWrite(LED, LOW);
    digitalWrite(SSR, HIGH);
  }
  delay(10);
  
}
