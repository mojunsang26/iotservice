#include <string.h>
#define MAX_BUF_SIZE 1024


char delimeter[10] = "\n ";
char* resetLog = "DevReset";

void checkResetLog();
int readLog(char* myLog);

void setup() {
  //Initialize
  Serial.begin(115200);

  //SetClass C
  Serial.println("LRW 4B 2");
  //ack 확인(??)
  delay(10000);
}

void loop() {

  checkResetLog();
  
  Serial.println("LRW 31 1234567890 cnf 1");

  delay(11000);  //권장 주기 : 1분  //최소 주기 : 10초
}


int readLog(char* myLog){
  int i = 0;
  while(Serial.available() != 0){
    myLog[i] = Serial.read();
    i++;  
  }
  return i;
}


void checkResetLog(){
  char downLinkLog[MAX_BUF_SIZE] = {0,};

  if(readLog(downLinkLog) != 0){
      char *myToken;
      char *pch = strtok(downLinkLog,delimeter);
      while(pch)
      {
          if(strcmp(pch, resetLog)  ==  0){
                Serial.println("LRW 31 PEM Device Reset! cnf 1");
                delay(5000); //device reset command에 대한 ack를 보내는 시간
                Serial.println("LRW 70");
                break;
           }
           else{
                pch = strtok(NULL,delimeter);
           }
      }
   }
}

