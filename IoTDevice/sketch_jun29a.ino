void setup() {
  //Initialize
  Serial.begin(115200);

  //SetClass
  Serial.println("LRW 4B 2");
  //ack 확인(??)
  
}

void loop() {
  //device control check
  //??



  
  //uplink packet
  byte buf[18];

 

  buf[0] = 0x4C; buf[1] = 0x52; buf[2] = 0x57; buf[3] = 0x20; buf[4] = 0x34;

  buf[5] = 0x44; buf[6] = 0x20; buf[7] = 0x01; buf[8] = 0x01; buf[9] = 0x06;

      buf[10] = 0x4D;

      buf[11] = 0x4A;

      buf[12] = 0x53;

      buf[13] = 0x4C;

      buf[14] = 0x44;

      buf[15] = 0x57;

  buf[16] = 0x0D; buf[17] = 0x0A;


 
  Serial.write(buf,18);

  delay(5000);
}
