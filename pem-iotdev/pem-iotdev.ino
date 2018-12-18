#include<stdio.h>
#include<time.h>
#include<TinyGPS.h>

TinyGPS gps;
//HardwareSerial Serial1(1); // GPS Module
#define SERIAL1_RXPIN 23
#define SERIAL1_TXPIN 22
//HardwareSerial Serial2(2); // pin 16=RX, pin 17=TX (LoRa Module)
char fromPC[2000];
char fromLoRa[6000];
char fromLoRa2[4000];
char fromLoRa3[4000];
char fromLoRa4[4000];
clock_t clk;
int joinCheck = 1;

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN);
  Serial2.begin(115200);
  Serial2.setRxBufferSize(6000);
  pinMode(2,OUTPUT); // LoRaRESET
  delay(1000);
  pinMode(2,INPUT);
  Serial.println("Setup is completed");
}


void loop() {
  //Serial.println("[MCU DEBUG] LOOP START");
  int i = 0;
  if(Serial.available())
  {
    i=0;
    while(Serial.available())
    {
      fromPC[i] = Serial.read();
      i++;
    }
    fromPC[i] = 0;
    OTBTest(fromPC,Serial2);
  }

  if(Serial2.available())
  {
    i=0;
    while(Serial2.available())
    {
      fromLoRa[i] = Serial2.read();
      fromLoRa2[i] = fromLoRa[i];
      fromLoRa3[i] = fromLoRa2[i];
      i++;
    }
    Serial2.flush();
    fromLoRa[i] = 0;
    fromLoRa2[i] = 0;
    fromLoRa3[i] = 0;
    Serial.println(fromLoRa);

    //Downlink DevReset
    if(checkReset(fromLoRa))
    {
      delay(10000);
      pinMode(0,OUTPUT); // MCU RESET
      delay(500);
      pinMode(0,INPUT);
      //delay(15000);
      //Serial2.println("LRW 70"); //LoRa Module system software reset
    }
    if(checkUplink(fromLoRa2))
    {
      clk = clock();
    }
    if(joinCheck == 0 && checkMyJoin(fromLoRa3))
    {
      delay(30000);
      Serial.println("[MCU DEBUG] JOIN BIT 0->1");
      joinCheck = 1;
      
    }
    if(checkInit(fromLoRa4))
    {
      Serial.println("[MCU DEBUG] JOIN BIT 1->0");
      joinCheck = 0;
    }
  }
  

  //Read Location
  if(sendLocGPS(gps,Serial1, Serial2) == false){
    if(sendLocWiFi() == false){
      if(sendLocBLE() == false){
        Serial.println("[MCU DEBUG] ERROR : Can't read Loc");
      }
    }
  }
  delay(300);
}


bool sendLocGPS(TinyGPS &gps,HardwareSerial gpsModule, HardwareSerial LoRa)
{
  while(gpsModule.available())
  {
    gps.encode(gpsModule.read());
  }
  char gpsInfo[300] = {NULL,};
  strcat(gpsInfo, "LRW 31 ");
  
//Append Datetime
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  char sz[32];
  sprintf(sz, "%02d/%02d/%02d#%02d:%02d:%02d#", year, month, day, hour, minute, second);
  strcat(gpsInfo, sz);

//Append Position&Speed
  float flat, flon;
  gps.f_get_position(&flat, &flon, &age);
  float fmps = gps.f_speed_mps();
  char pos[64];
  sprintf(pos, "%f#%f#%f", flat, flon, fmps);
  strcat(gpsInfo, pos);
  
  strcat(gpsInfo, " cnf 1");
//LoRa Uplink Msg
  if(joinCheck == 1 && (clk==NULL || (clock()-clk)/1000 > 20)){ 
    LoRa.println(gpsInfo); clk = clock(); 
    Serial.print("[MCU DEBUG]");
    Serial.println(gpsInfo);
  }
//TODO: implement not available
  return true;
}



void OTBTest(char* fromPC, HardwareSerial LoRa)
{
  int temp = atoi(fromPC);
  switch(temp)
  {
    case 0://CLI Command from Serial Monitor
        LoRa.println(fromPC);
        delay(5000);
        break;
    case 1://Data Send 65 Bytes
        if(clk==NULL || (clock()-clk)/1000 > 15) { send65Bytes(); clk = clock(); }
        else Serial.println("[ERROR] uplink time error");
        break;
    case 2://Data Send 66 Bytes
        if(clk==NULL || (clock()-clk)/1000 > 15) { send66Bytes(); }
        else Serial.println("[ERROR] uplink time error");
        break;
    case 3://Link Check Request
        LoRa.println("LRW 38");
        delay(3000);
        break;
    case 4://Device Time Request
        LoRa.println("LRW 39");
        delay(3000);
        break;
    default:
        //NYI
        Serial.print("[MCU DEBUG] ERROR : ");
        Serial.write(fromPC);
  }
}

bool sendLocWiFi()
{
  //NYI
  return false;
}

bool sendLocBLE()
{
  //NYI
  return false;
}

bool checkReset(char* str){
  int cnt = 0;
  char* tokens[1000] = { NULL, };
  char* delimiter = " ,.-:[]/\n\r";
  char* ptr = strtok(str,delimiter);
  
  int i = 0;
  while (ptr != NULL)               
    {
      tokens[cnt++] = ptr;
      ptr = strtok(NULL, delimiter);
    }
    
  char* keyword = "008000";
  //char* keyword = "BUSY";
  
  for (i = 0; i < cnt; i++){
    if (tokens[i] != NULL){
      if(strcmp(tokens[i],keyword) == 0){
        return true;
      }
    }
  }

  return false;
}

bool checkUplink(char* str){
  int cnt = 0;
  char* tokens[1000] = { NULL, };
  char* delimiter = " \n\r";
  char* ptr = strtok(str,delimiter);
  
  int i = 0;
  while (ptr != NULL)               
    {
      tokens[cnt++] = ptr;
      ptr = strtok(NULL, delimiter);
    }
    
  char* keyword = "DataReport";

  for (i = 0; i < cnt; i++){
    if (tokens[i] != NULL){
      if(strcmp(tokens[i],keyword) == 0){
        return true;
      }
    }
  }

  return false;
}

bool checkMyJoin(char* str){
  int cnt = 0;
  char* tokens[1000] = { NULL, };
  char* delimiter = "\n\r";
  char* ptr = strtok(str,delimiter);
  
  int i = 0;
  while (ptr != NULL)               
    {
      tokens[cnt++] = ptr;
      ptr = strtok(NULL, delimiter);
    }
    
  char* keyword = "Join is completed";

  for (i = 0; i < cnt; i++){
    if (tokens[i] != NULL){
      if(strcmp(tokens[i],keyword) == 0){
        return true;
      }
    }
  }

  return false;
}

bool checkInit(char* str){
  int cnt = 0;
  char* tokens[1000] = { NULL, };
  char* delimiter = "\n\r";
  char* ptr = strtok(str,delimiter);
  int i = 0;
  while (ptr != NULL)               
    {
      tokens[cnt++] = ptr;
      ptr = strtok(NULL, delimiter);
    }
  char* keyword = "Enhanced Provisioning status : None ";
  for (i = 0; i < cnt; i++){
    if (tokens[i] != NULL){
      if(strcmp(tokens[i],keyword) == 0){
        return true;
      }
    }
  }
  return false;
}

void send65Bytes(){
  Serial.println("[MCU DEBUG] Send 65bytes");
  Serial2.println("LRW 31 12345678901234567890123456789012345678901234567890123456789012345 cnf 1");
  delay(3000);
}

void send66Bytes(){
  Serial.println("[MCU DEBUG] Send 66bytes");
  Serial2.println("LRW 31 123456789012345678901234567890123456789012345678901234567890123456 cnf 1");
  delay(3000);
}
