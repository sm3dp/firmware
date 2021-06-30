/*
  temperature.h - temperature controller
  Part of Marlin

  Copyright (c) 2011 Erik van der Zalm

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef temperature_h
#define temperature_h 
#include "sys.h"
#include "sm_firmware.h"

// public functions
void tp_init(void);  //initialise the heating
void manage_heater(void); //it is critical that this is called periodically.

// low level conversion routines
// do not use these routines and variables outside of temperature.cpp
extern volatile int target_temperature[EXTRUDERS];  
extern volatile float current_temperature[EXTRUDERS];
extern volatile int target_temperature_bed;
extern volatile float current_temperature_bed;

#ifdef PIDTEMP
  extern float Kp,Ki,Kd,Kc;
  float scalePID_i(float i);
  float scalePID_d(float d);
  float unscalePID_i(float i);
  float unscalePID_d(float d);

#endif
#ifdef PIDTEMPBED
  extern float bedKp,bedKi,bedKd;
#endif
  
//high level conversion routines, for use outside of temperature.cpp
//inline so that there is no performance decrease.
//deg=degreeCelsius
float degHotend(u8 extruder); 
float degBed(void); 
float degTargetHotend(u8 extruder); 
float degTargetBed(void); 
void setTargetHotend(const float celsius, uint8_t extruder); 
void setTargetBed(const float celsius); 
bool isHeatingHotend(u8 extruder);
bool isHeatingBed(void); 
bool isCoolingHotend(u8 extruder); 
bool isCoolingBed(void); 

int getHeaterPower(int heater);
void disable_heater(void);
void setWatch(void);
void updatePID(void);

//void test(char temp);

#define degHotend0() degHotend(0)
#define degTargetHotend0() degTargetHotend(0)
#define setTargetHotend0(_celsius) setTargetHotend((_celsius), 0)
#define isHeatingHotend0() isHeatingHotend(0)
#define isCoolingHotend0() isCoolingHotend(0)
#if EXTRUDERS > 1
#define degHotend1() degHotend(1)
#define degTargetHotend1() degTargetHotend(1)
#define setTargetHotend1(_celsius) setTargetHotend((_celsius), 1)
#define isHeatingHotend1() isHeatingHotend(1)
#define isCoolingHotend1() isCoolingHotend(1)
#else
#define setTargetHotend1(_celsius) do{}while(0)
#endif
#if EXTRUDERS > 2
#define degHotend2() degHotend(2)
#define degTargetHotend2() degTargetHotend(2)
#define setTargetHotend2(_celsius) setTargetHotend((_celsius), 2)
#define isHeatingHotend2() isHeatingHotend(2)
#define isCoolingHotend2() isCoolingHotend(2)
#else
#define setTargetHotend2(_celsius) do{}while(0)
#endif

void PID_autotune(float temp, int extruder, int ncycles);
void autotempShutdown(void);

#endif

