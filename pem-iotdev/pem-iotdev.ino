#include <stdio.h>
#include <time.h>
#include <TinyGPS.h>
#include <string.h>

#define SERIAL1_RXPIN 23
#define SERIAL1_TXPIN 22
#define PINNUM_LORARST 2
#define PINNUM_WKUP1 3    /////////////////////////////////////////////
#define MAINLOOP_TIME 3000
#define UPLINK_TIME 60000
#define TRIGGER_DELAY 10
#define TX_TO_RX_DELAY 3000
#define RX_BUF_SIZE 6000
#define MSG_65_BYTES "12345678901234567890123456789012345678901234567890123456789012345"
#define MSG_66_BYTES "123456789012345678901234567890123456789012345678901234567890123456"
#define LORA_CON_SEND "LRW 31 %s cnf 1"
#define LINK_CHECK_REQ "LRW 38"
#define TIME_SYNC_REQ "LRW 39"

#define LORA_JOINED "join is completed"
clock_t CLK;

void check_pc_command(void);
void lora_reset(void);
void read_and_print_downlink_msg(char* downlink_msg, HardwareSerial module);
void set_class(int cls);
void send_handler(char * buf);
bool send_packet_and_check_rsp(char* packet_buf, char* check_rsp);
bool parsing_downlink_msg(char* str);

void setup() {
  Serial.begin(115200); //Host PC
  Serial1.begin(9600, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); //GPS Module
  Serial2.begin(115200); //LoRa Module
  Serial2.setRxBufferSize(RX_BUF_SIZE - 10);
  lora_reset();
}

void loop() {
  if(Serial.available()){
    check_pc_command();
  }
  if(clock()-CLK > UPLINK_TIME){
    char packet_buf[128], gps_buf[128];
    if(payload_GPS_Module(gps_buf) == false){
      //NYI
    }
    sprintf(packet_buf, LORA_CON_SEND, gps_buf);
    if(send_packet_and_check_rsp(packet_buf, NULL) == true) CLK = clock();
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
        send_packet_and_check_rsp(fromPC, NULL);
        delay(5000);
        break;
    case 1://Data Send 65 Bytes
        if(clock() - CLK > UPLINK_TIME) { 
          if(send_packet_and_check_rsp(MSG_65_BYTES, NULL) == true) CLK = clock();
        }
        else Serial.println("[ERROR] uplink time error");
        break;
    case 2://Data Send 66 Bytes
        if(clock() - CLK > UPLINK_TIME) { send_packet_and_check_rsp(MSG_66_BYTES, NULL); }
        else Serial.println("[ERROR] uplink time error");
        break;
    case 3://Link Check Request
        send_packet_and_check_rsp(LINK_CHECK_REQ, NULL);
        delay(3000);
        break;
    case 4://Device Time Request
        send_packet_and_check_rsp(TIME_SYNC_REQ, NULL);
        delay(3000);
        break;
    default:
        //NYI
        Serial.print("[MCU DEBUG] ERROR : ");
        Serial.write(fromPC);
  }
}

void lora_reset(){
  pinMode(PINNUM_LORARST,OUTPUT);
  delay(TRIGGER_DELAY);
  pinMode(PINNUM_LORARST,INPUT);

  bool ret = false;
  while(ret == false){
    char downlink_msg[RX_BUF_SIZE];
    read_and_print_downlink_msg(downlink_msg, Serial2);
    ret = parsing_downlink_msg(downlink_msg, LORA_JOINED);
  }
  //set_class(0);
  //set_adr(0);
  //set_dr(0);
  //set_con_retrans(8);
}

void read_and_print_downlink_msg(char* downlink_msg, HardwareSerial module){
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
  int ret = false;
  
  pinMode(PINNUM_WKUP1,OUTPUT);
  delay(TRIGGER_DELAY);
  pinMode(PINNUM_WKUP1,INPUT);
  
  Serial2.print(packet_buf);
  delay(TX_TO_RX_DELAY);

  char downlink_msg[RX_BUF_SIZE];
  while(ret == false){
    read_and_print_downlink_msg(downlink_msg, Serial2);
    ret = parsing_downlink_msg(downlink_msg, check_rsp);
    delay(1000);
  }
  return ret;
}

bool parsing_downlink_msg(char* str, char* check_rsp){
  //TODO : busy
  bool ret = false;
  if(strstr(str, "DevReset") != 0){
    delay(10000);
    pinMode(0,OUTPUT); // MCU RESET
    delay(500);
  }
  if(check_rsp != NULL){
    if(strstr(str, check_rsp) != 0){
      ret = true;
    }
  }
  else{
    if(strstr(str, "FRAME_TYPE_DATA_UNCONFIRMED_DOWN") != 0){
      ret = true;
    }
  }
  return ret;
}

bool payload_GPS_Module(char* buf)
{
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
