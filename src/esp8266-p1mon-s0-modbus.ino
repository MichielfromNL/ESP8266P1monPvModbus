/*
*   ESP8266-p1mon-S0-modbus
* 
*   Michiel Steltman 2022
*
*   ESP8266 mini D1 module as add-on for p1mon (https://www.ztatz.nl/)
*   p1mon is a stand-alone PI energy monitor. 
*   It can use PV watts from solaredge, or from S0 pules from a seperate KWH meter
*   https://www.ztatz.nl/kwh-meter-met-s0-meting/
*   I don;t have a S0 KWH meter, but a convertor that gives its data through a TCP/IP modbus interface, 
*   SO the idea is: read modbus, convert the data to S0 pulses. That can be done in Python on the PI>
*   But the problem is: p1mon is delivered aand installed as a single PI image.
*   It is possible to run extra code on the  PI, but that would require manual installs after each p1mon update (which is run automatically) 
*     
*   Hence this approach: I add a WEMOS D1 in the PI case that outputs to a PI GPIO. 
*   This piece of code reads the solar data (modbus) , and convetrs it into S0 pulses
*   It also rund a simple webserver to produce JSON requests for the solar data
*   
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
//#include <ESPTelnet.h>
#include <Simpletimer.h>
#include "Inverter.h"

const int S0out = D2; // Wemos D1 GPIO 4 (D2), S0 pulses output
const char * ssid = "***";
const char * password = "***";
const char * Hostname = "ESPp1monS0";

#define SAMPLEMODBUS  60    // update energy data every minute

//ESPTelnet telnet;
Simpletimer Pulsetimer;
Simpletimer Invertertimer;
int  pulseinterval = 0;
EnergyData Energy;
ESP8266WebServer webServer(80);
ADC_MODE(ADC_VCC);
//
//  Send error via email
//
void MailError() {
}

// ESP webserver serving JSON
//
void setupWebServer()
{
  Serial.println("[WebServer] Setup");
  webServer.on("/", handleRoot);
  webServer.on("/status", handleStatus);

  Serial.println("[WebServer] Starting..");
  webServer.begin();
  Serial.println("[WebServer] Running!");
}

// Return JSON with energy data:
// 
void handleRoot()
{
  String response;
  Serial.println("[WebServer] Request: /");
  if (Energy.Valid ) {
    response = "{\n \"BatteryPercentage\" : " + String(Energy.BatteryCharge) + "," ;
    response += "\n \"BatteryCharge\" : " + String(Energy.BatteryPower) + "," ;
    response += "\n \"PvOut\" : " + String(Energy.PvOut) + "," ;
    response += "\n \"Usage\" : " + String(Energy.Usage) + "," ;
    response += "\n \"Meter\" : " + String(Energy.Meter) + "\n}\n" ;
    webServer.send(200, "application/json", response);
  } else {
    webServer.send(204, "text/plain", "No data");
  }
}

//
// Return device status
//
void handleStatus()
{
  String Response;
  char time_str[15];
  
  Response =    "Heap Mem      :" + String(ESP.getFreeHeap() / 1024) + "kb";
  Response += "\nHeap Frag     :" + String(ESP.getHeapFragmentation()) + "%";
  Response += "\nFlash Mem     :" + String(ESP.getFlashChipRealSize() / 1024 / 1024) + "MB";
  Response += "\nWiFi Strength :" + String(WiFi.RSSI()) + "dB";
  Response += "\nChip ID       :" + String(ESP.getChipId());
  Response += "\nVCC           :" + String(ESP.getVcc() / 1024.0) + "V";
  Response += "\nCPU Freq.     :" + String(ESP.getCpuFreqMHz()) + "MHz";
  
  const uint32_t millis_in_day = 1000 * 60 * 60 * 24;
  const uint32_t millis_in_hour = 1000 * 60 * 60;
  const uint32_t millis_in_minute = 1000 * 60;
  uint8_t days = millis() / (millis_in_day);
  uint8_t hours = (millis() - (days * millis_in_day)) / millis_in_hour;
  uint8_t minutes = (millis() - (days * millis_in_day) - (hours * millis_in_hour)) / millis_in_minute;
  sprintf(time_str, "%2dd%2dh%2dm", days, hours, minutes);
  
  Response += "\nUptime        :" + String(time_str);
  Response += "\nLast Reset    :" + ESP.getResetInfo();
  Response += "\nPulse time    :" + String(pulseinterval);
  
  webServer.send(200, "text/plain", Response);
}
// read inverter data
// try three times
//
void UpdateInverterData() {
  int errcnt=3;
  bool ok = false;
  while (!ok && errcnt--) {
    Serial.println (F("Getting energy data"));
    ok = EnergyUpdate(&Energy);
  }
  if (! ok) {
    BlinkError(10 * 1000);
    MailError();
    Serial.println("Reset..");
    ESP.restart();
  } else
      
  // convert watts to S0 pulse duration
  // common factor = 2000 pulses / Kwh , 1 pulse is 0.0005 kWh , 
  // so , if e.g. PvOut = 500 Watts, is 0,5 Kwh / hr = 1000 pulses / 3600 sec = 1 pulse each 3,6 secs or 3600 msec
  // pulse interval ms = 3600 * 1000 / ( watts  * 2)
  pulseinterval =  Energy.PvOut > 0 ? 3600 * 1000 / ((int)Energy.PvOut * 2) : 0;
  Serial.printf ("Done , PvOut = %d , pulsetime = %d\n",Energy.PvOut, pulseinterval);
}

// Flash an error
//
void BlinkError(int duration) {

  while (duration > 0) {
    for (int i=0; i<3; i++) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(50);
      digitalWrite(LED_BUILTIN, HIGH);
      delay (200);
    }
    delay(500);
    duration -= 1250; // 3 x 250 + 500
  }
}

//
// Send an S0 pulse (LOw = pulse)
//
void Pulse() {
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(S0out, LOW);
  delay(50);
  digitalWrite(S0out, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
}


void setup() {
  Serial.begin(115200);
  while (!Serial) delay(50);

  pinMode(S0out, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(S0out, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
  
  Pulsetimer.register_callback(Pulse);
  Invertertimer.register_callback(UpdateInverterData);

  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
    // keep connection alive
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  if (MDNS.begin(Hostname)) {
    Serial.println("mDNS started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(Hostname);

  // set code to execute on OTA connect
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
    //telnet.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    //telnet.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char s[128];
    sprintf(s,"Progress: %u%%\r", (progress / (total / 100)));
    Serial.println(String(s));
    //telnet.println(String(s));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    const char *s;
    char errstr[128];
    if (error == OTA_AUTH_ERROR) s = "Auth Failed";
    else if (error == OTA_BEGIN_ERROR) s = "Begin Failed";
    else if (error == OTA_CONNECT_ERROR) s = "Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) s = "Receive Failed";
    else if (error == OTA_END_ERROR) s = "End Failed";
    sprintf(errstr,"Error[%u]: %s ", error , s);
    
    Serial.println(String(s));
    //telnet.println(String(s));
  });

  Serial.print(Hostname);
  Serial.println(" Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  //setupTelnet();
  setupWebServer();
  ArduinoOTA.begin();

  // now, init, dont stop until fine.
  while (!EnergyInit()) {
    Serial.println(F("No inverter found"));
    // what to do ? beep ?? send mail?
    BlinkError(60 * 1000);
  }
  Serial.println(F("Found inverter"));
  UpdateInverterData();
  Serial.println(F("Setup complete"));
}


// 
int cnt=0;
void loop() {
  // put your main code here, to run repeatedly:
  // JSon web server handle
  ArduinoOTA.handle();
  webServer.handleClient();
  //telnet.loop();
  if (pulseinterval > 0) Pulsetimer.run(pulseinterval);
  Invertertimer.run(SAMPLEMODBUS * 1000L);
}
