#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

const char button = D2;
int LEDSTATE = 0;
uint8_t broadcastAddress[] = {0x68, 0xC6, 0x3A, 0xA4, 0xC9, 0x60};

typedef struct struct_message {
  int b;
} struct_message;

typedef struct struct_answer{
  int ledS;
}struct_answer;

struct_message myData;
struct_answer ledData;


void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&ledData, incomingData, sizeof(ledData));
  Serial.print("Ledka je: ");
  Serial.println(ledData.ledS);
  LEDSTATE = ledData.ledS;
}

void setup() {
  pinMode(button,INPUT_PULLUP);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}

void loop() {
  if(digitalRead(button)==HIGH && LEDSTATE ==0){
    myData.b = 1;
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    delay(500);
  }

}