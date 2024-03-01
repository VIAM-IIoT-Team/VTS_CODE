#include "ModbusMaster.h"
#include <HardwareSerial.h>
#include <stdint.h>
#include <PubSubClient.h>
#include "WiFi.h"

ModbusMaster myModbus;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
const char* ssid = "HELLO";
const char* password = "12345678";
const char* mqtt_server = "40.82.154.13";
const char* mqtt_topic = "MaiXuanCanh/Siemens/Sentronpac3100/NangLuong";

unsigned long lastSendTime = 0; // Biến lưu thời gian gửi lần cuối
const unsigned long sendInterval = 5000; // Thời gian gửi sau mỗi 5 giây

void setup() {
  Serial.begin(9600);
  Serial2.begin(19200, SERIAL_8N2, 16, 17); // TX: GPIO17, RX: GPIO16
  myModbus.begin(126, Serial2); // Slave ID = 126, sử dụng UART2
}
void loop () {

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

}