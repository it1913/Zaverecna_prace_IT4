#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

const char button = D2;
int LEDSTATE = 0;
// REPLACE WITH RECEIVER MAC Address

uint8_t broadcastAddress[] = {0x68, 0xC6, 0x3A, 0xA4, 0xC9, 0x60};

// Structure example to send data
// Must match the receiver structure
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

// Create a struct_message called myData
struct_message myData;
struct_answer ledData;

// Callback when data is sent
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
  pinMode(button,INPUT_PULLUP); //button
  // Init Serial Monitor
  Serial.begin(115200);
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}

void loop() {
  if(digitalRead(button)==HIGH && LEDSTATE ==0){
    // Set values to send
    strcpy(myData.a, "THIS IS A CHAR");
    myData.b = 1;
    myData.c = 1.2;
    myData.d = "Hello";
    myData.e = false;
    LEDSTATE = 1;
    // Send message via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    delay(500);
  }
 
}