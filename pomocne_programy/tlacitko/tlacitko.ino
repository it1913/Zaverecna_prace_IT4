const char button = D2;

void setup() {
    Serial.begin(115200);
    pinMode(button,INPUT_PULLUP); //button
}

void loop() {
  if(digitalRead(button)==HIGH){  //if button 6 is pressed...
     Serial.println("I'm Pressed!"); //print the message on a new line¨
     delay(300);
  }
}
