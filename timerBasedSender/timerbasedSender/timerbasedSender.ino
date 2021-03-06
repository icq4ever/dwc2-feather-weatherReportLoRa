#include <SPI.h>
#include <RH_RF95.h>
#include "DHT.h"

// define pinouts
const int PIN_LED     = 13;
const int PIN_DHT     = 5;
const int PIN_CHG     = 6;
const int PIN_GOOD    = 9;
const int PIN_LED_CHG = 11;
const int PIN_LED_GOD = 12;

// DHT setup
#define DHTTYPE DHT22
DHT dht(PIN_DHT, DHTTYPE);

// LoRa message/buffer setup
const int REQ_MESSAGE_SIZE = 2;
const int REQ_BUFFER_SIZE = REQ_MESSAGE_SIZE + 2;

const int REPLY_MESSAGE_SIZE = 4;

const int RECV_MAX_BUFFER_SIZE = 20;
uint8_t recvBuffer[RECV_MAX_BUFFER_SIZE];

// LoRa SPI / freq Setting
const int RFM95_CS    = 8;
const int RFM95_RST   = 4;
const int RFM95_INT   = 7;
const float RF95_FREQ = 900.0;

RH_RF95 rf95(RFM95_CS, RFM95_INT);

uint64_t timer;

typedef union{
  float numFloat;
  byte numBin[4];
} floatingNumber;

floatingNumber temperature, humidity;
bool isCharging = false;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_CHG, INPUT);
  pinMode(PIN_LED_GOD, OUTPUT);
  pinMode(PIN_LED_CHG, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  temperature.numFloat = 0.f;
  humidity.numFloat = 0.f;

  delay(1000);
  Serial.println("BOOTING....");

  dht.begin();
  initLoRa();
  timer = millis();
}

void loop() {
  // put your main code here, to run repeatedly:
  if(millis() - timer > 1000){
    getWeatherData();
//    printWeatherData();
    sendMessage();
    timer = millis();
  }

  if(digitalRead(PIN_GOOD)) digitalWrite(PIN_LED_GOD, LOW);
  else                      digitalWrite(PIN_LED_GOD, HIGH);

  if(digitalRead(PIN_CHG))  {
    digitalWrite(PIN_LED_CHG, LOW);
    isCharging = false;
  }
  else {
    digitalWrite(PIN_LED_CHG, HIGH);
    isCharging = true;
  }

//  Serial.print(digitalRead(PIN_GOOD));
//  Serial.print(" - ");
//  Serial.println(digitalRead(PIN_CHG));
}

void initLoRa(){ // init
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.println("Feather LoRa RX Test!");

  // manual LoRa rest
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while(!rf95.init()){
    Serial.println("LoRa radio init failed");
    while(1);
  }

  Serial.println("LoRa radio init OK!");

  if(!rf95.setFrequency(RF95_FREQ)){
    Serial.println("setFrequency failed.");
    while(1);
  }

  Serial.print("Set Freq to : ");
  Serial.println(RF95_FREQ);

  // default transmitter power is 13dbm, using PA_BOOST
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
}

void getWeatherData(){
  temperature.numFloat = dht.readTemperature();
  humidity.numFloat = dht.readHumidity();
  digitalWrite(PIN_LED, HIGH);
}

void printWeatherData(){
  Serial.print(temperature.numFloat);
  Serial.print("C,\t");
  Serial.print(humidity.numFloat);
  Serial.println("%");
}

void sendMessage(){
  uint8_t reply[7];
  int tempVal, humidVal;
  tempVal = int(temperature.numFloat*100);
  humidVal = int(humidity.numFloat*100);
  reply[0] = '/';
  reply[1] = (tempVal >> 8) & 0xff;
  reply[2] = tempVal & 0xff;
  reply[3] = (humidVal >> 8) & 0xff;
  reply[4] = humidVal & 0xff;
  if(isCharging)  reply[5] = 1;
  else            reply[5] = 0;
  reply[6] = 0;
  
  rf95.send(reply, sizeof(reply));
  digitalWrite(PIN_LED, LOW);
//  Serial.println("weatherData Sent!");
  printWeatherData();
}
