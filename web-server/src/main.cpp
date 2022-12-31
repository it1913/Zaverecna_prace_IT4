#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <espnow.h>
#include <common.h>
#include <classes.h>
#include <game.h>

int self_id;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
Game game(10);

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
  button[index].enabled = true;
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
    //notifyClients();
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
      notifyClients();
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
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Output Control Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Segoe UI; display: inline-block; text-align: center;}
    h2 {font-size: 1.5rem;}
    h4 {font-size: 1.0rem;}
    p {font-size: 1.0rem;}
    body {max-width: 900px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #FF0000; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}    
    input:checked+.slider {background-color: #27c437}
    input:disabled+.slider {background-color: gray}    
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    .row { display: flex; }
    .column { flex: 50vw; }
    button { width: 30vw; padding: 5px; margin: 5px; }
    .data { width: 30vw; padding: 5px; margin: 5px; background-color: LightSkyBlue; }
    .header { font-weight: bold; background-color: DodgerBlue; }
    .current { font-weight: bold; background-color: Gold; }
    .done { background-color: LightGrey; }
    .waiting { }
  </style>
</head>
<body>
  <h2>ESP Output Control Web Server</h2>
  <div class="row">
    <div class="column">
%BUTTONPLACEHOLDER%
      <div><button type="button" onclick="whoIsHere()">INIT</button></div>
      <div><button type="button" onclick="startGame()">START</button></div>
      <div><button type="button" onclick="stopGame()">STOP</button></div>
      <div>
        <label for="stepCount">Step count:</label>
        <input type="number" id="stepCount" min="1" max="100" value="10">
      </div>
      <div>
        <label for="cars">Choose a player:</label>
        <select id="players">
          <option value="martin">Martin</option>
          <option value="petr">Petr</option>
          <option value="pavel">Pavel</option>
          <option value="josef">Josef</option>
        </select>      
      </div>
    </div>
    <div class="column" id="game">
%GAMEDATAHOLDER%    
    </div>
  </div>
<script>
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
    }
  }        
  function onLoad(event) {
    initWebSocket();
  }

setInterval(function readState( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      const state = this.responseText.split(',').map(Number);
      const elements = document.querySelectorAll('input');      
      elements.forEach((el, index) => { 
        el.checked = Boolean(state[index] & 1); 
        el.disabled = Boolean(state[index] & 2);
        os = document.getElementById("outputState"+(index+1));
        if (os) {
          os.innerHTML = (el.checked ? "ON" : "OFF" );
        };  
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

function whoIsHere(element){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      const state = this.responseText.split(',').map(Number);
      const elements = document.querySelectorAll('input');      
      elements.forEach((el, index) => { 
        el.checked = Boolean(state[index] & 1); 
        el.disabled = Boolean(state[index] & 2);
        os = document.getElementById("outputState"+(index+1));
        if (os) {
          os.innerHTML = (el.checked ? "ON" : "OFF" );
        };  
      });
    }
  };
  xhr.open("GET", "/whoIsHere", true);
  xhr.send();
}

function startGame(element){ 
  let stepCount = document.getElementById("stepCount").value;
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  xhr.open("GET", "/startGame?stepCount="+stepCount, true);
  xhr.send();  
}

function stopGame(element){  
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  xhr.open("GET", "/stopGame", true);
  xhr.send();  
}

</script>
</body>
</html>
)rawliteral";

String outputState(Button button)
{
  //if (digitalRead(output))
  if (button.state == HIGH)
  {
    return "checked";
  }
  else
  {
    return "";
  }
  return "";
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
  if (var == "BUTTONPLACEHOLDER")
  {
    String buttons = "";
    for (int i = 0; i < button_count; i++)
    {
      String outputStateValue = outputState(button[i]);
      String id_outputState = "outputState" + String(button[i].id);
      String id_output = "output" + String(button[i].id);
      String disabled = "";
      if (!button[i].enabled) { disabled = " disabled"; };
      buttons += "  <div><h4>Output State LED #" + String(button[i].id) + ": <span id=\"" + id_outputState + "\"></span></h4>\n" +
                 "  <label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + id_output + "\" " + outputStateValue + disabled + "><span class=\"slider\"></span></label></div>";
    }
    return buttons;
  } else
  if (var == "GAMEDATAHOLDER") {
    return game.data();
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
          if ("output"+String(button[i].id) == input_id) {
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

  server.on("/whoIsHere", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    game.stop();
    for(int i = 0; i< button_count; i++){
      button[i].state = LOW;
      button[i].enabled = DISABLED;
      sendData.state = WHO_IS_HERE_STATE;
      sendData.id = button[i].id;
      sendData.value = i;
      sendData.command = CMD_WHO_IS_HERE;
      sendData.response = RESP_I_AM_HERE;
      esp_now_send(button[i].address.bytes, (uint8_t *) &sendData, sizeof(sendData));
    }
    request->send(200, "text/plain", "OK");    
  });

  server.on("/startGame", HTTP_GET, [](AsyncWebServerRequest *request)
  {
      game.handleStart(request);
      notifyClients();
  });

  server.on("/stopGame", HTTP_GET, [](AsyncWebServerRequest *request)
  {
      game.handleStop(request);
      notifyClients();
  });

  initWebSocket();
  initGame();
  server.begin();
}

void loop()
{
  ws.cleanupClients();
  game.loop();
}
