void wait(unsigned int value) {
    Serial.println("waiting "+String(value)+" ms");
    delay(value);
}

void setButton(int index, int state, int command) {
    Button *btn = &button[index];
    if (!btn) return;
    //if (!button[index].enabled) return;
    //if (!button[index].playing) return;
    button[index].state = state;
    sendData.id = button[index].id;
    sendData.state = state;
    sendData.value = index;
    sendData.dateTime = millis();
    sendData.command = command;
    sendData.response = RESP_I_SWITCHED_STATE;  
    check(esp_now_send(button[index].address.bytes, (uint8_t *) &sendData, sizeof(sendData)),"esp_now_send");
    /*
    Serial.println("sending to #" + String(button[index].id) 
                                  + " " + mac2str(button[index].address)
                                  + " state " + String(sendData.state));
    */
    //digitalWrite(pin_LED, btn->state);
}

int waitButton(int index, int state) {
    Button *btn = &button[index];
    if (!btn) return 0;
    if (!btn->enabled) return 0;
    while (btn->state != state) {
        wait(10000);
    }
    return 0;
}

void allButtons(int state) {
    Serial.println("allButtons "+String(state));
    for (int i = 0; i < button_count; i++) {
        setButton(i, state, CMD_SWITCH_STATE);
    }
}

int gameButtonIndex = NO_BUTTON_INDEX;
int gameIsActive = false;
int gameCountOfStep = 30;
int gameCurrrentStep = 0;

void gameState() {
   //Serial.println("IsActive: "+String(gameIsActive));    
   Serial.println("Step "+String(gameCurrrentStep)+" of "+String(gameCountOfStep)+" on #"+String(gameButtonIndex));    
   //Serial.println("Button index: "+String(gameButtonIndex));    
}

void gameOver() {
    if (gameIsActive && (gameCurrrentStep >= gameCountOfStep)) {     
        Serial.println("Game over");
        gameState();
        gameIsActive = false;            
    }
}

void gameStop() {
    gameState();
    gameIsActive = false;        
}

void gameStep(){
    //Serial.println("gameStep "+String(gameCurrrentStep)+" "+String(gameButtonIndex));
    if (!gameIsActive) { return; };
    if (gameButtonIndex > NO_BUTTON_INDEX) { return; };
    //int index = rand() % button_count;
    gameCurrrentStep++;
    int index = 0;
    if (gameCurrrentStep % 2 == 0)
        { index = 0; }
    else  
        { index = 2; }   
    gameButtonIndex = index;    
    setButton(index, HIGH, CMD_SWITCH_BY_GAME);
    delay(10);
    gameState();
    gameOver();
}

void gameStart() {
  if (gameIsActive) { return; }  
  gameIsActive = true;
  gameCurrrentStep = 0;
  gameButtonIndex = NO_BUTTON_INDEX;
  Serial.println("Game start");
  gameState();
}
