/**The MIT License (MIT)

*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "Inverter.h"

WiFiClient client;
IPAddress InverterIP;
const char * Irequest = PVBREQUEST;
  
// find the IP address of the inverter
//
bool EnergyInit() {

  bool found = false;
  bool c;
  Serial.println(F("Initializing energy datacollection"));
  InverterIP.fromString(INVERTERIP);
  
  client.setTimeout(1000);
  if  (client.connect(InverterIP, INVERTERPORT)) {
      Serial.print(F("Found inverter at IP "));
      Serial.println(InverterIP);
      client.stop();
      found = true;
  } else {
    Serial.print(F("Inverter not found at at "));
    Serial.print(InverterIP);
    Serial.println(F("Searching"));
    client.setTimeout(125);
    InverterIP = WiFi.localIP();
    int ad = 10;
    while (! found & ad++ < 245) {
      InverterIP[3] = ad;
      if (client.connect(InverterIP, INVERTERPORT)) {
        Serial.print(F("Found inverter at IP "));
        Serial.println(InverterIP);
        client.stop();
        found = true;
      } else {
        Serial.print(F("Tried "));
        Serial.println(InverterIP);
      }
    }
  }
  client.setTimeout(5000);
  return found;
}

//
// Get databytes from buffer
int RegVal(String DataBuf, int regnum) {
   String rvs;
   char cpregval[6]; 
   
   rvs = DataBuf.substring((regnum-1)*4,((regnum-1)*4)+4);
   rvs.toCharArray(cpregval,5); // cpregval[4] = '\0';
   //Serial.printf("Reading register %d , substring was %s \n", regnum, cpregval);
   return strtol(cpregval,NULL,16);
}


// Now fetch data from solar and battery unit
//  Is a MODBUS string
// +ok=5504400967002E04510F7C00000004000013880202FF26FBA700A400000FA0000000140F7B0000001BFF93FF930BD90BDF0289000000000952000200361387000000001C69
// Respons = +ok= (4) 5504 (4) len (2) buf (128) CRC (4) = we must read 142 valid bytes
//
bool EnergyUpdate(EnergyData *energy) {
  
  String mbdata;
  String DataBuffer;
  bool valid = false;
  uint16_t cnt = 0;

  energy->Valid = false;

  Serial.print (F("[MODBUS] Requesting data from "));
  Serial.print (InverterIP);
  Serial.printf(":%d %s \n", INVERTERPORT,Irequest);
  
  if(client.connect(InverterIP, INVERTERPORT)) {
    client.print(Irequest);
    while (client.connected() && ( mbdata.length() < PVBRESPLEN)) {
      if (client.available()) {
          char c = client.read();
          if  ( isAscii(c) ) {
            // Skips leading garbage
            if (c == '+') { valid = true; }
            if (valid == true) { mbdata += c; }
          }
      } else {
        delay(100);
        yield();
        if (cnt++ > 100 ) {
            Serial.println (F("[MODBUS] response timeout"));
            return false;
        }
      }
    }
  } else {
    Serial.print (F("[MODBUS] Can't connect to "));
    Serial.print (InverterIP);
    Serial.println (":" + String(INVERTERPORT));
    return false;
  }
  // give WiFi and TCP/IP libraries a chance to handle pending events
  yield();
  client.stop();
  
  // now set the databuffer pick the relevant hex ASCII registers & decode, we need strtol because it is hex
  DataBuffer = mbdata.substring(10, 426);
  //Serial.println("Data buffer = " + DataBuffer);
  energy->Valid = true;
  energy->BatteryPower = RegVal(DataBuffer,R_BATTPOWER);
  energy->BatteryCharge = RegVal(DataBuffer,R_BATTCHARGE);
  energy->PvOut = RegVal(DataBuffer,R_PVOUT);
  energy->Meter = RegVal(DataBuffer,R_METER);
  // now calculate internal use from all 3: meter, PV and BAattery charge. Charge is +, discharge = - 
  energy->Usage = energy->Meter + energy->PvOut - energy->BatteryPower;
     
  Serial.printf ("[MODBUS] Data: Meter=%d, Pv=%d, BatPwr=%d, Usage=%d, Bat%%=%d\n" , 
      energy->Meter,
      energy->PvOut,     
      energy->BatteryPower,
      energy->Usage,
      energy->BatteryCharge
  );
  
  //Serial.println("Energy update done");
  return energy->Valid;
}
