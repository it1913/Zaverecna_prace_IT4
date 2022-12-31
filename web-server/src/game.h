const int STATE_OFF = 0;
const int STATE_ON = 1;
const int STATE_OVER = 2;

typedef std::function<void(int value)> StateCallback;

class Game {
    private:
        int state;
        StateCallback stateCallback;
        void setState(int value) { 
            if (state != value) {
                state = value; 
                Serial.println("GAME IS " + stateText());
            }
            if (stateCallback) stateCallback(state);
        }
        bool isValidButtonIndex(int index) {
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
        bool isOff() {      //je hra stopnuta?
            return (state == STATE_OFF);
        }
        bool isActive() {   //je hra aktivni?
            return (state == STATE_ON);
        }
        bool isOver() {     // hra skoncila?
            //test, zda uz neni konec + vraceni odpovidajici hodnoty
            if (isActive() && (currentStep > stepCount)) {             
                setState(STATE_OVER);            
            }
            return (state == STATE_OVER);
        }
        int stepCount;      //celkovy pocet kroku
        int currentStep;    //aktualni krok
        int buttonIndex;    //index aktualniho tlacitka

        Game(int AStepCount) { 
            buttonIndex = NO_BUTTON_INDEX;
            setState(STATE_OFF);
            stepCount = AStepCount;
            currentStep = 0; 
        }; 

        void printState() {
            //Serial.println("IsActive: "+String(isActive));    
            Serial.println("Waiting press button #"+String(buttonIndex)+", step "+String(currentStep)+" of "+String(stepCount));    
            //Serial.println("Button index: "+String(buttonIndex));    
        } 

        void stop() {
            //zastaveni/ukonceni hry
            if (!isOff()){        
                printState();
                setState(STATE_OFF);        
                currentStep = 0;
            }
        }

        void loop(){
            //aktualizacni metoda volana v loopu aplikace
            if (!isActive()) { return; };
            if (buttonIndex > NO_BUTTON_INDEX) { return; };
            currentStep++;
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
            if (isActive()) { return; } 
            stepCount = AStepCount;   
            currentStep = 0;
            buttonIndex = NO_BUTTON_INDEX;
            setState(STATE_ON);
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

        void setStateCallback(StateCallback value) {
            stateCallback = value;
        }

        String notifyText() {
            //text, ktery se bude posilat na klienty pres websockety pri zmene stavu
            return data();
        }

        int handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
            //obsluha zprav od klientů
            return 0;
        }

        String stateText() {
            switch(state) {
                case STATE_OFF:
                    return "OFF";
                    break;
                case STATE_ON:
                    return "ON";
                    break;
                case STATE_OVER:
                    return "OVER";
                    break;
                default:
                    return "UNKNOWN";
            }            
        }

        String stepsData() {
            if (isOff()) return "";
            String s;            
            for (int i = 0; i < stepCount; i++) {
                if (i < currentStep) {
                    s += "<div>step #" + String(i) + " is done</div>";
                } else
                if (i == currentStep) {
                    s += "<div><strong>Step #" + String(i) + " is current</strong></div>";
                } else {
                    s += "<div>Step #" + String(i) + " is waiting</div>";
                };
            }
            return s;
        }

        String data() {
            return "<div><strong>GAME IS " + stateText() + "</strong><br>" +
                String(min(currentStep, stepCount)) + " of " + String(stepCount) + "<br>" +
                "Button index: " + String(buttonIndex) +
                "</div>" + stepsData();
        }
};
