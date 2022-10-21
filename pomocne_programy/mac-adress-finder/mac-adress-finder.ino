

void setup() {
    Serial.begin(115200);
    pinMode(D2,INPUT_PULLUP); //button
}

void loop() {
  if(digitalRead(D2)==HIGH){  //if button 6 is pressed...
     Serial.println("I'm Pressed!"); //print the message on a new line¨
     delay(300);
  }
}
