#pragma once
#include <stdint.h>
#include <assert.h>
#define OUTPUT 1
#define LOW 0
#define HIGH 1
static uint32_t duties[16]={};
static int attached[40]={},levels[40]={};
inline void pinMode(int pin,int) { assert(pin>=0 && pin<40); }
inline void digitalWrite(int pin,int level) { assert(pin>=0 && pin<40);levels[pin]=level; }
inline double ledcSetup(int,double hz,int) { return hz; }
inline void ledcAttachPin(int pin,int) { assert(pin>=0 && pin<40);++attached[pin]; }
inline void ledcWrite(int channel,uint32_t value) {
  duties[channel]=value;
  // Assertion runs at every hardware write, not just after the pair.
  assert(!(duties[2]>0 && duties[3]>0));
}
