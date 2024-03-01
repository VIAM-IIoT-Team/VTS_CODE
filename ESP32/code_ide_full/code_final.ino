#include "WiFi.h"
#include <HTTPClient.h>
#include <SD.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include "ModbusMaster.h"
#include <HardwareSerial.h>
#include <stdint.h>
#include <PubSubClient.h>
#include <set>

ModbusMaster myModbus;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

int oldvitricontro;

const char* ssid = "HELLO";
const char* password = "12345678";
const char* mqtt_server = "40.82.154.13";
const char* mqtt_topic = "MaiXuanCanh/Siemens/Sentronpac3100/NangLuong";

String Web_App_URL = "https://script.google.com/macros/s/AKfycbwocH0ldKyaXSmATtrd6ZT9w7N44h_PDBA1Q2YsXeYg2i2hGQDktUkTfT-P4aLWaaOEfg/exec";
const int chipSelect = 14;  // CS pin for SD card
String maduantoancuc = "";
const int buttonPin = 32;  // LOAD DATA
const int button2Pin = 33; // DOWN
const int button3Pin = 25; // UP
const int button4Pin = 26; // ENTER





const int GPIO_13 = 13;
const int GPIO_39 = 39;
const int GPIO_34 = 34;
int buttonState13 = 0;
int lastButtonState13 = 0;
int buttonState39 = 0;
int lastButtonState39 = 0;
int buttonState34 = 0;
int lastButtonState34 = 0;
int pressCount = 0;




int buttonState = LOW;  // Variable to store the button state
int lastButtonState = LOW;  // Variable to store the previous button state
int currentPosition = 0;  // Variable to track the current position in the file
int gpio25Value = 0;
int gpio26Value = 0;
int gpio33Value = 0;
int vitridautien = 0 ; 
int button25State = LOW;
int button26State = LOW;
int button33State = LOW;
int filehienthi = 0 ; 
int button25Count = 0;
int xacnhan = 0;
int button33Count = 0;
int vitricontro = 0; 
LiquidCrystal_I2C lcd(0x27, 20, 4); // I2C address 0x27, 20 columns, 4 rows

unsigned long lastButton2PressTime = 0;
unsigned long lastButton3PressTime = 0;
const unsigned long debounceDelay = 50;
unsigned long lastSendTime = 0; // Biến lưu thời gian gửi lần cuối
const unsigned long sendInterval = 5000; // Thời gian gửi sau mỗi 5 giây

const char* tenfilecanhienthi = "/MACHITIET.txt";  // Default file name
const char* tenfile2 = "/MADUAN.txt"; 

// Set to store unique lines from MADUAN.txt
std::set<String> uniqueLines;

int getFileRowCount(const char* fileName) {
  File file = SD.open(fileName);

  if (file) {
    int rowCount = 0;
    while (file.available()) {
      String line = file.readStringUntil('\n');
      line.trim();  // Remove leading and trailing whitespace
      if (line.length() > 0) {
        rowCount++;
      }
    }

    file.close();
    return rowCount;
  } else {
    Serial.println("Error opening file for reading");
    return -1;
  }
}
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void saveColumnToSD(JsonArray data, const char* fileName) {
  File file = SD.open(fileName, FILE_WRITE);
  if (!file) {
    Serial.println("Error opening file on SD card");
    return;
  }

  // Clear the set before writing to the file
  uniqueLines.clear();

  for (JsonVariant row : data) {
    String rowData = row[0].as<String>();
    // Check if the string is not empty
    if (rowData.length() > 0) {
      // Add the line to the set to ensure uniqueness
      uniqueLines.insert(rowData);
    }
  }

  // Write the unique lines to the file
  for (const String& line : uniqueLines) {
    file.println(line);
  }

  // Close the file
  file.close();

  Serial.println(fileName + String(" data saved"));
}


void displayPreviousData(const char* tenfilecanhienthi) {
  currentPosition--;  // Move back to the previous data in the file
  // Check if currentPosition goes below 0, reset to the last row
  int rowCount = getFileRowCount(tenfilecanhienthi);
  if (rowCount != -1 && currentPosition < 0) {
    currentPosition = rowCount - 1;
    Serial.println("currentPosition reset to the last row.");
  }
  readAndPrintFile(tenfilecanhienthi, currentPosition);
}
void displayNextData(const char* tenfilecanhienthi) {
  currentPosition++;  // Move to the next data in the file

  // Check if currentPosition exceeds the current number of rows, reset to 0
  int rowCount = getFileRowCount(tenfilecanhienthi);
  if (rowCount != -1 && currentPosition >= rowCount) {
    currentPosition = 0;
    Serial.println("currentPosition reset to 0.");
  }

  readAndPrintFile(tenfilecanhienthi, currentPosition);


}






void hienthithenho(const char* tenfilecanhienthi) {

  int button2State = digitalRead(button2Pin);
  if (button2State == HIGH) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastButton2PressTime > debounceDelay) {
      lastButton2PressTime = currentMillis;
      displayNextData(tenfilecanhienthi);
    //  delay(150);  // Wait to avoid multiple reads due to button bouncing
    }
  }

  // Check if button 3 is pressed
  int button3State = digitalRead(button3Pin);
  if (button3State == HIGH) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastButton3PressTime > debounceDelay) {
      lastButton3PressTime = currentMillis;
      displayPreviousData(tenfilecanhienthi);
   //   delay(150);  // Wait to avoid multiple reads due to button bouncing
    }
  }
}

void setup() {


  Serial.begin(9600);
  Serial2.begin(19200, SERIAL_8N2, 16, 17); // TX: GPIO17, RX: GPIO16
  myModbus.begin(126, Serial2); // Slave ID = 126, sử dụng UART2
  setup_wifi();
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqtt_callback);
  Serial.println();
  delay(1000);

  if (!SD.begin(chipSelect)) {
    Serial.println("SD Card initialization failed!");
    return;
  }
  Wire.begin();

  lcd.begin(20, 4); // Initialize LCD
  lcd.backlight(); // Turn on the backlight
  lcd.setCursor(0, 0);
  lcd.print("HELLO");

  pinMode(buttonPin, INPUT);
  pinMode(button2Pin, INPUT);
  pinMode(button3Pin, INPUT);
  pinMode(button4Pin, INPUT);
  pinMode(GPIO_13, INPUT);
  pinMode(GPIO_39, INPUT);
  pinMode(GPIO_34, INPUT);
 
  // WiFi.mode(WIFI_STA);
  // delay(1000);
  // WiFi.begin(ssid, password);

  // Check the number of rows in the default file
  int rowCount = getFileRowCount(tenfilecanhienthi);
  if (rowCount != -1) {
    Serial.print("Number of rows in ");
    Serial.print(tenfilecanhienthi);
    Serial.print(": ");
    Serial.println(rowCount);

    if (currentPosition > rowCount || currentPosition < 0) {
      Serial.println("currentPosition is out of bounds. Resetting currentPosition to 0.");
      currentPosition = 0;
    }
  }
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    mqttClient.publish("MaiXuanCanh/RFID/", data.c_str());
  }
   uint32_t result;
  uint16_t volt1, volt2;
  float volt;

  result = myModbus.readHoldingRegisters(0x40001, 10); 

  if (result == myModbus.ku8MBSuccess) {
      volt1 = myModbus.getResponseBuffer(4);  // Lấy giá trị từ Modbus
      volt2 = myModbus.getResponseBuffer(5);  // Lấy giá trị từ Modbus
      uint32_t combinedValue = (volt1 << 16) | volt2;
      volt = *((float*)&combinedValue);
      // Serial.print("Register 3 ");
      // Serial.print(": 0x");
      // Serial.println(combinedValue, HEX); // In ra dạng hex
      // Serial.print("Volt 3");
      // Serial.print(": ");
      // Serial.println(volt, 6); // In ra dạng float với 6 chữ số thập phân
      
    unsigned long currentTime = millis(); // Lấy thời gian hiện tại
    if (currentTime - lastSendTime >= sendInterval) { // Kiểm tra nếu đã đủ thời gian
      char payload[20];
      snprintf(payload, sizeof(payload), "%.6f", volt);
      mqttClient.publish(mqtt_topic, payload, true); //true để giữ retain
      lastSendTime = currentTime; // Cập nhật thời gian gửi lần cuối
    }
    
  } else {
    Serial.println("Error reading Modbus");
  }
mqttClient.loop();

  int buttonState13 = digitalRead(GPIO_13);
  if (buttonState13 != lastButtonState13) {
    if (buttonState13 == HIGH) {
      Serial.println("Button 13 is pressed");
      mqttClient.publish("MaiXuanCanh/Switch Journey/", "Switch On",1);
    }
    lastButtonState13 = buttonState13;
  }

  int buttonState39 = digitalRead(GPIO_39);
  if (buttonState39 != lastButtonState39) {
    if (buttonState39 == HIGH) {
      pressCount++;
      if (pressCount == 1 || pressCount == 3) {
        Serial.println("Button 39 is pressed once");
        mqttClient.publish("MaiXuanCanh/BUTTON/", "ON1 BUTTON",1);
      } else if (pressCount == 2 || pressCount == 4) {
        Serial.println("Button 39 is pressed twice");
        mqttClient.publish("MaiXuanCanh/BUTTON/", "ON2 BUTTON",1);
      }
      if (pressCount > 2) {
        pressCount = 0; // Đặt lại pressCount về 0 nếu lớn hơn 2
      }
    }
    lastButtonState39 = buttonState39;
  }

  int buttonState34 = digitalRead(GPIO_34);
  if (buttonState34 != lastButtonState34) {
    if (buttonState34 == HIGH) {
      Serial.println("Button 34 is pressed");
      mqttClient.publish("MaiXuanCanh/STATUS/", "START",1);
    }
    lastButtonState34 = buttonState34;
  }

  if (buttonState != lastButtonState) {
    if (buttonState == HIGH) {
      unsigned long currentMillis = millis();
      if (currentMillis - lastButton2PressTime > debounceDelay) {
        lastButton2PressTime = currentMillis;

        if (WiFi.status() == WL_CONNECTED) {
          String Read_Data_URL = Web_App_URL + "?sts=read";
          Serial.println("-------------");
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Loading...");
          lcd.setCursor(0, 1);
          lcd.print("Please wait.");

          HTTPClient http;
          http.setTimeout(5000);  // Set timeout to 5 seconds (adjust as needed)

          http.begin(Read_Data_URL.c_str());
          http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

          int httpCode = http.GET();
          Serial.print("HTTP Status Code : ");
          Serial.println(httpCode);

          String payload;
          if (httpCode > 0) {
            payload = http.getString();
            Serial.println("Payload : " + payload);

            DynamicJsonDocument doc(1024);
            deserializeJson(doc, payload);

            // Extract data from JSON object
            JsonArray columnC = doc["columnC"];

            saveColumnToSD(columnC, tenfilecanhienthi);
          }

          http.end();
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Done");
        }
      }

     // delay(150); // Delay to avoid multiple requests due to button bouncing
    }

    lastButtonState = buttonState;
  }

  // HIEN THI LCD MENU 
    lcd.setCursor(1, 0);
    lcd.print("MA DU AN");
    lcd.setCursor(1, 1);
    lcd.print("MA CHI TIET");
    lcd.setCursor(1, 2);
    lcd.print("MA CONG NHAN");
    lcd.setCursor(1, 3);
    lcd.print("XAC NHAN THONG TIN");
// Đọc dữ liệu từ GPIO
  gpio25Value = digitalRead(25);
  gpio26Value = digitalRead(26);
  gpio33Value = digitalRead(33);

  // Đếm số lần bấm nút 25
  if (gpio25Value == HIGH && button25State == LOW) {
    vitricontro++; 
    Serial.print("VITRICONTRO: ");
    Serial.println(vitricontro);
  }
  button25State = gpio25Value;

  // Đếm số lần bấm nút 26
  if (gpio26Value == HIGH && button26State == LOW) {
    xacnhan++;
    Serial.print("Button 26 Pressed! Count: ");
    Serial.println(xacnhan);
  }
  button26State = gpio26Value;

  // Đếm số lần bấm nút 33
  if (gpio33Value == HIGH && button33State == LOW) {
    vitricontro--; 
    Serial.print("VITRICONTRO: ");
    Serial.println(vitricontro);
  }
  button33State = gpio33Value;
if (vitricontro >3 ) {vitricontro=0 ; }
if (vitricontro<0) {vitricontro = 3 ;}  
if (xacnhan >1) {xacnhan = 0 ; } 
  buttonState = digitalRead(buttonPin);
  XC:
switch(vitricontro) {
case 0:     
            lcd.setCursor(0, 1);
            lcd.print(" ");
            lcd.setCursor(0, 2);
            lcd.print(" ");
            lcd.setCursor(0, 3);
            lcd.print(" ");
            lcd.setCursor(0, 0);
            lcd.print(">");
            if (xacnhan !=0 ) {
                lcd.clear() ; 
                filehienthi = 1; 
                while (1) {
                xacnhan = 0 ; 
                lcd.setCursor(2, 0);
                lcd.print("VTSTEK COMPANY");
                lcd.setCursor(6, 1);
                lcd.print("MA DU AN");
                hienthithenho(tenfile2); // chi can doii ten file la co the print ra man hinh roi 
                delay(100) ; 
                if (digitalRead(26) == 1) {
                  lcd.clear() ; 
                  xacnhan = 0 ; 
                  Serial.println(maduantoancuc);
                  break ; 
                }
                }

            }
break; 
case 1:     

            lcd.setCursor(0, 0);
            lcd.print(" ");
            lcd.setCursor(0, 2);
            lcd.print(" ");
            lcd.setCursor(0, 3);
            lcd.print(" ");
            lcd.setCursor(0, 1);
            lcd.print(">");
            if (xacnhan !=0 ) {
                lcd.clear() ; 
                filehienthi =2; 
                while (1) {
                xacnhan = 0 ; 
                lcd.setCursor(2, 0);
                lcd.print("VTSTEK COMPANY");
                lcd.setCursor(4, 1);
                lcd.print("MA CHI TIET");

/////////////////////////////////////// NO TRO VAO VI TRI NAY NE ///////////////////////////////////////////////////
  vitricontro = 0 ; 
  File file = SD.open(tenfilecanhienthi);
xacnhan = 0 ; 
int rowCount = getFileRowCount(tenfilecanhienthi);

  if (file) {
    int currentLine = 0;
    while (file.available() && currentLine <= rowCount) { ///  QUET HET FILE 
      String line = file.readStringUntil('\n');
      currentLine++; 
          if (line.startsWith(maduantoancuc)) {
            vitridautien = currentLine ; 
            break ;
        } 
      }
     int vitricuoicung = vitridautien ; 
     currentLine = 0;
     while (file.available() && currentLine <= rowCount) { ///  QUET HET FILE 
      String line = file.readStringUntil('\n');
      currentLine++; 
          if (line.startsWith(maduantoancuc)) {
            vitricuoicung++;  
        } 
      } 
      /// DEM XONG 
    Serial.print("VI TRI DAU TIEN ");
    Serial.println(vitridautien);    
    Serial.print("VI TRI CUOI CUNG ");
    Serial.println(vitricuoicung);   
    vitricontro = vitridautien;     
while(1) {
  gpio25Value = digitalRead(25);
  gpio26Value = digitalRead(26);
  gpio33Value = digitalRead(33);

  // Đếm số lần bấm nút 25
  if (gpio25Value == HIGH) {
    vitricontro++; 
  }


  // Đếm số lần bấm nút 26
  if (gpio26Value == HIGH && button26State == LOW) {
    xacnhan++;
    Serial.print("Button 26 Pressed! Count: ");
    Serial.println(xacnhan);
  }
  button26State = gpio26Value;

  // Đếm số lần bấm nút 33
  if (gpio33Value == HIGH) {
    vitricontro--; 

  }

// vitricontro = vitridautien ; 
//   if (vitricontro > vitricuoicung ) {
// vitricontro = vitridautien ; 
//   }
//   if (vitricontro < vitridautien ) {
//     vitricontro = vitricuoicung ; 
//   }
  if (xacnhan > 1) {
    xacnhan = 0 ; 
  }
    Serial.print("VITRICONTRO: ");
    Serial.println(vitricontro);

    Serial.println("BREAK");
    delay(100) ; 
                if (digitalRead(26) == 1 || xacnhan ==1 ) {
                  delay(50) ; 
                  lcd.clear() ; 
                  xacnhan = 0 ; 
                  goto XC; 
                  break ; 
                }
    
  file = SD.open(tenfilecanhienthi);  
 
    // Đếm số dòng
 
    // Đọc từng dòng từ tệp
   int lineCount = 0;
   while (1) {
   String line = file.readStringUntil('\n'); // Đọc mỗi dòng
      line.trim(); // Loại bỏ khoảng trắng ở đầu và cuối dòng
      lineCount++;
      //Serial.println("Line count: " + lineCount);  
        //  oldvitricontro = vitricontro ; 
      if (lineCount == vitricontro) {
            lcd.setCursor(0, 2);
            lcd.print(line);
 
        break;
      }
   }
   
    }
  }


    file.close();
    Serial.println();
///////////////////////////////////
                }
                if (digitalRead(26) == 1) {
                  lcd.clear() ; 
                  xacnhan = 0 ; 
                  goto XC; 
                  break ; 
                }
                

            }
break; 
case 2:     
            lcd.setCursor(0, 0);
            lcd.print(" ");
            lcd.setCursor(0, 1);
            lcd.print(" ");
            lcd.setCursor(0, 3);
            lcd.print(" ");
            lcd.setCursor(0, 2);
            lcd.print(">");
break;  
case 3:     
            lcd.setCursor(0, 1);
            lcd.print(" ");
            lcd.setCursor(0, 2);
            lcd.print(" ");
            lcd.setCursor(0, 0);
            lcd.print(" ");
            lcd.setCursor(0, 3);
            lcd.print(">");
break;  
default: break; 

}


}
void readAndPrintFile(const char* fileName, int position) {
  File file = SD.open(fileName);

  if (file) {
    Serial.print("Contents of ");
    Serial.print(fileName);
    Serial.print(": ");

    int currentLine = -1;
    while (file.available() && currentLine <= position) {
      String line = file.readStringUntil('\n');
      currentLine++;
      if (currentLine == position) {
        line.trim();  // Remove leading and trailing whitespace

        // Check if the line is not empty before displaying on LCD
        if (line.length() > 0) {
          maduantoancuc = line ; 
          lcd.setCursor(0, 2);
          lcd.print("                       ");
          lcd.setCursor(0, 2);
          lcd.print(maduantoancuc);
          Serial.println(maduantoancuc);          

        } 
      }
    }

    file.close();
    Serial.println();
  } else {
    Serial.println("Error opening file for reading");
  }
}


void setup_wifi() {
  delay(10);
  Serial.println("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (mqttClient.connect("ESP32Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}