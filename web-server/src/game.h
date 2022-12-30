class Game {
    public:        
        int isActive;
        int stepCount;
        int currrentStep; 
        int buttonIndex;

        Game(int AStepCount) { 
            buttonIndex = NO_BUTTON_INDEX;
            isActive = false;
            stepCount = AStepCount;
            currrentStep = 0; 
        }; 

        void printState() {
            //Serial.println("IsActive: "+String(isActive));    
            Serial.println("Waiting press button #"+String(buttonIndex)+", step "+String(currrentStep)+" of "+String(stepCount));    
            //Serial.println("Button index: "+String(buttonIndex));    
        } 

        int isOver() {
            if (isActive && (currrentStep > stepCount)) {             
                Serial.println("Game over");
                isActive = false;            
            }
            return !isActive;
        }

        void stop() {
            if (isActive){        
                printState();
                isActive = false;        
                currrentStep = 0;
            }
        }

        void step(){
            //Serial.println("gameStep "+String(gameCurrrentStep)+" "+String(gameButtonIndex));
            if (!isActive) { return; };
            if (buttonIndex > NO_BUTTON_INDEX) { return; };
            //int index = rand() % button_count;
            currrentStep++;
            if (isOver()) { return; };
            int index = 0;
            if (currrentStep % 2 == 0)
                { index = 0; }
            else  
                { index = 2; }   
            buttonIndex = index;    
            setButton(index, HIGH, CMD_SWITCH_BY_GAME);
            delay(10);
            printState();    
        }

        void start(int AStepCount) {
            if (isActive) { return; } 
            stepCount = AStepCount;   
            currrentStep = 0;
            buttonIndex = NO_BUTTON_INDEX;
            Serial.println("Game start");
            isActive = true;
            printState();
        }

        void handleStart(AsyncWebServerRequest *request){
            const String parameter_stepCount = "stepCount"; 
            // /startGame?stepCount=<stepCount>
            if (request->hasParam(parameter_stepCount)) {
                start(request->getParam(parameter_stepCount)->value().toInt());
            }
            request->send(200, "text/plain", "OK");
        }

        void handleStop(AsyncWebServerRequest *request){
            stop();
            request->send(200, "text/plain", "OK");
        }

        void setButton(int index, int state, int command) {
            Button *btn = &button[index];
            if (!btn) return;
            //if (!button[index].enabled) return;
            //if (!button[index].playing) return;
            SendData data;
            button[index].state = state;
            data.id = button[index].id;
            data.state = state;
            data.value = index;
            data.command = command;
            data.response = RESP_I_SWITCHED_STATE;  
            check(esp_now_send(button[index].address.bytes, (uint8_t *) &data, sizeof(data)),"esp_now_send");
            /*
            Serial.println("sending to #" + String(button[index].id) 
                                        + " " + mac2str(button[index].address)
                                        + " state " + String(data.state));
            */
            //digitalWrite(pin_LED, btn->state);
        }

};

Game game(10);