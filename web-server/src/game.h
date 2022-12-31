const int STATE_OFF = 0;
const int STATE_ON = 1;
const int STATE_OVER = 2;

typedef std::function<void()> ClientCallback;

struct Step
{
	int buttonIndex;
	bool done;
    uint32_t time;
};

typedef struct Step Step;

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
        Step* _step;
        
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
            delete[] _step;
            _step = new Step[_stepCount];
            for (int i = 0; i < _stepCount; i++) {
                _step[i].buttonIndex = NO_BUTTON_INDEX;
                _step[i].done = false;
                _step[i].time = 0;
            }
        }

        int getStepDone() {
            return _stepDone;
        }

        void clearStepDone() {
            _stepDone = 0;
            _prevButtonIndex = NO_BUTTON_INDEX;
            setButtonIndex(NO_BUTTON_INDEX);
        }

        void incStepDone() {
            int index = _stepDone;
            _stepDone++;
            _modified = true;
            _prevButtonIndex = _buttonIndex;
            _step[index].buttonIndex = _buttonIndex;
            _step[index].done = true;
            _step[index].time = millis() - _startTime;            
            Serial.println("Step #"+String(_stepDone)+" done in "+stopwatch(_step[index].time)+" s"); 
            setButtonIndex(NO_BUTTON_INDEX);
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
                        clearStepDone();
                        break;
                    case STATE_OFF:
                        _stopTime = millis();                        
                        clearStepDone();
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
            _prevButtonIndex = NO_BUTTON_INDEX;
            for (int i = 0; i < getStepCount(); i++) {
                setButtonIndex(nextButtonIndex());
                Serial.println("go from "+String(_prevButtonIndex)+" to "+String(getButtonIndex()));                
                _prevButtonIndex = getButtonIndex();
            }
        }
    public:      
        bool isButtonIndex(int value) {
            return (_buttonIndex == value);
        }

        void setButtonDown() {            
            incStepDone();              
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
        ~Game() {
            delete[] _step;
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

        void init() {
            SendData data;
            for(int i = 0; i< button_count; i++){
                button[i].state = LOW;
                button[i].enabled = DISABLED;
                data.state = WHO_IS_HERE_STATE;
                data.id = button[i].id;
                data.value = i;
                data.command = CMD_WHO_IS_HERE;
                data.response = RESP_I_AM_HERE;
                esp_now_send(button[i].address.bytes, (uint8_t *) &data, sizeof(data));
            }
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
            init();
            request->send(200, "text/plain", "OK");
        }

        void handleInit(AsyncWebServerRequest *request){
            stop();
            init();
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
            //obsluha zprav od klientů a pokud dojde ke zmene stavu, 
            //tak se zavola callback na promitnuti zpet.
            clientCallback();
            return 0;
        }

        String stepData(int step) {
            String s;
            String t = "";
            if ((step < getStepDone()) || isOver()) {
                s = "done";
                if (_step[step].done) t = " in " + stopwatch(_step[step].time)+" s";
            } else
            if (step > getStepDone()) {
                s = "waiting";
            } else {
                s = "current";
            };
            s = "<div class=\"data "+s+"\">Step #" + String(step+1) + " is " + s + t + "</div>";
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
                "<span id=\"timewatch\">" + getTimeText() + "</span><br>" +
                "</div>" + stepsData();
        }
};
