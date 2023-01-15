//Game.state
const int STATE_OFF = 0;
const int STATE_ON = 1;
const int STATE_OVER = 2;

//Game.postData
const int POSTDATA_OK = 0;
const int POSTDATA_NO_DATA = -1;
const int POSTDATA_NOT_CONNECTED = -2;
const int POSTDATA_BAD_SERVER = -3;

//Game.nextSelect
const int NEXT_SELECT_SEQUENCE = 0;
const int NEXT_SELECT_RANDOM = 1;
const int NEXT_SELECT_PIVOT_AND_RANDOM = 2;

const int FIRST_STEP = 1;
const int FIRST_BUTTON_INDEX = 0;

typedef std::function<void()> ClientCallback;

struct Step
{
	int buttonIndex;
	bool done;
    ExerciseTime time;
    int id() {
        return buttonIndex == NO_BUTTON_INDEX ? 0 : button[buttonIndex].id;
    }
    void set(int AButtonIndex, bool ADone, ExerciseTime ATime ) {
        buttonIndex = AButtonIndex;
        done = ADone;
        time = ATime;  
    }                      

    void init() {
        set(NO_BUTTON_INDEX, false, 0);
    }
    void setDone(int AButtonIndex, ExerciseTime ATime) {
        set(AButtonIndex, true, ATime);
    }                      
};

typedef struct Step Step;

class Game {
    protected:
        //definicni informace
        ExerciseId _exerciseId;             //identifikator cviceni
        SessionId _sessionId;               //poradove cislo cviceni
        ParticipantId _participantId;       //aktualni ucastnik
        int _stepCount;                     //celkovy pocet kroku
        bool _sameButtonIsAllowed;          //mohou sousedni kroky pouzit stejne tlacitko?
        bool _firstButtonIsRandom;          //tlacitko v prvnim kroku je nahodne nebo pevne urcene?
        bool _startAfterFirstStep;          //mereni casu zacina az po stisku prvniho tlacitka?
        int _nextSelect;

        //data
        String _exerciseList;               //seznam definovanych cviceni
        String _participantList;            //seznam ucastniku

        //stavove informace
        int _state;                         //stav hry
        int _buttonIndex;                   //index aktualniho tlacitka        
        int _prevButtonIndex;               //index tlacitka z minuleho kroku
        int _stepDone;                      //pocet splnenych kroku
        bool _modified;                     //od posledniho callbacku doslo ke zmene stavu
        bool _postData;                     //priznak "je potreba ulozit data"
        ExerciseTime _startTime;
        ExerciseTime _stopTime;
        Step* _step;
        
        ClientCallback _clientCallback;

        void define(ExerciseId exerciseId, 
                    bool sameButtonIsAllowed, 
                    bool firstButtonIsRandom, 
                    bool startAfterFirstStep,
                    int nextSelect) {        
            _exerciseId = exerciseId;
            _sameButtonIsAllowed = sameButtonIsAllowed;
            _firstButtonIsRandom = firstButtonIsRandom;
            _startAfterFirstStep = startAfterFirstStep;
            _nextSelect = nextSelect;
        }                        

        void init(SessionId sessionId, 
                  ParticipantId participantId,
                  int stepCount,
                  int state) {        
            _sessionId = sessionId;
            _participantId = participantId;            
            _step = nullptr;
            setStepCount(stepCount);
            setState(state);            
            initButtons();
        }

        void initButtons() {
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
                delay(10);
            }
        }

        int getButtonIndex() {
            return _buttonIndex;
        }

        void setButtonIndex(int value) {
            _buttonIndex = value;
        }

        bool isModified() {
            return _modified;
        }

        void setModified(bool value) {
            _modified = value;
            if (_modified) _postData = true;
        }

        int getStepCount() {
            return _stepCount;
        }

        void setStepCount(int value) {
            _stepCount = value;
            delete[] _step;
            _step = new Step[_stepCount];
            for (int i = 0; i < _stepCount; i++) {
                _step[i].init();
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
            ExerciseTime now = millis(); 
            int index = _stepDone;
            _stepDone++;            
            if ((_stepDone==FIRST_STEP) && getStartAfterFirstStep()) {
                _startTime = now;
            }
            setModified(true);
            _prevButtonIndex = _buttonIndex;
            _step[index].setDone(_buttonIndex, now - _startTime);
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
                setModified(true); 
                switch (_state) {
                    case STATE_OVER:
                        _stopTime = millis();
                        break;
                    case STATE_ON:
                        _startTime = millis();
                        _sessionId++;
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
                    return "vypnuté";
                    break;
                case STATE_ON:
                    return "aktivní";
                    break;
                case STATE_OVER:
                    return "dokončené";
                    break;
                default:
                    return "UNKNOWN";
            }            
        }

        bool getFirstButonIsRandom () {
            return _firstButtonIsRandom;
        }

        bool getStartAfterFirstStep () {
            return _startAfterFirstStep;
        }

        bool isRandomSelection() {
            return (_nextSelect == NEXT_SELECT_RANDOM);
        }

        bool isPivotAndRandomSelection() {
            return (_nextSelect == NEXT_SELECT_PIVOT_AND_RANDOM);
        } 

        bool isValidButtonIndex(int index) {
            delay(10);
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
            int index;
            if (!getFirstButonIsRandom() && (getStepDone() == 0)) {
                Serial.println("first");
                index = FIRST_BUTTON_INDEX;
            } else 
            if (isRandomSelection()) {
               index = randomEnabledButtonIndex(_prevButtonIndex);
            } else 
            if (isPivotAndRandomSelection()) {
                if (_prevButtonIndex == FIRST_BUTTON_INDEX) {
                    index = randomEnabledButtonIndex(FIRST_BUTTON_INDEX);
                } else {
                    index = FIRST_BUTTON_INDEX;
                }
            } else {
                bool over = false;
                index = _prevButtonIndex; 
                do {
                    index = index+1 < button_count ? index+1 : FIRST_BUTTON_INDEX;
                    over = isValidButtonIndex(index) || (index == _prevButtonIndex); 
                } while (!over);
            };
            if (isValidButtonIndex(index)) return index;
            int i;
            i = index+1;
            while (i<button_count) {
                if (isValidButtonIndex(i)) return i;
                i++;
            };
            i = index-1;
            while (i>=0) {
                if (isValidButtonIndex(i)) return i;
                i--;
            };
            Serial.println("no next button index");
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

        Game() { 
            define(0, false, false, false, NEXT_SELECT_SEQUENCE);
            init(0, 0, 0, STATE_OFF);
        } 

        ~Game() {
            delete[] _step;
        }  

        void start(ExerciseId exerciseId, SessionId sessionId, ParticipantId participantId, int stepCount) {
            if (isActive()) { return; } 
            Serial.println("start");
            switch (exerciseId) {
                case EXERCISE_STOPWATCH1:
                    //
                    define(exerciseId, false, false, false, NEXT_SELECT_SEQUENCE);
                    stepCount = 1;
                    break;
                case EXERCISE_STOPWATCH2:
                    //
                    define(exerciseId, false, false, true, NEXT_SELECT_SEQUENCE);
                    stepCount = 2;
                    break;
                case EXERCISE_SHUTTLERUN:
                    //
                    define(exerciseId, false, false, true, NEXT_SELECT_SEQUENCE);
                    break;
                case EXERCISE_FAN:
                    //
                    define(exerciseId, false, false, true, NEXT_SELECT_PIVOT_AND_RANDOM);
                    break;
                case EXERCISE_REACTION:
                    //               
                    define(exerciseId, false, true, false, NEXT_SELECT_RANDOM);
                    break;
            };
            Serial.println("hra=" + String(_exerciseId));
            init(sessionId, participantId, stepCount, STATE_ON);
            me_("start after switch");
        }

        void me_(String name) {
            Serial.println("*** name="+name+" ***"); 
            Serial.println("exerciseId="+_exerciseId);
            Serial.println("sessionId="+String(_sessionId));
            Serial.println("participantId="+_participantId);
            Serial.println("stepCount="+String(_stepCount));
            Serial.println("state="+String(_state));
            Serial.println("buttonIndex="+String(_buttonIndex));
            Serial.println("prevButtonIndex="+String(_prevButtonIndex));
            Serial.println("stepDone="+String(_stepDone));
            Serial.println("sameButtonIsAllowed="+String(_sameButtonIsAllowed));
            Serial.println("firstButtonIsRandom="+String(_firstButtonIsRandom));
            Serial.println("startAfterFirstStep="+String(_startAfterFirstStep));
            Serial.println("nextSelect="+String(_nextSelect));
            buttons();
        }

        void stop() {
            //zastaveni/ukonceni hry
            setState(STATE_OFF);   
        }

        void stopWatch() {
        }

        void loop() {
            internalLoop();
            post();
        }

        void internalLoop(){
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

        int intParam(AsyncWebServerRequest *request, String paramName, int defaultValue) {
            return request->hasParam(paramName) ? 
                   request->getParam(paramName)->value().toInt() : defaultValue; 
        }

        String strParam(AsyncWebServerRequest *request, String paramName, String defaultValue) {
            return request->hasParam(paramName) ? 
                   request->getParam(paramName)->value() : defaultValue; 
        }

        void handleStart(AsyncWebServerRequest *request){
            start(intParam(request, "exerciseId", _exerciseId),
                  intParam(request, "sessionId", _sessionId),
                  intParam(request, "participantId", _participantId),                
                  intParam(request, "stepCount", 10));
            request->send(200, "text/plain", "OK");
        }

        void handleStop(AsyncWebServerRequest *request){
            stop();
            initButtons();            
            request->send(200, "text/plain", "OK");
        }

        void handleInit(AsyncWebServerRequest *request){
            stop();
            initButtons();   
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
            if (_clientCallback && isModified()) {
                setModified(false);
                _clientCallback();
            }
        }

        bool areDataReady() {
            //jsou k dispozici data k odeslani do uloziste?
            return (_postData && getStepDone()); 
        }

        String getData() {
            Record record(_exerciseId, _sessionId, _participantId);
            record.init(_stepDone,_step[_stepDone-1].time,_state);
            return record.data();
        }

        int post() {
            //odeslani dat do uloziste
            if (!areDataReady()) return POSTDATA_NO_DATA;
            _postData = false;
            return internalPostData(getData());
        }

        int internalPostData(String data) {
            //Tento callback se vola pri pozadavku na ulozeni dat do uloziste
            const String SERVER_NAME = LIGHTCONE_URL;
            const String PATH = "api?"+data;

            //Check WiFi connection status
            if (WiFi.status() == WL_CONNECTED) {
                WiFiClient client;
                HTTPClient http;
                String url = SERVER_NAME + PATH + data;

                int httpResponseCode;

                if (http.begin(client, url)) {

                    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
                    String httpRequestData = "data="+data;           

                    // If you need an HTTP request with a content type: application/json, use the following:
                    //http.addHeader("Content-Type", "application/json");
                    //int httpResponseCode = http.POST("{\"api_key\":\"tPmAT5Ab3j7F9\",\"sensor\":\"BME280\",\"value1\":\"24.25\",\"value2\":\"49.54\",\"value3\":\"1005.14\"}");

                    // If you need an HTTP request with a content type: text/plain
                    //http.addHeader("Content-Type", "text/plain");
                    //int httpResponseCode = http.POST("Hello, World!");        

                    httpResponseCode = http.POST(httpRequestData);
                    
                    if (httpResponseCode>0) {
                        // Serial.print("HTTP Response code: ");
                        // Serial.println(httpResponseCode);
                        // String payload = http.getString();
                        // Serial.println(payload);
                    }
                    else {
                        Serial.print("Error code: ");
                        Serial.println(httpResponseCode);
                    }
                } else {        
                    httpResponseCode = POSTDATA_BAD_SERVER;
                }
                // Free resources
                http.end(); 
                return httpResponseCode;     
            } else {
                Serial.println("WiFi Disconnected");
                return POSTDATA_NOT_CONNECTED;
            }
        }

        String GET(String url) {
            if (WiFi.status() == WL_CONNECTED) {
                WiFiClient client;
                HTTPClient http;

                if (http.begin(client, url)) {

                    http.addHeader("Content-Type", "text/plain; charset=UTF-8");
                    int httpCode = http.GET();
                    
                    if (httpCode>0) {
                      return http.getString();
                    } else {
                        Serial.println("HTTP CODE:" + httpCode);
                    }
                }
                // Free resources
                http.end(); 
            }
            return "Error " + url;
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
            String b = "";
            String s;
            String c;
            String t = "";
            if ((step < getStepDone()) || isOver()) {
                s = "dokončen";
                c = "list-group-item-success";
                if (_step[step].done) {
                    t = " za " + stopwatch(_step[step].time)+" s";
                    b = " na #" + String(_step[step].id());                      
                }
            } else
            if (step > getStepDone()) {
                s = "připraven";
                c = "list-group-item-warning";
            } else {
                s = "aktivní";
                c = "active";
            };
            s = "<li class=\"list-group-item "+c+"\">Krok #" + String(step+1) + b + " je " + s + t + "</li>";
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

        String progressBar() {
            String s = String(_stepDone) + " z " + String(_stepCount);
            return 
                "<div class=\"container p-1 my-1\">"
                "  <div class=\"progress\" style=\"height:30px\">"
                "    <div class=\"progress-bar bg-success\" style=\"width:1%;height:30px\">" + s + "</div>"
                "  </div>"
                "</div>";
        }

        String data() {
            String s = "Cvičení je " + stateText();
            if (!isOff()) {
                s += "<br>Krok: " + String(_stepDone) + " z " + String(_stepCount) + "<br>" +
                     "<span id=\"timewatch\">Čas: " + getTimeText() + " s</span>";
            }
            return "<ul class=\"list-group\">"
                   "<li class=\"list-group-item list-group-item-primary\">" + s + "</li>" +
                   stepsData() +
                   "</ul>";
        }

        String getDataList(String name) {
            String url = LIGHTCONE_URL;
            return GET(url+name);
        }

        void load() {
            _exerciseList = getDataList("exercise.txt");
            _participantList = getDataList("participant.txt");
        }

        String getExerciseList() {
            return _exerciseList;
        }

        String getParticipantList () {
            return _participantList;
        }
};
