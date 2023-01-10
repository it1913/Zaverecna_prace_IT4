#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "../../web-server/include/common.h"


struct Device {
  DeviceAddress addr;
  int id;
  int state;
  int value;
  int waiting;
};

typedef struct Device Device;

const int server_id = 0;
int slave_id = -1;

Device self;
Device recv;

void Log(Device device, String prefix, String name) {
  Serial.println(prefix
                 + " " + name   
                 + " #" + String(device.id) 
                 + " state=" + String(device.state)
                 + " value=" + String(device.value));
}

void Begin(Device device, String name) {
  Log(device,"+",name);
}

void End(Device device, String name) {
  Log(device,"/",name);
}

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

int press(String name) {
  Begin(self,name);
  self.state = !self.state;
  self.value += 1;
  self.waiting = HIGH;
  sendData.id = self.id;
  sendData.state = self.state;
  sendData.value = self.value;
  sendData.command = CMD_SWITCH_STATE;
  sendData.response = RESP_I_SWITCHED_STATE;
  check(esp_now_send(recv.addr.bytes, (uint8_t *) &sendData, sizeof(sendData)),"esp_now_send");
  digitalWrite(pin_LED, self.state);
  End(self,name);
  return self.state;
}

int answer(String name, int command, int response, int state) {
  Begin(self,name);
  self.state = state;
  self.value += 1;
  sendData.id = self.id;
  sendData.state = self.state;
  sendData.value = self.value;
  sendData.command = command;
  sendData.response = response;  
  check(esp_now_send(recv.addr.bytes, (uint8_t *) &sendData, sizeof(sendData)),"esp_now_send");
  digitalWrite(pin_LED, self.state);
  End(self,name);
  return self.state;
}

void flicker(int count, int millis) {
  for (int i = 0; i < count; i++) {
    digitalWrite(pin_LED, HIGH);
    delay(millis);
    digitalWrite(pin_LED, LOW);
    delay(millis);
  }
  digitalWrite(pin_LED, self.state);
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&recvData, incomingData, sizeof(recvData));  
  Begin(self,"recv");
  if((recvData.command == CMD_SWITCH_STATE) || (recvData.command == CMD_SWITCH_BY_GAME)){    
    Serial.printf("Received state %d and value=%d from device #%d, ",recvData.state,recvData.value,recvData.id);
    self.state = recvData.state;
    self.value = recvData.value;    
    digitalWrite(pin_LED, self.state);    
    self.waiting = LOW;
  } else
  if (recvData.command == CMD_WHO_IS_HERE) {
    answer("i_am_here",recvData.command,RESP_I_AM_HERE,WHO_IS_HERE_STATE);
  }
  End(self,"recv"); 
}
 
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.printf("Sent state %d and value=%d to device #%d, ",sendData.state,sendData.value,recv.id);
  check(sendStatus,"Last Packet Send Status");  
}

bool isDevice(String selfAddress, int id, DeviceAddress addr) {
  if (selfAddress == mac2str(addr)) {
    slave_id = id;
    self.addr = addr;
    self.id = slave_id;
    recv.addr = mac_server;
    recv.id = server_id;
    return true;
  } else {
    return false;
  }
} 

void setup() {

  pinMode(pin_button,INPUT_PULLUP);
  pinMode(pin_LED, OUTPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  String mac = WiFi.macAddress();
  int32_t wifi_channel = getWiFiChannel(WIFI_SSID);

  Serial.print("IP Address : ");
  Serial.println(WiFi.localIP());
  Serial.print("Channel    : ");
  Serial.println(wifi_channel);
  Serial.print("MAC Address: ");
  Serial.println(mac);

  WiFi.printDiag(Serial); 
  wifi_promiscuous_enable(true);
  wifi_set_channel(wifi_channel);
  wifi_promiscuous_enable(false);  
  WiFi.printDiag(Serial); 

  bool knownDevice = false;
  for (int i = 0; i < button_count; i++) {
    knownDevice = knownDevice || isDevice(mac, button[i].id, button[i].address); 
  }

  if (knownDevice) {
    Serial.println("Known device at address "+mac);  
  } else
  if (mac == mac2str(mac_server)) {
    Serial.println("I am server at address "+mac);
    return;
  } else {
    Serial.println("Unknown device at address "+mac);
    return;
  }
  self.state = LOW;
  self.value = 0;
  self.waiting = LOW;
  
  recv.state = LOW;
  recv.value = 0;
  recv.waiting = LOW;
  
  Serial.println();
  Serial.println("Restart");
  Serial.print("Self #"+String(self.id)+": ");
  Serial.println(mac2str(self.addr));
  Serial.print("Recv #"+String(recv.id)+": ");
  Serial.println(mac2str(recv.addr));

  if (check(esp_now_init(),"esp_now_init") != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  check(esp_now_set_self_role(ESP_NOW_ROLE_COMBO),"esp_now_set_self_role");
  check(esp_now_register_recv_cb(OnDataRecv),"esp_now_register_recv_cb");
  check(esp_now_register_send_cb(OnDataSent),"esp_now_register_send_cb");
  
  check(esp_now_add_peer(recv.addr.bytes, ESP_NOW_ROLE_COMBO, wifi_channel, NULL, 0),"esp_now_add_peer");

  recvData.id = self.id;
  recvData.state = self.state;
  recvData.value = self.value;

  flicker(10, 100);
}

const long interval = 5000; 

void loop(){
  if (digitalRead(pin_button)==HIGH && self.waiting==LOW)
  {
    self.waiting = HIGH;
    press("manual");
    delay(CLIENT_PRESS_DELAY);
  }
}