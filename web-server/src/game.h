class Game {
    private:
        boolean isValidButtonIndex(int index) {
            return ((index>=0) && (index<button_count) && (button[index].enabled) && (index != buttonIndex));
            /*  Posledni podminka rika, ze dalsi musi byt jiny nez aktualni 
                a je pouze docasna, nez se podari odstranit chybku v metode loop,
                kde dochazi k preskovani kroku, pokus je nasledujici tlacitko stejne
                jako aktualni.
            */
        }
        int nextButtonIndex() {
            int index = rand() % button_count;
            if (isValidButtonIndex(index)) return index;
            int i;
            i = index-1;
            while (i>=0) {
                if (isValidButtonIndex(i)) return i;
                i--;
            };
            i = index+1;
            while (i<button_count) {
                if (isValidButtonIndex(i)) return i;
                i++;
            };
            return NO_BUTTON_INDEX;
        }
        void testButtonIndex() {
            //Pomocna metoda, ktera slouzi k otestovani vyberu dalsiho tlacitka
            for (int i = 0; i < stepCount; i++) {
                int prevIndex = buttonIndex;
                buttonIndex = nextButtonIndex();
                Serial.println("go from "+String(prevIndex)+" to "+String(buttonIndex));                
            }
        }
    public:        
        int isActive;       //je hra aktivni?
        int stepCount;      //celkovy pocet kroku
        int currrentStep;   //aktualni krok
        int buttonIndex;    //index aktualniho tlacitka

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
            //test, zda uz neni konec + vraceni odpovidajici hodnoty
            if (isActive && (currrentStep > stepCount)) {             
                Serial.println("Game over");
                isActive = false;            
            }
            return !isActive;
        }

        void stop() {
            //zastaveni/ukonceni hry
            if (isActive){        
                printState();
                isActive = false;        
                currrentStep = 0;
            }
        }

        void loop(){
            //aktualizacni metoda volana v loopu aplikace
            if (!isActive) { return; };
            if (buttonIndex > NO_BUTTON_INDEX) { return; };
            currrentStep++;
            if (isOver()) { return; };

            //urceni dalsiho tlacitka
            int index = nextButtonIndex();
            Serial.println("Next  button index is "+String(index));
            if (index == NO_BUTTON_INDEX) {
                //kdyz se nepodari urcit dalsi tlacitko, ukoncit hru
                stop();
                return;
            }
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