/**

read inverter 

*/
#pragma once

typedef struct EnergyData {
  bool    Valid ;
  int16_t BatteryPower;
  int16_t BatteryCharge;
  int16_t PvOut;
  int16_t Usage;
  int16_t Meter;
} EnergyData;



#define INVERTERIP    "192.168.178.82"
#define INVERTERPORT  8899
#define PVBREQUEST    "AT+INVDATA=8,550400000068FC30"  // Cmd : read 104 registers at 0. returns a string of 430 bytes (4 + 2 + 2 + 2 + 416 + 4)
#define PVBRESPLEN    430

#define R_USAGE 3       // 30003
#define R_BATTPOWER 11  // 30011
#define R_PVOUT 12      // 30012
#define R_BATTCHARGE 19 // 30019
#define R_METER  94     // 30094

bool EnergyUpdate(EnergyData *energy);
bool EnergyInit();
