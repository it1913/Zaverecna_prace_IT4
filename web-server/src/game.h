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

int gameButtonIndex = NO_BUTTON_INDEX;
int gameIsActive = false;
int gameStepCount = 4;
int gameCurrrentStep = 0;

void gameState() {
   //Serial.println("IsActive: "+String(gameIsActive));    
   Serial.println("Waiting press button #"+String(gameButtonIndex)+", step "+String(gameCurrrentStep)+" of "+String(gameStepCount));    
   //Serial.println("Button index: "+String(gameButtonIndex));    
}

void gameOver() {
    if (gameIsActive && (gameCurrrentStep > gameStepCount)) {             
        Serial.println("Game over");
        gameIsActive = false;            
    }
}

void gameStop() {
    if (gameIsActive){        
        gameState();
        gameIsActive = false;        
        gameCurrrentStep = 0;
    }
}

void gameStep(){
    //Serial.println("gameStep "+String(gameCurrrentStep)+" "+String(gameButtonIndex));
    if (!gameIsActive) { return; };
    if (gameButtonIndex > NO_BUTTON_INDEX) { return; };
    //int index = rand() % button_count;
    gameCurrrentStep++;
    gameOver();  
    if (!gameIsActive) { return; };  
    int index = 0;
    if (gameCurrrentStep % 2 == 0)
        { index = 0; }
    else  
        { index = 2; }   
    gameButtonIndex = index;    
    setButton(index, HIGH, CMD_SWITCH_BY_GAME);
    delay(10);
    gameState();    
}

void gameStart(int stepCount) {
  if (gameIsActive) { return; } 
  gameStepCount = stepCount;   
  gameCurrrentStep = 0;
  gameButtonIndex = NO_BUTTON_INDEX;
  Serial.println("Game start");
  gameIsActive = true;
  gameState();
}
