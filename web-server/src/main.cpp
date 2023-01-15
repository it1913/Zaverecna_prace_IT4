#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <espnow.h>
#include <common.h>
#include <db.h>
#include <game.h>

int self_id;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
Game game;

int light(Button btn) {
  //digitalWrite(pin_LED, btn.state);
  //Serial.printf("Button #%d: state of LED %d = %d",btn.id,pin_LED,btn.state);
  //Serial.println();
  return btn.state;
}

void notifyClients() {
  ws.textAll(game.notifyText());
}

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  if (LOG_SEND && (sendStatus != 0)){
    Serial.print("Last Packet Send Status: ");
    Serial.println("Delivery fail, sendStatus = " + String(sendStatus));
  }
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&recvData, incomingData, sizeof(recvData));
  if (LOG_RECV) {
    Serial.print("ID tlačítka je: ");
    Serial.println(recvData.id);
  }    
  int index = recvData.id-1;
  button[index].state = recvData.state;
  button[index].enabled = ENABLED;
  if (LOG_RECV){
    Serial.println("recv (cmd,resp,state)=("+String(recvData.command)+","+String(recvData.response)+","+String(recvData.state)+")");
  }
  if (recvData.command == CMD_WHO_IS_HERE) {
   //button[index].enabled = (recvData.response == RESP_I_AM_HERE);
  } else
  if (recvData.command == CMD_WHO_WILL_PLAY) {
   //button[index].playing = (recvData.response == RESP_I_WILL_PLAY);
  }
  light(button[index]);
  if ((recvData.command == CMD_SWITCH_STATE) and (game.isButtonIndex(index)) and (recvData.state == LOW)) {
    if (LOG_RECV) {
      Serial.println("End of waiting for press button #"+String(button[index].id));
    }
    game.setButtonDown();
    recvData.command = CMD_UNDEFINED;
  }
}

/* Web sockets: 
  Zpracovano podle prikladu: 
  https://randomnerdtutorials.com/esp32-websocket-server-arduino/?fbclid=IwAR3p-KTpddU7N6ht95dzeSMY4_dnYLVAUVALZK73QKHJWcdE5932p6gn7fw 
*/

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (game.handleWebSocketMessage(arg, data, len) == 0) {      
      //zatim nic
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

/* Web sockets */

void gameCallback() {
  //Pri kazde zmene hodnoty Game.state se zavola tento callback
  notifyClients();
}

void initGame() {
  game.setClientCallback(&gameCallback);
  game.load();
}
void initMDNS(String name) {
  if (!MDNS.begin(name)) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started: "+name);
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="cz">
<head>
  <title>%TILE%</title>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/css/bootstrap.min.css" rel="stylesheet">
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/js/bootstrap.bundle.min.js"></script>
  <style>
    #timewatch { font-weight: bold; font-size: 2em; }
  </style>
</head>
<body>
  <div class="container-sm mt-5">
    <div class="mt-4 p-5 bg-primary text-white rounded">
      <h1>%TITLE%</h1>
      <p>Stiskni a běž!</p>
      <div class="row">
        <div class="col-sm-12">
          <button type="button" class="btn btn-warning btn-lg" onclick="initGame()">Připravit</button>
          <button type="button" class="btn btn-success btn-lg" onclick="startGame()">Start</button>
          <button type="button" class="btn btn-danger btn-lg" onclick="stopGame()" >Stop</button>
        </div>
      </div>  
    </div>  
  </div>
    
  <div class="container-sm mt-5">
    <h3>Parametry</h3>  
    <form>      
      <div class="row">          
        <div class="col-sm-6">
          <div class="input-group input-group-lg">
            <span class="input-group-text">Hráč</span>
            <select class="form-select" id="participantId">
              <option value="1">Adam</option>
              <option value="2">Břetislav</option>
              <option value="3">Cyril</option>
            </select>
            <input type="number" class="form-control" placeholder="Session" id="sessionId" min="1" value="1" disabled>
          </div>
        </div>
        <div class="col-sm-6">
          <div class="input-group input-group-lg">
            <span class="input-group-text">Cvičení</span>
            <select class="form-select" id="exerciseId">
              <option value="1">Stopky 1</option>
              <option value="2">Stopky 2</option>
              <option value="3">Člunkový běh</option>
              <option value="4">Vějíř</option>
              <option value="5" selected>Postřeh</option>
            </select>
            <input type="number" class="form-control" placeholder="Počet kroků" id="stepCount" min="1" max="100" value="10">
          </div>
        </div>
      </div>
    </form>            
  </div>

  <div class="container-sm mt-5">
    <div class="row">
      <div class="col-sm-8">
        <h3>Výsledky</h3>
        <div id="game">
          <ul class="list-group">
            %GAMEDATA%
          </ul>        
        </div>
      </div>
      <div class="col-sm-4">
        <h3>Stav tlačítek</h3>
        <div class="btn-group-vertical" id="buttons">
          %BUTTON%
        </div>    
      </div>
    </div>
  </div>

<script>
  let initAudio = new Audio("%LIGHTCONE_URL%init.mp3");
  initAudio.preload="auto";
  let startAudio = new Audio("%LIGHTCONE_URL%start.mp3");
  startAudio.preload="auto";
  let stepAudio = new Audio("%LIGHTCONE_URL%step.mp3");
  stepAudio.preload="auto";
  let overAudio = new Audio("%LIGHTCONE_URL%over.mp3");
  overAudio.preload="auto";
  let stopAudio = new Audio("%LIGHTCONE_URL%stop.mp3");
  stopAudio.preload="auto";


  function playSound(audioObj, currentTime){
      let audio = audioObj.cloneNode();
      audio.currentTime = currentTime;
      audio.play();
  }

  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    let el = document.getElementById('game');
    if (el) {
      el.innerHTML = event.data;
      playSound(stepAudio, 0.3);
      let isOver = event.data.includes("dokončené");
      if (isOver) playSound(overAudio, 0.3);
    }
  }        
  function onLoad(event) {
    initWebSocket();
  }
  function clearGame() {
    let el = document.getElementById('game');
    while (el.lastElementChild) {
      el.removeChild(game.lastElementChild);
    }
  }

setInterval(function readState( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      const state = this.responseText.split(',').map(Number);
      const buttons = document.getElementById("buttons");
      const elements = buttons.querySelectorAll('input');      
      elements.forEach((el, index) => { 
        el.checked = Boolean(state[index] & 1); 
        el.disabled = Boolean(state[index] & 2);
      });
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();  
},200);

const Milliseconds = 1000; //interval pro aktualizaci stavu diod

function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked)
       { xhr.open("GET", "/update?state=1&id="+element.id, true); }
  else { xhr.open("GET", "/update?state=0&id="+element.id, true); }
  xhr.send();
  //readState();
}

function initGame(element){
  clearGame();
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      const state = this.responseText.split(',').map(Number);
      const buttons = document.getElementById("buttons");
      const elements = buttons.querySelectorAll('input');      
      elements.forEach((el, index) => { 
        el.checked = Boolean(state[index] & 1); 
        el.disabled = Boolean(state[index] & 2);
      });
    }
  };
  xhr.open("GET", "/init", true);
  xhr.send();
  playSound(initAudio, 0.3);
}

function startGame(element){ 
  let sc = document.getElementById("stepCount").value;
  let ei = document.getElementById("exerciseId").value;
  let si = document.getElementById("sessionId").value;
  let pi = document.getElementById("participantId").value;
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  xhr.open("GET", `/startGame?stepCount=${sc}&exerciseId=${ei}&sessionId=${si}&participantId=${pi}`, true);
  xhr.send();  
  playSound(startAudio, 0.3);
}

function stopGame(element){  
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  xhr.open("GET", "/stopGame", true);
  xhr.send();  
  playSound(stopAudio, 0.3);
}

</script>
</body>
</html>
)rawliteral";

String checkBox(Button button) {
  String checked = (button.state == HIGH ? " checked" : "");
  String id = String(button.id);
  String checkboxId = "cb" + id;
  String disabled = (button.enabled ? "" : " disabled");
  String s = "<div class=\"form-check form-switch\">";
  return s +
         "<input class=\"form-check-input\" type=\"checkbox\" id=\"" + checkboxId + "\" onchange=\"toggleCheckbox(this)\"" + checked + disabled + ">" +
         "<label class=\"form-check-label\" for=\"" + checkboxId + "\">Tlačítko #" + id +"</label>" +
         "</div>";
}

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

String processor(const String &var)
{
  if (var == "TITLE") {
    return "Velká modrá tlačítka";
  } else
  if (var == "BUTTON") {
    String buttons = "";
    for (int i = 0; i < button_count; i++)
    {
      buttons += checkBox(button[i]);
    }
    return buttons;
  } else
  if (var == "LIGHTCONE_URL") {
    return LIGHTCONE_URL;
  } else
  if (var == "GAMEDATA") {
    return game.data();
  } else
  if (var == "EXERCISE") {
    return game.getExerciseList();
  } else
  if (var == "PARTICIPANT") {
    return game.getParticipantList();
  }
  return String("");
}

void setup()
{
  pinMode(pin_button,INPUT_PULLUP);
  pinMode(pin_LED, OUTPUT);

  // Serial port for debugging purposes
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  String mac = WiFi.macAddress();
  int32_t wifi_channel = getWiFiChannel(WIFI_SSID);

  Serial.print("IP Address : ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC Address: ");
  Serial.println(mac);
  
  // Start server
  if (check(esp_now_init(),"esp_now_init") != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  check(esp_now_set_self_role(ESP_NOW_ROLE_COMBO),"esp_now_set_self_role");
  check(esp_now_register_recv_cb(OnDataRecv),"esp_now_register_recv_cb");
  check(esp_now_register_send_cb(OnDataSent),"esp_now_register_send_cb");

  for (int i=0; i< button_count;  i++) {
    String pmac = mac2str(button[i].address);
    if (pmac != mac) {
      Serial.printf("#%d at ",button[i].id);
      Serial.println(pmac);
      if (check(esp_now_add_peer(button[i].address.bytes, ESP_NOW_ROLE_COMBO, wifi_channel, NULL, 0),"esp_now_add_peer") != 0) {
        Serial.println("ESP_NOW: There was an error registering the slave "+String(i));
      }
    } else {
      self_id = button[i].id;
    }
  }
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, processor); });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String input_state;
    String input_id;
    String parameter_state = "state";
    String parameter_id = "id";

      // GET input1 value on <ESP_IP>/update?state=<input_state>&id=<input_id>
      if (request->hasParam(parameter_state) && request->hasParam(parameter_id))
      {
        input_state = request->getParam(parameter_state)->value();
        input_id = request->getParam(parameter_id)->value();
        for (int i=0; i< button_count;  i++) {
          if ("cb"+String(button[i].id) == input_id) {
            button[i].state = input_state.toInt();
            sendData.state = button[i].state;
            sendData.id = button[i].id;
            sendData.value = i;
            sendData.command = CMD_SWITCH_STATE;
            sendData.response = RESP_I_SWITCHED_STATE;
            if (button[i].id == self_id) {
              light(button[i]);
            } else {
              //Serial.println("before sending button id=" + String(sendingData.id));
              Serial.println("sending to #" + String(button[i].id) 
                                      + " " + mac2str(button[i].address)
                                      + " state " + String(button[i].state));
              esp_now_send(button[i].address.bytes, (uint8_t *) &sendData, sizeof(sendData));
              //Serial.println(" after sending button id=" + String(sendingData.id));
            } 
            //Serial.println("update button "+input_id + " = " + input_state);
            request->send(200, "text/plain", "OK");
          }
        }
      }
    });

  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      String message = "";
      for (int i=0; i< button_count;  i++) {
        message += String(button[i].state+2*!button[i].enabled) + ",";
      } 
      //Serial.println("read state "+ message);
      request->send(200, "text/plain", message.c_str());
    });

  server.on("/init", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    game.handleInit(request);
  });

  server.on("/startGame", HTTP_GET, [](AsyncWebServerRequest *request)
  {
      game.handleStart(request);
  });

  server.on("/stopGame", HTTP_GET, [](AsyncWebServerRequest *request)
  {
      game.handleStop(request);
  });

  initWebSocket();
  initGame();
  initMDNS("lightcone");

  server.begin();

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}

void loop()
{
  MDNS.update();
  ws.cleanupClients();
  game.loop();
}
