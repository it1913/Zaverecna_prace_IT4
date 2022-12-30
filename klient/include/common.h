const char *WIFI_SSID = "Lego3";
const char *WIFI_PASSWORD = "AdelaMartinPetraIvo";

struct Address {
  uint8_t bytes[6];
};

typedef struct Address DeviceAddress;

DeviceAddress mac_0 = {0xAC, 0x0B, 0xFB, 0xD9, 0x9A, 0x50}; //server
DeviceAddress mac_1 = {0x68, 0xC6, 0x3A, 0xA4, 0xC9, 0x60};
DeviceAddress mac_2 = {0xAC, 0x0B, 0xFB, 0xD9, 0x95, 0x58};
DeviceAddress mac_3 = {0xAC, 0x0B, 0xFB, 0xDC, 0x91, 0x79};

DeviceAddress mac_server = mac_0;

struct Button
{
  int id;
  int state;
  DeviceAddress address;
  bool enabled; //tlacitko reaguje na zpravu CMD_I_AM_HERE odpovedi RESP_I_AM_HERE 
  bool playing; //tlacitko reagovalo na zpravu CMD_I_WILL_PLAY odpovedi RESP_I_WILL_PLAY
};

const int NO_BUTTON_INDEX = -1;
const int WHO_IS_HERE_STATE = LOW;
const boolean LOG_RECV = false;
const boolean LOG_SEND = false;

const int RESP_NONE = 0;
const int RESP_I_AM_HERE = 1;
const int RESP_I_WILL_PLAY = 2;
const int RESP_I_SWITCHED_STATE = 3;

const int CMD_UNDEFINED = 0;
const int CMD_WHO_IS_HERE = 1;
const int CMD_WHO_WILL_PLAY = 2;
const int CMD_SWITCH_STATE = 3;
const int CMD_SWITCH_BY_GAME = 4;

const int button_count = 3;
const char pin_LED = D6;
const char pin_button = D1;

const int DISABLED = false;
const int ENABLED = true; 

const int INACTIVE = false;
const int ACTIVE = true;

Button button[button_count] = {
    {1, LOW, mac_1, DISABLED, INACTIVE},
    {2, LOW, mac_2, DISABLED, INACTIVE},
    {3, LOW, mac_3, DISABLED, INACTIVE}
};

struct Message {
  int id;
  int state;
  int value;
  int command;
  int response;
};

typedef struct Message SendData;
typedef struct Message RecvData;

SendData sendData;
RecvData recvData;

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
