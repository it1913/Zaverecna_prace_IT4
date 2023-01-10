//Game.state
const int STATE_OFF = 0;
const int STATE_ON = 1;
const int STATE_OVER = 2;

//Game.postData
const int POSTDATA_OK = 0;
const int POSTDATA_NO_DATA = -1;
const int POSTDATA_NOT_CONNECTED = -2;
const int POSTDATA_BAD_SERVER = -3;

typedef std::function<void()> ClientCallback;

struct Step
{
	int buttonIndex;
	bool done;
    uint32_t time;
};

typedef struct Step Step;

class Game {
    protected:
        ExerciseId _exerciseId;
        SessionId _sessionId;
        ParticipantId _participantId;

        String _exerciseList;
        String _participantList;

        int _state;             //stav hry
        int _buttonIndex;       //index aktualniho tlacitka        
        int _prevButtonIndex;   //
        int _stepCount;         //celkovy pocet kroku
        int _stepDone;          //pocet splnenych kroku
        bool _modified;         //od posledniho callbacku doslo ke zmene stavu
        bool _postData;
        bool _sameButtonIsAllowed;
        bool _firstButtonIsRandom;
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
            setModified(true);
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

        bool getFirstButoonIsRandom () {
            return _firstButtonIsRandom;
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
            int index;
            if (!getFirstButoonIsRandom() && (getStepDone() == 0)) {
                index = 0;
            } else {
                index = rand() % button_count;
            };
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
            _firstButtonIsRandom = false;
            _step = nullptr;
            setStepCount(AStepCount);
            setState(STATE_OFF);            
        } 
        ~Game() {
            delete[] _step;
        }        

        void start(ExerciseId exerciseId, SessionId sessionId, ParticipantId participantId, int AStepCount) {
            if (isActive()) { return; } 
            _exerciseId = exerciseId;
            _sessionId = sessionId;
            _participantId = participantId;
            setStepCount(AStepCount);
            setState(STATE_ON);
        }

        void stop() {
            //zastaveni/ukonceni hry
            setState(STATE_OFF);   
        }

        void init(ExerciseId exerciseId, SessionId sessionId, ParticipantId participantId) {
            _exerciseId = exerciseId;
            _sessionId = sessionId;
            _participantId = participantId;
            init();
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
            start(strParam(request, "exerciseId", _exerciseId),
                  intParam(request, "sessionId", _sessionId),
                  strParam(request, "participantId", _participantId),                
                  intParam(request, "stepCount", 10));
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
            const String SERVER_NAME = "http://www.kavala.cz/martin/";
            // const String POST_URL = "button.php";
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
                    Serial.print("HTTP Response code: ");
                    Serial.println(httpResponseCode);
                    String payload = http.getString();
                    Serial.println(payload);
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

        String getDataList(String name) {
            String url = "http://www.kavala.cz/martin/";
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
