//Ajust the following:

//#define DEBUG
#define VERSION              "3.0.1"       //Change this value to reset current config on the next boot...
#define DEFAULTHOSTNAME      "ESP8266"
//NOTA: no SSID declared (in web interface) will qualify me as a slave candidate...
#define DEFAULTWIFIPASS      "defaultPassword"
#define WIFISTADELAYRETRY     30000UL
#define MAXWIFIRETRY          2
#define WIFIAPDELAYRETRY      300000UL
#define REFRESH_PERIOD        30
//#define MEMORYLEAKS           10000UL
#define SSIDCount()           3
#define RESTO_VALUES_ON_BOOT  false
#define REVERSE_OUTPUT        false
#define DEBOUNCE_TIME         25UL

#define DEFAULT_MQTT_SERVER  "mosquitto.home.lan"
#ifdef  DEFAULT_MQTT_SERVER
  #define DEFAULT_MQTT_PORT   1883
  #define DEFAULT_MQTT_IDENT ""
  #define DEFAULT_MQTT_USER  ""
  #define DEFAULT_MQTT_PWD   ""
  #define DEFAULT_MQTT_QUEUE "domoticz/in"
#else
  #define NOTIFYPROXY        "domoticz.home.lan"
  #define NOTIFYPORT          8081
#endif

#define LUMIBLOC_RELAY6X_D1MINI

#ifdef LUMIBLOC_SSD_D1MINI
  #define HOLD_TO_DISABLE_TIMER 3UL       //(s)
  static std::vector<ushort>   _inputPin ={ D5, D6, D7 };             //lumibloc
  static std::vector<ushort>   _outputPin={ D1, D2, D3, D4, D0, D8 };
#endif

#ifdef LUMIBLOC_RELAY6X_D1MINI
  #define HOLD_TO_DISABLE_TIMER 3UL       //(s)
  static std::vector<ushort>   _inputPin ={ D5, D6, D7 };             //relay 6x
  static std::vector<ushort>   _outputPin={ D8, D1, D2, D3, D4, D0 };
//static std::vector<ushort>   _outputPin={ D8, D1, D2, D3, D4, D0, (ushort)-1 }; //with a virtual output...
#endif

#ifdef SIMPLESWITCH_ESP01
  #define HOLD_TO_DISABLE_TIMER 3UL       //(s)
  static std::vector<ushort>   _inputPin ={ 2 };             //ESP-01S relay 1x
  static std::vector<ushort>   _outputPin={ 0 };
#endif

#ifdef POWERSTRIP
  #define HOLD_TO_DISABLE_TIMER 5UL       //(s)
  static std::vector<ushort>   _inputPin ={ D7 };             //TUYA TYSE3S Power Strip 3x
  static std::vector<ushort>   _outputPin={ D6, D5, D8, D3, D2 };
  #define POWER_LED             D0
  #define BLINKING_POWERLED     1000UL,0UL
  #define WIFISTA_LED           D1
  #define BLINKING_WIFILED      5000UL,250UL
#endif
