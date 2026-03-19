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
File file;
struct tUser {
  byte UID[4];
  String name;
};
tUser users[MAX_USERS];          // Массив для хранения данных о пользователях
byte userCount = 0;             // Счетчик количества пользователей
byte idToEdit = 254;            // Идентификатор пользователя, которого нужно отредактировать
bool confirmed = false;          // Флаг подтверждения

void readUsersFromFile() {
  byte i;
  File file = LittleFS.open("/users.data", "r");
  if(!file){
     Serial.println("Failed to open 'users.data' for reading...");
     return;
  }
  Serial.println("Read '/users.data': ");
  for (i = 0; i < MAX_USERS; i++) {
    if (file.available() >= 5) { // Проверяем, есть ли достаточно данных для чтения UID и имени
      file.read(users[i].UID, 4); // Читаем UID
      users[i].name = file.readStringUntil('\n'); // Читаем имя до новой строки
      Serial.print("User ");
      Serial.print(i);
      Serial.print(": UID=");
      for (byte j = 0; j < 4; j++) {
        Serial.print(users[i].UID[j] < 0x10 ? " 0" : " ");
        Serial.print(users[i].UID[j], HEX);
        if (j < 3) Serial.print(":");
      }
      Serial.print(", Name=");
      Serial.println(users[i].name);
    } else {
      break; // Если данных меньше, прекращаем чтение
    }
  }
  userCount = i; // Сохраняем количество считанных пользователей
  String Out="Readed " + String(userCount) + " users";
  Serial.println(Out);
  file.close();
}
void writeUsersToFile() {
  byte i;
  File file = LittleFS.open("/users.data", "w");
  if(!file){
     Serial.println("Failed to open 'users.data' for writing...");
     return;
  }
  Serial.println("Writing to '/users.data': ");
  for (i = 0; i < userCount; i++) {
    file.write(users[i].UID, 4);
    file.println(users[i].name);
  }
  file.close();
}
void appendUserToFile(){
  File file = LittleFS.open("/users.data", "a");
  if(!file){
     Serial.println("Failed to open 'users.data' for appending...");
     return;
  }
  file.write(UID, 4);
  file.println("User" + String(userCount+1));
  file.close();
  users[userCount].name = "User" + String(userCount+1);
  memcpy(users[userCount].UID, UID, 4);
  userCount++;
  Serial.println("Appended new user to 'users.data'");
}
void handleAddNew() {
  Serial.println("Button 'ADD NEW' pressed");
  appendUserToFile();

  server.sendHeader("Location", "/userlist");
  server.send(303);
}
void handleRoot() {
  String html = "";
  String data = "";

  File file = LittleFS.open("/home.html", "r");
  if(!file){
     Serial.println("Failed to open file for reading...");
     return;
  }

  Serial.println("Read '/home.html': ");
  while(file.available()){
     html += char(file.read());
   }
  file.close();
  /*
  Serial.println("Write '/users.data': ");
  file = LittleFS.open("/users.data", "a");
  for (int i = 0; i < 4; i++) {
    file.write(lastUID[i]);
  }
  file.println(":User");
  file.close();
  
  */
  
  /*
  file = LittleFS.open("/users.data", "r");
  if(!file){
     Serial.println("Failed to open 'users.data' for reading...");
     return;
  }
  Serial.println("Read '/users.data': ");
  while(file.available()){
     //data += file.read();
     byte buffer;
      file.read(&buffer, 1);
      Serial.print(buffer, DEC);
   }
   file.close();
  Serial.println(data);
  */
  server.send(200, "text/html", html);
}
void handleUserList() {
  String html = "";
  String buffer = "";

  readUsersFromFile();

  File file = LittleFS.open("/userlist.html", "r");
  if(!file){
     Serial.println("Failed to open file for reading...");
     return;
  }
  Serial.println("Read '/userlist.html': ");
  while(file.available()){
    // html += char(file.read());
    buffer = file.readStringUntil('\n');
    //Serial.println(buffer);
    if (buffer.substring(0, 5) == "*****") {
      for (byte i = 0; i < userCount; i++) {
        buffer = "<tr>\n<td>" + users[i].name + "</td>\n";
        buffer += "<td><a href='/edit?id="+String(i+1)+"'><button>Редагувати</button></a></td>\n";
        buffer += "<td><a href='/delete?id="+String(i+1)+"'><button>Видалити</button></a></td>\n";
        buffer += "</tr>\n";
        html += buffer;
      }
    } else html += buffer;
  }
  file.close();
  //Serial.println(html);
  server.send(200, "text/html", html);
}
void handleEdit() {
  if (server.hasArg("id")) {
    String id = server.arg("id");
    Serial.print("Id: ");
    Serial.println(id);
    idToEdit = id.toInt() - 1;
  }
  String html = "";
  String buffer = "";

  File file = LittleFS.open("/edituser.html", "r");
  if(!file){
     Serial.println("Failed to open file for reading...");
     return;
  }
  Serial.println("Read '/edituser.html': ");
  while(file.available()){
    // html += char(file.read());
    /*
    buffer = file.readStringUntil('\n');
    //Serial.println(buffer);
    if (buffer.substring(0, 5) == "*****") {
      for (byte i = 0; i < userCount; i++) {
        buffer = "<tr>\n<td>" + users[i].name + "</td>\n";
        buffer += "<td><a href='/edit?id="+String(i+1)+"'><button>Редагувати</button></a></td>\n";
        buffer += "<td><a href='/delete?id="+String(i+1)+"'><button>Видалити</button></a></td>\n";
        buffer += "</tr>\n";
        html += buffer;
      }
    } else html += buffer;
 
    */
   html += file.readStringUntil('\n');
   int pos = html.indexOf("*****");
   if (pos != -1) {
      String before = html.substring(0, pos);
      String after = html.substring(pos + 5);
      html = html.substring(0, pos) + users[0].name + html.substring(pos + 5); // Вставляем имя пользователя вместо "*****"      
    }
      
  }
  file.close();
  //Serial.println(html);
  server.send(200, "text/html", html);
}
void handleConfirmEdit() { // to be implemented
  if (server.hasArg("name")) {
    String name = server.arg("name");
    Serial.print("Id: ");
    Serial.println(idToEdit);
    Serial.print(", Name: ");
    Serial.println(name);
  }
  users[idToEdit].name = server.arg("name");
  writeUsersToFile();
  server.sendHeader("Location", "/userlist");
  server.send(303);
}

void setup() {
  byte  i;
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

  readUsersFromFile();

  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  
  server.begin();
  server.on("/", handleRoot);
  server.on("/userlist", handleUserList);
  server.on("/edit", handleEdit);
  server.on("/confirmedit", handleConfirmEdit);
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
void blinkOnce(){
  digitalWrite(LED_BUILTIN, HIGH);
  delay(700);
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
   // Serial.print(UID[i], DEC);
  }
  Serial.println();
  memcpy(lastUID, UID, 4); // Сохраняем UID как последний считанный
  // rfid.PICC_DumpToSerial(&(rfid.uid));
  for (byte i = 0; i < userCount; i++) {
    if (memcmp(UID, users[i].UID, 4) == 0) {
      Serial.print("Welcome, ");
      Serial.println(users[i].name);
      blinkOnce();
      confirmed = true;
      return;
    }
  }
  if (!confirmed) blinkTwice();
  //appendUserToFile();

}