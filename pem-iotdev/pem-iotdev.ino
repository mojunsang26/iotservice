#include <stdio.h>
#include <time.h>
#include <TinyGPS.h>
#include <string.h>

#define SERIAL1_RXPIN 23
#define SERIAL1_TXPIN 22
#define PINNUM_MCURST 0
#define PINNUM_LORARST 2
#define PINNUM_WKUP1 4
#define MAINLOOP_TIME 3000
#define NUM_TRY_TX 8
#define UPLINK_TIME 60000
#define TX_TIME 12000
#define TRIGGER_DELAY 1
#define TX_TO_RX_DELAY 3000
#define RX_BUF_SIZE 6000
#define MSG_65_BYTES "12345678901234567890123456789012345678901234567890123456789012345"
#define MSG_66_BYTES "123456789012345678901234567890123456789012345678901234567890123456"

#define SET_ACTIVATION_MODE "LRW 30"
#define LORA_CON_SEND "LRW 31 %s cnf 1"
#define LINK_CHECK_REQ "LRW 38"
#define TIME_SYNC_REQ "LRW 39"
#define ENHANCED_PROVISIONING_1 "LRW 3C"
#define ENHANCED_PROVISIONING_2 "LRW 3B"
#define SET_CLASS_TYPE "LRW 4B"
#define SET_UPLINK_CYCLE "LRW 58"
#define SYSTEM_SOFTWARE_RESET "LRW 70"
#define SET_UART_BAUDRATE "LRW 61"
#define SET_ONESECONDDELAY "LRW 2E"


#define LORA_ACK "FRAME_TYPE_DATA_UNCONFIRMED_DOWN"
#define LORA_CONFIRMED_DOWN "FRAME_TYPE_DATA_CONFIRMED_DOWN"
#define LORA_CLI_OK "OK"
#define LORA_CLI_ERROR "ERROR"
#define LORA_JOINED "Join is completed"
#define RX2CH_OPEN "RX2CH Open"
#define LORA_DEVRESET "DevReset"
clock_t CLK;
clock_t CLK_TX;

void check_pc_command(void);
void lora_reset(void);
void read_and_print_downlink_msg(char* downlink_msg, HardwareSerial module);
void set_class(int cls);
void send_handler(char * buf);
bool send_packet_and_check_rsp(char* packet_buf, char* check_rsp);
bool parsing_downlink_msg(char* str, char* check_rsp, int& rx2ch_open_cnt);

void setup() {
  Serial.begin(115200); //Host PC
  Serial1.begin(9600, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); //GPS Module
  Serial2.begin(115200); //LoRa Module
  Serial2.setRxBufferSize(RX_BUF_SIZE - 10);
  lora_reset();
  CLK = clock();
  CLK_TX = clock();
}

void loop() {
  if(Serial.available())
    check_pc_command();
    
  if(clock()-CLK > UPLINK_TIME && clock()-CLK_TX > TX_TIME){
    char packet_buf[128];
    char gps_buf[128];
    if(payload_GPS_Module(gps_buf) == false){
      //NYI
    }
    sprintf(packet_buf, LORA_CON_SEND, gps_buf);
    if(send_packet_and_check_rsp(packet_buf, LORA_ACK) == true){
      CLK = clock();
      CLK_TX = clock();
    }
  }
  delay(MAINLOOP_TIME);
}

void check_pc_command(void){
  char fromPC[128];
  read_and_print_downlink_msg(fromPC, Serial);
  int temp = atoi(fromPC);
  switch(temp)
  {
    case 0://CLI Command from Serial Monitor
        if(invoke_reset(fromPC)){
          send_packet_and_check_rsp(fromPC, LORA_JOINED);
          CLK_TX = clock();
          CLK = clock();
        }
        
        else if(strstr(fromPC, "LRW 31") != 0){
          if(clock() - CLK_TX < TX_TIME)
            Serial.println("[ERROR] uplink time error");
          else{
            send_packet_and_check_rsp(fromPC, LORA_ACK);
            CLK_TX = clock();
          }
        }
        
        else
          send_packet_and_check_rsp(fromPC, LORA_CLI_OK);

        break;
        
    case 1://Data Send 65 Bytes
    {
        char packet_buf[128];
        if(clock() - CLK_TX > TX_TIME) {
          sprintf(packet_buf, LORA_CON_SEND, MSG_65_BYTES);
          if(send_packet_and_check_rsp(packet_buf, LORA_ACK) == true)
            CLK_TX = clock();
        }
        
        else 
          Serial.println("[ERROR] uplink time error");
          
        break;
    }
    case 2://Data Send 66 Bytes
        if(clock() - CLK_TX > TX_TIME)
          send_packet_and_check_rsp(MSG_66_BYTES, LORA_CLI_ERROR); 
          
        else 
          Serial.println("[ERROR] uplink time error");
          
        break;
        
    case 3://Link Check Request
        if(clock() - CLK_TX > TX_TIME){
          send_packet_and_check_rsp(LINK_CHECK_REQ, LORA_CLI_OK);
          delay(3000);
        }
        else
          Serial.println("[ERROR] uplink time error");
        break;
        
    case 4://Device Time Request
        if(clock() - CLK_TX > TX_TIME){
        send_packet_and_check_rsp(TIME_SYNC_REQ, LORA_CLI_OK);
        delay(3000);
        }
        else
          Serial.println("[ERROR] uplink time error");
        break;
        
    default:
        //NYI
        Serial.print("[MCU DEBUG] ERROR : ");
        Serial.write(fromPC);
  }
  Serial.flush();
}

bool invoke_reset(char * str){
  bool ret = false;
  if(strstr(str, SET_ACTIVATION_MODE)) ret = true;
  else if(strstr(str, ENHANCED_PROVISIONING_1)) ret = true;
  else if(strstr(str, ENHANCED_PROVISIONING_2)) ret = true;
  else if(strstr(str, SET_CLASS_TYPE)) ret = true;
  else if(strstr(str, SET_UPLINK_CYCLE)) ret = true;
  else if(strstr(str, SYSTEM_SOFTWARE_RESET)) ret = true;
  else if(strstr(str, SET_UART_BAUDRATE)) ret = true;
  else if(strstr(str, SET_ONESECONDDELAY)) ret = true;
  return ret;
}

void lora_reset(){
  pinMode(PINNUM_LORARST,OUTPUT);
  delay(TRIGGER_DELAY);
  pinMode(PINNUM_LORARST,INPUT);
  delay(1000);
  bool ret = false;
  int rx2ch_open_cnt = 0;
  while(ret == false){
    char downlink_msg[RX_BUF_SIZE];
    read_and_print_downlink_msg(downlink_msg, Serial2);
    ret = parsing_downlink_msg(downlink_msg, LORA_JOINED, rx2ch_open_cnt);
    delay(1000);
  }
  rx2ch_open_cnt = 0;
  //set_class(0);
  //set_adr(0);
  //set_dr(0);
  //set_con_retrans(8);
}

void read_and_print_downlink_msg(char* downlink_msg, HardwareSerial module){
  if(!module.available())
    return;
    
  int i=0;
  while(module.available()){
    downlink_msg[i] = module.read();
    i++;
  }
  downlink_msg[i] = 0;
  Serial.println(downlink_msg);
}

void set_class(int cls){
//  int ret;
//  char get_cls, buf[30];
//
//  ret = send_and_check_command(LORA_GET_CLASS, LORA_GET_CLASS_RSP, LORA_CMD_BUSY_RSP, 0);
//  while(!(lora_cli_loop() == RET_OK));
//  get_cls = lora_cmd.lora_rsp_buffer[sizeof(LORA_GET_CLASS_RSP)-1];
//
//  if((cls == 0 && get_cls == 'A') || (cls == 2 && get_cls == 'C'))
//  {
//    PRINTF("[DEBUG]Same_class %c\r\n", get_cls);
//    ret = send_and_check_command(NULL, LORA_JOINED , NULL, 0);
//    while(!(lora_cli_loop() == RET_OK));
//  }
//  else
//  {
//    sprintf(buf, LORA_SET_CLASS, cls);
//    ret = send_and_check_command(buf, LORA_JOINED , NULL, 0);
//    while(!(lora_cli_loop() == RET_OK));
//  }
//
//  PRINTF("[DEBUG]JOIN Completed\r\n");
//  HAL_Delay(1000);
//  lora_cmd.proceeding_flag = 0;
//  return ret;
}

bool send_packet_and_check_rsp(char* packet_buf, char* check_rsp){
  //WKUP1(Pin 39)은 rising edge에 의해 트리거 되며, rising 이후 최소 3.5usec 이상 high 상태를 유지하도록 한다.
  //UART 입력은 rising edge 시작 기준 최소 1msec이상의 시간 이후에 진행 한다. 
  bool ret = false;
  int rx2ch_open_cnt = 0;
  pinMode(PINNUM_WKUP1,OUTPUT);
  delay(TRIGGER_DELAY);
  pinMode(PINNUM_WKUP1,INPUT);

  delay(1000);
  Serial2.println(packet_buf);
  delay(TX_TO_RX_DELAY);

  char downlink_msg[RX_BUF_SIZE];
  while(ret == false && rx2ch_open_cnt != NUM_TRY_TX){
    read_and_print_downlink_msg(downlink_msg, Serial2);
    ret = parsing_downlink_msg(downlink_msg, check_rsp, rx2ch_open_cnt);
    delay(1000);
  }
  rx2ch_open_cnt = 0;
  return ret;
}

bool parsing_downlink_msg(char* str, char* check_rsp, int& rx2ch_open_cnt){
  //TODO : busy
  bool ret = false;
  if(strstr(str, LORA_CONFIRMED_DOWN) != 0){
    delay(10000); //ACK 보내는 시간동안 기다리고 디버그 메세지 모으기
    char additional[RX_BUF_SIZE];
    read_and_print_downlink_msg(additional, Serial2);
    strcat(str, additional);

    if(strstr(str, LORA_DEVRESET)){
      pinMode(PINNUM_MCURST, OUTPUT); // MCU RESET
      delay(500);
    }
    ret = true;
  }

  if(strstr(str, RX2CH_OPEN) != 0){
    rx2ch_open_cnt++;
    //Serial.print("num tx try : ");
    //Serial.println(rx2ch_open_cnt);
  }
    
  if(check_rsp != NULL){
    if(strstr(str, check_rsp) != 0){
      ret = true;
    }
  }
  else{
    if(strstr(str, LORA_CLI_OK) != 0){
      ret = true;
    }
    if(strstr(str, LORA_CLI_ERROR) != 0){
      delay(10000);
      pinMode(PINNUM_MCURST, OUTPUT); // MCU RESET
      delay(500);
    }
  }
  str[0] = '\0';
  return ret;
}

bool payload_GPS_Module(char* buf)
{
  buf[0] = '\0';
  TinyGPS gps;
  while(Serial1.available())
    gps.encode(Serial1.read());
 
  //Append Datetime
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  char sz[32];
  sprintf(sz, "%02d/%02d/%02d#%02d:%02d:%02d#", year, month, day, hour, minute, second);
  strcat(buf, sz);

  //Append Position&Speed
  float flat, flon;
  gps.f_get_position(&flat, &flon, &age);
  float fmps = gps.f_speed_mps();
  char pos[64];
  sprintf(pos, "%f#%f#%f", flat, flon, fmps);
  strcat(buf, pos);

  //TODO: implement not available
  return true;
}
