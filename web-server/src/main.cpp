#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <espnow.h>

const int button_count = 3;
int activeButton = 0;
const char pin_LED = D6;
const char pin_button = D1;

struct Address {
  uint8_t bytes[6];
};

typedef struct Address DeviceAddress;

struct Button
{
  int id;
  int state;
  DeviceAddress address;
  int activated;
};

Button button[button_count] = {
    {1, LOW, {0x68, 0xC6, 0x3A, 0xA4, 0xC9, 0x60},0},
    {2, LOW, {0xAC, 0x0B, 0xFB, 0xD9, 0x95, 0x58},0},
    {3, LOW, {0xAC, 0x0B, 0xFB, 0xDC, 0x91, 0x79},0}
};

int self_id;

struct Message {
  int id;
  int state;
  int value;
  int typeOfMessage;
  uint32_t dateTime;
};

typedef struct Message SendingData;
typedef struct Message ReceivingData;

SendingData sendingData;
ReceivingData receivingData;

const char *WIFI_SSID = "Lego3";
const char *password = "";

AsyncWebServer server(80);

int light(Button btn) {
  //digitalWrite(pin_LED, btn.state);
  Serial.printf("Button #%d: state of LED %d = %d",btn.id,pin_LED,btn.state);
  Serial.println();
  return btn.state;
}

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
    activeButton = 1;
  }
  else{
    Serial.println("Delivery fail, sendStatus = " + String(sendStatus));
    activeButton = 0;
  }
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&receivingData, incomingData, sizeof(receivingData));
  Serial.print("ID tlačítka je: ");
  Serial.println(receivingData.id);
  int index = receivingData.id-1;
  button[index].state = receivingData.state;
  light(button[index]);
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
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Output Control Web Server</h2>
  <table>
%BUTTONPLACEHOLDER%
<tr><td><button type="button" onclick="whoIsHere()">Kontrola pripojeni</button></td></tr>
</table>
<script>
setInterval(function readState( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      const state = this.responseText.split(',').map(Number);
      const elements = document.querySelectorAll('input');      
      elements.forEach((el, index) => { 
        el.checked = state[index]; 
        document.getElementById("outputState"+(index+1)).innerHTML = (state[index] ? "ON" : "OFF" );
      });
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();  
},1000);

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
        el.checked = state[index]; 
        document.getElementById("outputState"+(index+1)).innerHTML = (state[index] ? "ON" : "OFF" );
      });
    }
  };
  xhr.open("GET", "/whoIsHere", true);
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

String mac2str(DeviceAddress addr) {
  char s[20];
  sprintf(s, "%02X:%02X:%02X:%02X:%02X:%02X",addr.bytes[0],addr.bytes[1],addr.bytes[2],addr.bytes[3],addr.bytes[4],addr.bytes[5]);  
  return s;
}

String time2str(uint32_t millis) {
  unsigned long allSeconds=millis/1000;
  int runHours=allSeconds/3600;
  int secsRemaining=allSeconds%3600;
  int runMinutes=secsRemaining/60;
  int runSeconds=secsRemaining%60;
  char s[21];
  sprintf(s,"%02d:%02d:%02d",runHours,runMinutes,runSeconds);
  return s;
}

int check(int code, String name) {
  if (code != 0) {
    Serial.println(name + " = " + String(code));
  }
  return code;
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
      buttons += "  <tr><td><h4>Output State LED #" + String(button[i].id) + ": <span id=\"" + id_outputState + "\"></span></h4>\n" +
                 "  <label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + id_output + "\" " + outputStateValue + "><span class=\"slider\"></span></label></td></tr>";
    }
    return buttons;
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
  WiFi.begin(WIFI_SSID, password);
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
            sendingData.state = button[i].state;
            sendingData.id = button[i].id;
            sendingData.value = i;
            sendingData.dateTime = millis();
            sendingData.typeOfMessage = 1;
            if (button[i].id == self_id) {
              light(button[i]);
            } else {
              //Serial.println("before sending button id=" + String(sendingData.id));
              Serial.println("sending to #" + String(button[i].id) 
                                      + " " + mac2str(button[i].address)
                                      + " state " + String(button[i].state));
              esp_now_send(button[i].address.bytes, (uint8_t *) &sendingData, sizeof(sendingData));
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
        message += String(button[i].state) + ",";
      } 
      //Serial.println("read state "+ message);
      request->send(200, "text/plain", message.c_str());
    });

  server.on("/whoIsHere", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    String message = "";
    for(int i = 0; i< button_count; i++){
      sendingData.state = 0;
      sendingData.id = button[i].id;
      sendingData.value = i;
      sendingData.dateTime = 0;
      sendingData.typeOfMessage = 0;
      esp_now_send(button[i].address.bytes, (uint8_t *) &sendingData, sizeof(sendingData));
      if(activeButton==1){
        button[i].activated = activeButton;
      }
      else{button[i].activated = 0;
      }
      message += String(button[i].activated) + ",";
      Serial.println("Je tlacitko #"+String(i) +" aktivni: "+ String(button[i].activated));
    }
    request->send(200, "text/plain", message.c_str());
  });

  server.begin();
}

void loop()
{
}
