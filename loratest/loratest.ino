#include<stdio.h>
#include<stdlib.h>


//HardwareSerial Serial1(1); // GPS Module
#define SERIAL1_RXPIN 23
#define SERIAL1_TXPIN 22
//HardwareSerial Serial2(2); // pin 16=RX, pin 17=TX (LoRa Module)
char fromPC[2000];
char fromLoRa[2000];

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN);
  Serial2.begin(115200);
  pinMode(2,OUTPUT);
  delay(10);
 pinMode(2,INPUT);
  delay(1000);
  Serial2.println("LRW 3C 1 CR NF");
}


void loop() {
  /*
Serial2.println("LRW 33 0000000000000003 CR NF");

while(Serial2.available()){
  Serial.write(Serial2.read());
}
Serial.println("hello");
delay(3000);

Serial2.println("LRW 51 60f3837116c091a9c73f9b7c6d511147 CR NF");

while(Serial2.available()){
  Serial.write(Serial2.read());
}
Serial.println("hello");
delay(3000);

Serial2.println("LRW 40 CR NF");

while(Serial2.available()){
  Serial.write(Serial2.read());
}
Serial.println("hello");
delay(3000);

Serial2.println("LRW 52 CR NF");

while(Serial2.available()){
  Serial.write(Serial2.read());
}
Serial.println("hello");
delay(3000);
Serial2.println("LRW 4C CR NF");
*/
while(Serial2.available()){
  Serial.write(Serial2.read());
}
delay(3000);
}
