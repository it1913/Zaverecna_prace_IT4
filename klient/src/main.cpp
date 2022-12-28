#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

const char pin_LED = D6;
const char pin_button = D1;
const bool automaticPress = false;

struct Address {
  uint8_t bytes[6];
};

typedef struct Address DeviceAddress;

struct Device {
  DeviceAddress addr;
  int id;
  int state;
  int value;
  int waiting;
};

typedef struct Device Device;

//DeviceAddress mac_1 = {0xAC, 0x0B, 0xFB, 0xDC, 0x91, 0x79};
//DeviceAddress mac_2 = {0x68, 0xC6, 0x3A, 0xA4, 0xC9, 0x60};
//DeviceAddress mac_3 = {0xAC, 0x0B, 0xFB, 0xDC, 0x91, 0x79};

DeviceAddress mac_1 = {0x68, 0xC6, 0x3A, 0xA4, 0xC9, 0x60};
DeviceAddress mac_2 = {0xAC, 0x0B, 0xFB, 0xD9, 0x95, 0x58};
DeviceAddress mac_3 = {0xAC, 0x0B, 0xFB, 0xDC, 0x91, 0x79};

const int server_id = 2;
int slave_id = 0;

Device self;
Device recv;

struct Message {
  int id;
  int state;
  int value;
  uint32_t dateTime;
};

typedef struct Message SendData;
typedef struct Message RecvData;

SendData sendData;
RecvData recvData;

String mac2str(DeviceAddress addr) {
  char s[20];
  sprintf(s, "%02X:%02X:%02X:%02X:%02X:%02X",addr.bytes[0],addr.bytes[1],addr.bytes[2],addr.bytes[3],addr.bytes[4],addr.bytes[5]);  
  return s;
}

String time2str(uint32_t millis) {
  unsigned long allSeconds=millis/1000;
  int runHours=allSeconds/3600;
  int secsRemaining=allSeconds%3600;
  int runMinutes=secsRemaining/60;
  int runSeconds=secsRemaining%60;
  char s[21];
  sprintf(s,"%02d:%02d:%02d",runHours,runMinutes,runSeconds);
  return s;
}

int check(int code, String name) {
  if (code != 0) {
    Serial.println(name + " = " + String(code));
  }
  return code;
}

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

// Insert your SSID
const char *WIFI_SSID = "Lego3";
const char *password = "";

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

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&recvData, incomingData, sizeof(recvData));
  Begin(self,"recv");
  Serial.printf("Received state %d and value=%d from device #%d, ",recvData.state,recvData.value,recvData.id);
  Serial.println(time2str(recvData.dateTime));
  self.state = recvData.state;
  self.value = recvData.value;
  self.waiting = LOW;
  digitalWrite(pin_LED, self.state);
  End(self,"recv"); 
}
 
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.printf("Sent state %d and value=%d to device #%d, ",sendData.state,sendData.value,recv.id);
  Serial.println(time2str(sendData.dateTime));
  check(sendStatus,"Last Packet Send Status");
}

void setup() {

  pinMode(pin_button,INPUT_PULLUP);
  pinMode(pin_LED, OUTPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, password);
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
  //esp_wifi_set_promiscuous(true);
  //esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  //esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); 

  if (mac == mac2str(mac_1)) {
    slave_id = 1;
    self.addr = mac_1;
    self.id = slave_id;
    recv.addr = mac_2;
    recv.id = server_id;
  } else
  if (mac == mac2str(mac_3)) {
    slave_id = 3;
    self.addr = mac_3;
    self.id = slave_id;
    recv.addr = mac_2;
    recv.id = server_id;
  } else
  if (mac == mac2str(mac_2)) {
    self.addr = mac_2;
    self.id = server_id;
    recv.addr = mac_1;
    recv.id = slave_id;
  } else
  {
    Serial.println("Unknown device "+mac);
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
}

int press(String name) {
  Begin(self,name);
  self.state = !self.state;
  self.value += 1;
  self.waiting = HIGH;
  sendData.id = self.id;
  sendData.state = self.state;
  sendData.value = self.value;
  sendData.dateTime = millis();
  check(esp_now_send(recv.addr.bytes, (uint8_t *) &sendData, sizeof(sendData)),"esp_now_send");
  digitalWrite(pin_LED, self.state);
  End(self,name);
  return self.state;
}

unsigned long previousMillis = 0;
const long interval = 5000; 

void loop(){
  if (digitalRead(pin_button)==HIGH && self.waiting==LOW)
  {
    press("manual");
  } else
  if (automaticPress && (self.id == slave_id)) 
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      press("slave automatic press");
    }
  }
}