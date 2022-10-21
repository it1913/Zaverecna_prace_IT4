#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

const char LED = D5;
const char button = D1;
int LEDSTATE = 0;
uint8_t broadcastAddress[] = {0xAC, 0x0B, 0xFB, 0xDC, 0x91, 0x79};

typedef struct struct_message {
    int b;
} struct_message;

typedef struct struct_answer{
  int ledS;
}struct_answer;

struct_message myData;
struct_answer ledData;

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  LEDSTATE = myData.b;
  digitalWrite(LED, HIGH);
}
 
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}

void setup() {
  pinMode(button,INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
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