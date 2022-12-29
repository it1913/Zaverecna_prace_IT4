void wait(unsigned int value) {
    Serial.println("waiting "+String(value)+" ms");
    delay(value);
}

void setButton(int index, int state) {
    Button *btn = &button[index];
    if (!btn) return;
    if (!btn->enabled) return;
    //if (!btn.playing) return;
    Serial.println("set button #"+String(btn->id)+" to "+String(state));
    SendData data;     
    btn->state = state;
    data.id = btn->id;
    data.state = btn->state;
    data.value = 0;
    data.dateTime = millis();
    data.command = CMD_SWITCH_STATE;
    data.response = RESP_I_SWITCHED_STATE;  
    check(esp_now_send(btn->address.bytes, (uint8_t *) &data, sizeof(data)),"esp_now_send");
    digitalWrite(pin_LED, btn->state);
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
        setButton(i, state);
    }
}

void runGame(){
    /**
    allButtons(LOW);
    delay(1000);
    allButtons(HIGH);
    delay(1000);
    allButtons(LOW);
    **/
    const int step_count = 9;
    int step[step_count] = {1,3,1,3,1,3,1,3,1};
    for (int i = 0; i < step_count; i++){
        int index = step[i]-1;        
        setButton(index, HIGH);
        waitButton(index, LOW);
    }
    allButtons(LOW);
}