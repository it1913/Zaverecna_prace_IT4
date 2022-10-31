#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>


const char* ssid = "Lego3"; //Write your SSID
const char* password = "AdelaMartinPetraIvo";   //Write your password

const char* input_parameter = "state";
const char* input_parameter2 = "state2";

const int output = 14;
const int Push_button_GPIO = 4 ;
const int output2 = 0; //2. led a button
const int Push_button_GPIO2 = 5;

// Variables will change:
int LED_state = LOW;         
int button_state;             
int lastbutton_state = LOW;
// Once more
int LED_state2 = LOW;         
int button_state2;             
int lastbutton_state2 = LOW;      

unsigned long lastDebounceTime = 0;  
unsigned long debounceDelay = 50;    

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Output Control Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Times New Roman; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    h4 {font-size: 2.0rem;}
    p {font-size: 3.0rem;}
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
  %BUTTONPLACEHOLDER%
  %BUTTONPLACEHOLDER2%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); }
  else { xhr.open("GET", "/update?state=0", true); }
  xhr.send();
}
function toggleCheckbox2(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state2=1", true); }
  else { xhr.open("GET", "/update?state2=0", true); }
  xhr.send();
}

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var inputChecked;
      var outputStateM;
      if( this.responseText == 1){ 
        inputChecked = true;
        outputStateM = "ON";
      }
      else { 
        inputChecked = false;
        outputStateM = "OFF";
      }
      document.getElementById("output").checked = inputChecked;
      document.getElementById("outputState").innerHTML = outputStateM;
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();

  var xhttp2 = new XMLHttpRequest();
  xhttp2.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var inputChecked2;
      var outputStateM2;
      if( this.responseText == 1){ 
        inputChecked2 = true;
        outputStateM2 = "ON";
      }
      else { 
        inputChecked2 = false;
        outputStateM2 = "OFF";
      }
      document.getElementById("output2").checked = inputChecked;
      document.getElementById("outputState2").innerHTML = outputStateM;
    }
  xhttp2.open("GET", "/state2", true);
  xhttp2.send();
}, 1000 ) ;
</script>
</body>
</html>
)rawliteral";

String outputState(){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
  return "";
}

String outputState2()
{
if(digitalRead(output2))
{
return "checked";
}
else
{
return "";
}
return "";
}

String processor(const String& var){
  if(var == "BUTTONPLACEHOLDER"){
    String buttons1 ="";
    String outputStateValue = outputState();
    buttons1+= "<h4>Output State: <span id=\"outputState\"></span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label>";
    return buttons1;
  }
  if(var == "BUTTONPLACEHOLDER2"){
    String buttons2 ="";
    String outputStateValue2 = outputState2();
    buttons2+= "<h4>Output State: <span id=\"outputState2\"></span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox2(this)\" id=\"output2\" " + outputStateValue2 + "><span class=\"slider\"></span></label>";
    return buttons2;
  }
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(output, OUTPUT);
  digitalWrite(output, LOW);
  pinMode(Push_button_GPIO, INPUT);

  pinMode(output2, OUTPUT);
  digitalWrite(output2, LOW);
  pinMode(Push_button_GPIO2, INPUT);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }


  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());


  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String input_message;
    String inputParameter;
    String input_message2;
    String inputParameter2;

    // GET input1 value on <ESP_IP>/update?state=<input_message>
    if (request->hasParam(input_parameter)) {
      input_message = request->getParam(input_parameter)->value();
      inputParameter = input_parameter;
      digitalWrite(output, input_message.toInt());
      LED_state = !LED_state;
    }
    else {
      input_message = "No message sent";
      inputParameter = "none";
    }
    Serial.println(input_message);
    request->send(200, "text/plain", "OK");

    if (request->hasParam(input_parameter2)) {
      input_message2 = request->getParam(input_parameter2)->value();
      inputParameter2 = input_parameter2;
      digitalWrite(output2, input_message2.toInt());
      LED_state2 = !LED_state2;
    }
    else {
      input_message2 = "No message sent";
      inputParameter2 = "none";
    }
    Serial.println(input_message2);
    request->send(200, "text/plain", "OK");
  });

  
  server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(digitalRead(output)).c_str());
  });

   server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(digitalRead(output2)).c_str());
  });
  // Start server
  server.begin();
}
  
void loop() {
  int data = digitalRead(Push_button_GPIO);
  int data2 = digitalRead(Push_button_GPIO2);

  if (data != lastbutton_state) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (data != button_state) {
      button_state = data;


      if (button_state == HIGH) {
        LED_state = !LED_state;
      }
    }
    if (data2 != button_state2) {
      button_state2 = data2;


      if (button_state2 == HIGH) {
        LED_state2 = !LED_state2;
      }
    }
  }

  
  digitalWrite(output, LED_state);
  digitalWrite(output2, LED_state2);


  lastbutton_state = data;
  lastbutton_state2 = data2;
}