/* WiFiPowerStrip C++ for ESP8266 (Version 3.0.0 - 2020/07)
    <https://github.com/peychart/WiFiPowerStrip>

    Copyright (C) 2020  -  peychart

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this program.
    If not, see <http://www.gnu.org/licenses/>.

    Details of this licence are available online under:
                        http://www.gnu.org/licenses/gpl-3.0.html
*/
//Reference: https://www.arduino.cc/en/Reference/HomePage
//See: http://esp8266.github.io/Arduino/versions/2.1.0-rc1/doc/libraries.html
//Librairies et cartes ESP8266 sur IDE Arduino: http://arduino.esp8266.com/stable/package_esp8266com_index.json
//http://arduino-esp8266.readthedocs.io/en/latest/

#define _MAIN_
#include <Arduino.h>
#include <string.h>
#include <FS.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <TimeLib.h>
//#include <Ethernet.h>
#include <uart.h>

#include "untyped.h"              //<-- de/serialization object for JSON communications and SD card backups
#include "WiFiManager.h"          //<-- WiFi connection manager
#include "pins.h"                 //<-- pins object manager with serial pin extension manager
#include "switches.h"             //<-- input management
#ifdef DEFAULT_MQTT_BROKER
  #include "mqtt.h"               //<-- mqtt input/output manager
#endif
#ifdef DEFAULT_NTPSOURCE
  #include "ntp.h"                //<-- time-setting manager
#endif
#include "webPage.h"              //<-- definition of a web interface

#include "setting.h"              //<--Can be adjusted according to the project...
#include "debug.h"                //<-- telnet and serial debug traces

//Avoid to change the following:
pinsMap                           myPins;
switches                          mySwitches( myPins );
WiFiManager                       myWiFi;
WiFiClient                        ethClient;
mqtt                              myMqtt(ethClient);
#ifdef DEFAULT_NTPSOURCE
  ntp                             myNTP;
#endif
volatile bool                     intr(false);
volatile ulong                    rebound_completed(0L);
ESP8266WebServer                  ESPWebServer(80);
ESP8266HTTPUpdateServer           httpUpdater;

std::string Upper( std::string s )  {std::for_each(s.begin(), s.end(), [](char & c){c = ::toupper(c);}); return s;};
std::string Lower( std::string s )  {std::for_each(s.begin(), s.end(), [](char & c){c = ::tolower(c);}); return s;};
std::string ltrim( std::string s, const std::string& chars = "\t\n\v\f\r " )
                                    {s.erase(0, s.find_first_not_of(chars)); return s;};
std::string rtrim( std::string s, const std::string& chars = "\t\n\v\f\r " )
                                    {s.erase(s.find_last_not_of(chars) + 1); return s;};
std::string trim ( std::string s, const std::string& chars = "\t\n\v\f\r " )
                                    {return ltrim(rtrim(s, chars), chars);};
void reboot(){
  DEBUG_print("Restart needed!...\n");
  myPins.serialSendReboot();
  myPins.mustRestore(true).saveToSD();
  ESP.restart();
}

void mqttSendConfig( void ) {
#ifdef DEFAULT_MQTT_BROKER
  for(auto &x : myPins) myMqtt.send( untyped(MQTT_SCHEMA(x.gpio())).serializeJson(), "sendConfig" );
#endif
}

void onSwitch( void ){
//myMqtt.send( "{\"" ROUTE_PIN_STATE "\":" + myPins.isOn() + "}", "Status-changed" );
}

void onWiFiConnect() {
  myPins.master(true);
}

void onStaConnect() {
#ifdef WIFI_STA_LED
  myPins(WIFISTA_LED).set(true);
#endif
  mqttSendConfig();
}

void onStaDisconnect() {
#ifdef WIFISTA_LED
  myPins(WIFISTA_LED).set(false);
#endif
}

void ifWiFiConnected() {
#ifdef DEFAULT_NTPSOURCE
  myNTP.getTime();
#endif

  if( myPins.slave() )
    myWiFi.disconnect();
  else if( Serial && !myPins.master() && !myPins.slave() )
    myPins.serialSendMasterSearch();                  //Is there a Master here?...
}

void onConnect() {
#ifdef DEBUG
#ifdef ALLOW_TELNET_DEBUG
  telnetServer.begin();
  telnetServer.setNoDelay(true);
#endif
#endif
}

void ifConnected() {
#ifdef DEBUG
#ifdef ALLOW_TELNET_DEBUG
  if( telnetServer.hasClient() ) {                  //Telnet client connection:
    if (!telnetClient || !telnetClient.connected()) {
      if(telnetClient){
        telnetClient.stop();
        DEBUG_print("Telnet Client Stop\n");
      }telnetClient=telnetServer.available();
      telnetClient.flush();
      DEBUG_print("New Telnet client connected...\n");
      DEBUG_print("ChipID(" + String(ESP.getChipId(), DEC) + ") says to "); DEBUG_print(telnetClient.remoteIP()); DEBUG_print(": Hello World, Telnet!\n\n");
  } }
#endif
#endif
}

bool isNumeric( std::string s ) {
  if(!s.size())                     return false;
  if(s[0]== '-' || s[0]=='+') s=s.substr(1, std::string::npos);  
  if(!s.size())                     return false;
  for(auto &x : s) if(!isdigit(x))  return false;
  return true;
}

#ifdef DEFAULT_MQTT_BROKER
void mqttPayloadParse( untyped &msg, pin p=pin() ) {  //<-- MQTT inputs parser...
  for(auto &x : msg.map())
    if( "/"+x.first == ROUTE_VERSION ) {                 // Display microcode version
      myMqtt.send( "{\"version\": \"" + myWiFi.version() + "\"}" );
    }else if ( "/"+x.first == ROUTE_HOSTNAME ) {            // { "hostname": "The new host name" }
      myWiFi.hostname( x.second.c_str() );                                                    myWiFi.saveToSD();
    }else if( x.first == "ssid" ) {                   // { "ssid": {"id", "pwd"} }
      for(auto &id : x.second.map()) myWiFi.push_back( id.first.c_str(), id.second.c_str() );  myWiFi.saveToSD();
#ifdef DEFAULT_NTPSOURCE
    }else if( "/"+x.first == ROUTE_NTP_SOURCE ) {     // { "ntpSource": "fr.pool.ntp.org" }
      myNTP.source( x.second.c_str() );                                                       myNTP.saveToSD();
    }else if( "/"+x.first == ROUTE_NTP_ZONE ) {       // { "ntpZone": 10 }
      myNTP.zone( x.second );                                                                 myNTP.saveToSD();
    }else if( "/"+x.first == ROUTE_NTP_DAYLIGHT ) {   // { "ntpDayLight": false }
      myNTP.dayLight( x.second );                                                             myNTP.saveToSD();
#endif
    }else if( "/"+x.first == ROUTE_RESTART ) {        // reboot Device
      reboot();
    }else if( isNumeric(x.first) && myPins.exist( atoi(x.first.c_str()) ) ) { // it's a pin config...
      untyped u(x.second.at( atoi(x.first.c_str()) ));
      if( u.map().size() )                            // ex: { "16": { "switch": "On", "timeout": 3600, "name": "switch0", "reverse": true, "hidden": false } }
        mqttPayloadParse( u, myPins( atoi(x.first.c_str()) ) );
      else if( Upper(trim(u.c_str())) == "ON" )             // ex: { "16": "On" }
        myPins( atoi(x.first.c_str()) ).set( true );
      else if( Upper(trim(u.c_str())) == "OFF" )            // ex: { "16": "Off" }
        myPins( atoi(x.first.c_str()) ).set( false );
      else if( Upper(trim(u.c_str())) == "TOGGLE" )         // ex: { "16": "Toggle" }
        myPins( atoi(x.first.c_str()) ).set( !myPins( atoi(x.first.c_str()) ).isOn() );
    }else if( "/"+x.first == ROUTE_PIN_SWITCH )  {    // set the pin output
      p.set( x.second );
    }else if( "/"+x.first == ROUTE_PIN_VALUE )   {    // set the pin timeout
      p.timeout( x.second * 1000UL );                                                         p.saveToSD();
    }else if( "/"+x.first == ROUTE_PIN_NAME )    {    // set the pin name
      p.name( x.second.c_str() );                                                             p.saveToSD();
    }else if( "/"+x.first == ROUTE_PIN_REVERSE ) {    // set the pin reverse
      p.reverse( x.second.value<bool>() );                                                    p.saveToSD();
    }else if( "/"+x.first == ROUTE_PIN_HIDDEN )  {    // set the pin display
      p.display( !x.second.value<bool>() );                                                   p.saveToSD();
    }
}void mqttCallback(char* topic, byte* payload, unsigned int length) {
  untyped msg; msg.deserializeJson( (char*)payload );
  DEBUG_print( "Message arrived on topic: " + String(topic) + "\n" );
  DEBUG_print( String(msg.serializeJson().c_str()) + "\n" );

  mqttPayloadParse( msg );
}
#endif

//Gestion des switchs/Switches management
void ICACHE_RAM_ATTR debouncedInterrupt(){if(!intr){intr=true; rebound_completed = millis() + DEBOUNCE_TIME;}}

// ***********************************************************************************************
// **************************************** SETUP ************************************************
void setup(){
  Serial.begin(115200);
  while(!Serial);
  Serial.print("\n\nChipID(" + String(ESP.getChipId(), DEC) + ") says: Hello World!\n\n");

//myWiFi.clear().push_back("hello world", "password").saveToSD();myWiFi.clear();  // only for DEBUG...
  //initialisation des broches /pins init
  for(ushort i(0); i<2; i++){
    myWiFi.version        ( VERSION )
          .onConnect      ( onWiFiConnect )
          .onStaConnect   ( onStaConnect )
          .ifConnected    ( ifWiFiConnected )
          .onConnect      ( onConnect )
          .ifConnected    ( ifConnected )
          .onStaDisconnect( onStaDisconnect )
          .onMemoryLeak   ( reboot )
          .hostname       ( DEFAULTHOSTNAME )
          .restoreFromSD();
    if( myWiFi.version() != VERSION )
      LittleFS.format();
    else
      break;
  }myWiFi.saveToSD();
  myWiFi.connect();
  DEBUG_print( ("WiFi: " + myWiFi.serializeJson() + "\n").c_str() );

  myPins.set( OUTPUT_CONFIG ).mode(OUTPUT).onSwitch( onSwitch ).restoreFromSD("out-gpio-");
  (myPins.mustRestore() ?myPins.set() :myPins.set(false)).mustRestore(false).saveToSD();
#ifdef DEBUG
  for(auto &x : myPins) DEBUG_print( ("Pins: " + x.serializeJson() + "\n").c_str() );
#endif
  if( myPins.exist(1) || myPins.exist(3) ) Serial.end();

  mySwitches.set( INPUT_CONFIG ).mode(INPUT_PULLUP).restoreFromSD("in-gpio-"); mySwitches.saveToSD();
  mySwitches.init( debouncedInterrupt, FALLING );   //--> input traitement declared...
#ifdef DEBUG
  for(auto &x : mySwitches) DEBUG_print( ("Switches: " + x.serializeJson() + "\n").c_str() );
#endif
  if( mySwitches.exist(1) || mySwitches.exist(3) ) Serial.end();

  // Servers:
  setupWebServer();                    //--> Webui interface started...
  httpUpdater.setup( &ESPWebServer );  //--> OnTheAir (OTA) updates added...

#ifdef DEFAULT_MQTT_BROKER
  myMqtt.broker       ( DEFAULT_MQTT_BROKER )
        .port         ( DEFAULT_MQTT_PORT )
        .ident        ( DEFAULT_MQTT_IDENT )
        .user         ( DEFAULT_MQTT_USER )
        .password     ( DEFAULT_MQTT_PWD )
        .inputTopic   ( DEFAULT_MQTT_INTOPIC )
        .outputTopic  ( DEFAULT_MQTT_OUTOPIC )
        .restoreFromSD();
  myMqtt.saveToSD();
  myMqtt.setCallback( mqttCallback );
  DEBUG_print( ("MQTT: " + myMqtt.serializeJson() + "\n").c_str() );
#endif

  //NTP service (not used here):
#ifdef DEFAULT_NTPSOURCE
  myNTP.source        ( DEFAULT_NTPSOURCE )
       .zone          ( DEFAULT_TIMEZONE )
       .dayLight      ( DEFAULT_DAYLIGHT )
       .restoreFromSD ();
  myNTP.saveToSD      ();
  myNTP.begin         ();
  DEBUG_print( ("NTP: " + myNTP.serializeJson() + "\n").c_str() );
#endif
}

// **************************************** LOOP *************************************************
void loop() {
  ESPWebServer.handleClient(); delay(1L);             //WEB server
  myWiFi.loop();                                      //WiFi manager
  mySwitches.event(intr, rebound_completed);          //Switches management
  myPins.timers();                                    //Timers control for outputs
  myPins.serialEvent();                               //Serial communication for the serial slave management
  myMqtt.loop();                                      //MQTT manager
}
// ***********************************************************************************************
