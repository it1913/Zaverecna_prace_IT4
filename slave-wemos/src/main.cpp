#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

uint8_t broadcastAddress[] = {0xAC, 0x0B, 0xFB, 0xDC, 0x91, 0x79};
const char LED = D5;
const char button = D1;
// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
    char a[32];
    int b;
    float c;
    String d;
    bool e;
} struct_message;

typedef struct struct_answer{
  int ledS;
}struct_answer;

int LEDSTATE = 0;
// Create a struct_message called myData
struct_message myData;
struct_answer ledData;
String success;

// callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Char: ");
  Serial.println(myData.a);
  Serial.print("Int: ");
  Serial.println(myData.b);
  Serial.print("Float: ");
  Serial.println(myData.c);
  Serial.print("String: ");
  Serial.println(myData.d);
  Serial.print("Bool: ");
  Serial.println(myData.e);
  Serial.println();
  LEDSTATE = myData.b;
  digitalWrite(LED, HIGH);
}
 
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }else {
  success = "Delivery Fail :(";
  }
}

void setup() {
  // Initialize Serial Monitor
  pinMode(button,INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
}

void loop(){
  if(digitalRead(button)==HIGH && LEDSTATE==1){
    ledData.ledS = 0;
    esp_now_send(broadcastAddress, (uint8_t *) &ledData, sizeof(ledData));
    digitalWrite(LED, LOW);
    LEDSTATE=0;
    delay(500);
  }
}