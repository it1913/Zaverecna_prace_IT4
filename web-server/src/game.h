const int STATE_OFF = 0;
const int STATE_ON = 1;
const int STATE_OVER = 2;

typedef std::function<void()> ClientCallback;

class Game {
    private:
        int _state;             //stav hry
        int _buttonIndex;       //index aktualniho tlacitka        
        int _prevButtonIndex;   //
        int _stepCount;         //celkovy pocet kroku
        int _stepDone;          //pocet splnenych kroku
        bool _modified;         //od posledniho callbacku doslo ke zmene
        bool _sameButtonIsAllowed;
        uint32_t _startTime;
        uint32_t _stopTime;
        
        ClientCallback _clientCallback;

        int getButtonIndex() {
            return _buttonIndex;
        }

        void setButtonIndex(int value) {
            _buttonIndex = value;
        }

        int getStepCount() {
            return _stepCount;
        }

        void setStepCount(int value) {
            _stepCount = value;
        }

        int getStepDone() {
            return _stepDone;
        }

        void setStepDone(int value) {
            if (_stepDone == value) return;
            _stepDone = value;
            _modified = true;
            _prevButtonIndex = _buttonIndex;
            setButtonIndex(NO_BUTTON_INDEX);
            Serial.println("step done: "+String(_stepDone));                        
            if (isOver() || _stepDone) clientCallback();          
        }

        bool isState(int value) {
            return (_state == value);
        }

        int getState() {
            return _state;
        }

        void setState(int value) { 
            if (_state != value) {
                _state = value;
                _modified = true; 
                switch (_state) {
                    case STATE_OVER:
                        _stopTime = millis();
                        break;
                    case STATE_ON:
                        _startTime = millis();
                        setButtonIndex(NO_BUTTON_INDEX);
                        setStepDone(0);
                        break;
                    case STATE_OFF:
                        _stopTime = millis();                        
                        setButtonIndex(NO_BUTTON_INDEX);
                        setStepDone(0);
                        break;
                }        
                _prevButtonIndex = NO_BUTTON_INDEX;        
                Serial.println("GAME IS " + stateText());                
                clientCallback();
            }
        }

        String stateText() {
            switch(getState()) {
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

        bool isValidButtonIndex(int index) {
            return ((index>=0) && 
                    (index<button_count) && 
                    (button[index].enabled) && 
                    (_sameButtonIsAllowed ||(index != _prevButtonIndex)));
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
            for (int i = 0; i < getStepCount(); i++) {
                int prevIndex = getButtonIndex();
                setButtonIndex(nextButtonIndex());
                Serial.println("go from "+String(prevIndex)+" to "+String(getButtonIndex()));                
            }
        }
    public:      
        bool isButtonIndex(int value) {
            return (_buttonIndex == value);
        }

        void setButtonDown() {            
            setStepDone(getStepDone()+1);              
        }

        bool isOff() {      //je hra zastavena/neaktivni?
            return isState(STATE_OFF);
        }

        bool isActive() {   //je hra aktivni?
            return isState(STATE_ON);
        }

        bool isOver() {     // hra skoncila?
            //test, zda uz neni konec + vraceni odpovidajici hodnoty
            if (isActive() && (getStepDone() >= getStepCount())) {             
                setState(STATE_OVER);            
            }
            return isState(STATE_OVER);
        }

        String getTimeText() {            
            if (isOver()) {
                return stopwatch(_stopTime - _startTime);
            } else
            if (isActive()) {   
                return stopwatch(millis() - _startTime);
            }
            return "";
        }

        Game(int AStepCount) { 
            _sameButtonIsAllowed = false;
            setStepCount(AStepCount);
            setState(STATE_OFF);            
        } 

        void start(int AStepCount) {
            if (isActive()) { return; } 
            setStepCount(AStepCount);
            setState(STATE_ON);
        }

        void stop() {
            //zastaveni/ukonceni hry
            setState(STATE_OFF);   
        }

        void loop(){
            //aktualizacni metoda volana v loopu aplikace
            if (!isActive()) { return; };
            if (!isButtonIndex(NO_BUTTON_INDEX)) { return; };
            if (isOver()) { return; };

            //Urceni dalsiho tlacitka
            int index = nextButtonIndex();
            if (index == NO_BUTTON_INDEX) {
                //kdyz se nepodari urcit dalsi tlacitko, ukoncit hru
                stop();
                return;
            }
            setButtonIndex(index);    

            setButton(index, HIGH, CMD_SWITCH_BY_GAME);
            delay(10);
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

        void setClientCallback(ClientCallback value) {
            _clientCallback = value;
        }

        void clientCallback() {            
            if (_clientCallback && _modified) {
                _modified = false;
                _clientCallback();
            }
        }

        String notifyText() {
            //text, ktery se bude posilat na klienty pres websockety pri zmene stavu
            return data();
        }

        int handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
            //obsluha zprav od klientů
            return 0;
        }

        String stepData(int step) {
            String s;
            if ((step < getStepDone()) || isOver()) {
                s = "done";
            } else
            if (step > getStepDone()) {
                s = "waiting";
            } else {
                s = "current";
            };
            s = "<div class=\"data "+s+"\">Step #" + String(step+1) + " is " + s + "</div>";
            return s;
        }

        String stepsData() {
            if (isOff()) return "";
            String s;            
            for (int i = 0; i < getStepCount(); i++) {
                s += stepData(i);
            }
            return s;
        }

        String data() {
            return "<div class=\"data header\">GAME IS " + stateText() + "<br>" +
                String(_stepDone) + " of " + String(_stepCount) + "<br>" +
                "Time: " + getTimeText() + " s<br>" +
                "Button index: " + String(_buttonIndex) +                
                "</div>" + stepsData();
        }
};
