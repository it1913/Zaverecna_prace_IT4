#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

const int button_count = 4;

struct Button
{
  int button_pin;
  int button_state;
  int LED_pin;
  int LED_state;
  int last_button_state;
};

Button button[button_count] = {
    {5, LOW, 2, HIGH, LOW},
    {5, LOW, 2, HIGH, LOW},
    {D2, LOW, D6, HIGH, LOW},
    {D2, LOW, D6, HIGH, LOW},
};

const char *ssid = "Lego3";
const char *password = "AdelaMartinPetraIvo";

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

AsyncWebServer server(80);

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
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); }
  else { xhr.open("GET", "/update?state=0", true); }
  xhr.send();
}
const Milliseconds = 100; //interval pro aktualizaci stavu diod
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      const state = this.responseText.split(',').map(Number);
      const elements = document.querySelectorAll('input');      
      elements.forEach((el, index) => { 
        el.checked = state[index]; 
        document.getElementById("outputState"+index).innerHTML = (state[index] ? "ON" : "OFF" );
      });
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, Milliseconds ) ;
</script>
</body>
</html>
)rawliteral";

String outputState(int output)
{
  if (digitalRead(output))
  {
    return "checked";
  }
  else
  {
    return "";
  }
  return "";
}

String processor(const String &var)
{
  if (var == "BUTTONPLACEHOLDER")
  {
    String buttons = "";
    for (int i = 0; i < button_count; i++)
    {
      String outputStateValue = outputState(i);
      String id_outputState = "outputState" + String(i);
      String id_output = "output" + String(i);
      buttons += "<h4>Output State LED #" + String(i) + ": <span id=\"" + id_outputState + "\"></span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + id_output + "\" " + outputStateValue + "><span class=\"slider\"></span></label>";
    }
    return buttons;
  }
  return String();
}

void setup()
{
  // Serial port for debugging purposes
  Serial.begin(115200);

  for (int i = 0; i < button_count; i++)
  {
    pinMode(button[i].LED_pin, OUTPUT);
    digitalWrite(button[i].LED_pin, LOW);
    pinMode(button[i].button_pin, INPUT);
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, processor); });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String input_message;
    String inputParameter;

    for (int i=0; i< button_count;  i++) {
      // GET input1 value on <ESP_IP>/update?state=<input_message>
      String input_parameter = "state";
      if (request->hasParam(input_parameter))
      {
        input_message = request->getParam(input_parameter)->value();
        inputParameter = input_parameter;
        button[i].LED_state = input_message.toInt();
        digitalWrite(button[i].LED_pin, button[i].LED_state);
      }
      else
      {
        input_message = "No message sent";
        inputParameter = "none";
      }
    }
    Serial.println(input_message);
    request->send(200, "text/plain", "OK"); });

  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      String message = "";
      for (int i=0; i< button_count;  i++) {
        message += String(digitalRead(button[i].LED_pin)) + ",";
      } 
      request->send(200, "text/plain", message.c_str());
    });
  // Start server
  server.begin();
}

void loop()
{
  for (int i = 0; i < button_count; i++)
  {
    int button_state = digitalRead(button[i].button_pin);

    if (button_state != button[i].last_button_state)
    {
      // reset the debouncing timer
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
      if (button_state != button[i].button_state)
      {
        button[i].button_state = button_state;

        if (button[i].button_state == HIGH)
        {
          button[i].LED_state = !button[i].LED_state;
        }
      }
    }

    digitalWrite(button[i].LED_pin, button[i].LED_state);

    button[i].last_button_state = button_state;
  }
}
