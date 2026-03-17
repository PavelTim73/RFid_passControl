#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

#define RST_PIN         38       // Пин rfid модуля RST
#define SS_PIN          34       // Пин rfid модуля SS
#define MISO_PIN        37       // Пин rfid модуля SS
#define MOSI_PIN        35       // Пин rfid модуля SS
#define SCK_PIN         36       // Пин rfid модуля SS
#define MAX_USERS       50       // Максимальное количество пользователей, которых может распознавать система 
MFRC522 rfid(SS_PIN, RST_PIN);   // Объект rfid модуля
MFRC522::MIFARE_Key key;         // Объект ключа
MFRC522::StatusCode status;      // Объект статуса
byte UID[4],lastUID[4];                     // Массив для хранения UID

const char* ssid = "WiFi_Control";
const char* password = "";
WebServer server(80);
IPAddress local_IP(33, 33, 33, 33);

struct tUser {
  byte UID[4];
  String name;
};
tUser users[MAX_USERS];          // Массив для хранения данных о пользователях
void handleAddNew() {
  Serial.println("Button pressed");
  server.sendHeader("Location", "/");
  server.send(303);
}
void handleRoot() {
  String html = "";

  File file = LittleFS.open("/home.html", "r");
  if(!file){
     Serial.println("Failed to open file for reading...");
     return;
  }

  Serial.println("Read from file: ");
  while(file.available()){
     html += char(file.read());
   }
  file.close();
  
  server.send(200, "text/html", html);
}
void setup() {
  Serial.begin(115200); 
  while (!Serial) 
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN); // Инициализация SPI (int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
  rfid.PCD_Init();               // Инициализация модуля 
  for (byte i = 0; i < 6; i++) { // Наполняем ключ
    key.keyByte[i] = 0xFF;       // Ключ по умолчанию 0xFFFFFFFFFFFF
  }
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println();
  Serial.println("Scan PICC to see UID");
  
  if (!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
  
  }
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  
  server.begin();
  server.on("/", handleRoot);
  server.on("/addnew", handleAddNew);

}
void blinkTwice(){
  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN, LOW);
}
void loop() {
  
   server.handleClient();
  // Занимаемся чем угодно
  if (!rfid.PICC_IsNewCardPresent()) return;  // Если новая метка не поднесена - вернуться в начало loop
  if (!rfid.PICC_ReadCardSerial()) return;    // Если метка не читается - вернуться в начало loop
  // Работаем с RFID
  memcpy(UID, rfid.uid.uidByte, 4);
  if (memcmp(UID, lastUID, 4) == 0) return;  // Если UID совпадает с последним считанным, не выводим его снова
     
  Serial.print("UID:");
  for (byte i = 0; i < 4; i++) {
    Serial.print(UID[i] < 0x10 ? " 0" : " ");
    Serial.print(UID[i], HEX);
  }
  Serial.println();
  memcpy(lastUID, UID, 4); // Сохраняем UID как последний считанный
  // rfid.PICC_DumpToSerial(&(rfid.uid));
  blinkTwice();
  handleRoot();

}