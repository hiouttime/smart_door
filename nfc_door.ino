#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#define RST_PIN         5           // 配置针脚
#define SS_PIN          4
#define SERVO_PIN       16          // 舵机
#define START           175         // 舵机起始位置
#define END             35          // 舵机转动位置
#define STASSID ""
#define STAPSK  ""
#define AUTH ""       // 认证服务器地址

const char* ssid = STASSID;
const char* password = STAPSK;
WiFiServer server(80); // 创建HTTP服务器
MFRC522 mfrc522(SS_PIN, RST_PIN);   // 创建新的RFID实例
MFRC522::MIFARE_Key key;
Servo myServo;

void setup() {
  Serial.begin(9600); // 设置串口波特率为9600
  SPI.begin();        // SPI开始
  Serial.println();
  Serial.print(F("正在连接到 "));
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  Serial.println(F("WiFi 已连接"));
  mfrc522.PCD_Init(); // 加载 MFRC522 card
  Serial.println("读卡器版本：");
  mfrc522.PCD_DumpVersionToSerial();
  myServo.attach(SERVO_PIN,500,2500);  // 设置舵机针脚
  myServo.write(START);
  server.begin();
  Serial.println(F("服务器已启动"));
  Serial.println("加载完毕，地址：");
  Serial.println(WiFi.localIP());
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println(F("new client"));
    client.setTimeout(5000);
    String req = client.readStringUntil('\r');
    if (req.indexOf(F("/open")) != -1) {
      Serial.println(F("调用主动开门"));
      client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nsuccess"));
      open();
    }
    while (client.available())
      client.read();
    return;
  }
  // 寻找新卡
  if ( ! mfrc522.PICC_IsNewCardPresent())
    return;
  // 选择一张卡
  if ( ! mfrc522.PICC_ReadCardSerial())
    return;
  // 显示卡片的详细信息
  Serial.print(F("卡片 UID:"));
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  //停止 PICC
  mfrc522.PICC_HaltA();
  unsigned long uid;
  uid =  mfrc522.uid.uidByte[0] << 24;
  uid += mfrc522.uid.uidByte[1] << 16;
  uid += mfrc522.uid.uidByte[2] <<  8;
  uid += mfrc522.uid.uidByte[3];
 
  WiFiClient http;
  if (!http.connect(AUTH,80)) {
    Serial.println("连接到验证服务器失败");
    return;
  }
  unsigned int timereq; // 请求计时
  String request = (String)("GET ") + "/door/?uid=" + (String)uid + " HTTP/1.1\r\n" + 
   "Content-Type: text/html;charset=utf-8\r\n" +  
   "Host: " + AUTH + "\r\n" +
   "User-Agent: ESP8266\r\n" +
   "Connection: close\r\n" +
   "\r\n";
  timereq = millis();
  http.print(request);  // 发送HTTP请求
  while (http.connected() || http.available()){
    if(http.available()){
      if(http.find("pass")){
        Serial.println("成功认证");
        open();
        break;
      }
      if(http.find("fail")){
        Serial.println("未授权的门卡");
        break;
      }
    }
    if(millis() - timereq > 1000){
      Serial.println("请求超时，结束...");
      break;
    }
  }
  http.stop();
  digitalWrite(RST_PIN, HIGH);
  delay(1000);
  return;
}

/**
   将字节数组转储为串行的十六进制值
*/
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void open(){
  myServo.write(END);
  delay(2000);//延时2s
  myServo.write(START);
}
