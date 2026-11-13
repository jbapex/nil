#pragma once
#include <Arduino.h>
#include "TruckLogic.h"

namespace hardware {
constexpr uint8_t steeringChannel=0,escChannel=1,rPwmChannel=2,lPwmChannel=3;
inline void optionalPin(int pin) { if(pin>=0) { pinMode(pin,OUTPUT); digitalWrite(pin,LOW); } }
inline void optionalWrite(int pin,bool state) { if(pin>=0) digitalWrite(pin,state?HIGH:LOW); }
inline uint32_t servoDuty(int us) { return uint32_t(us)*65535UL/20000UL; }
inline void servo(uint8_t channel,int us) { ledcWrite(channel,servoDuty(us)); }
inline void neutral() {
#if RC_DRIVER == 1
  servo(escChannel,cfg::escNeutralUs);
#elif RC_DRIVER == 2
  ledcWrite(rPwmChannel,0); ledcWrite(lPwmChannel,0);
  digitalWrite(cfg::btsREnPin,LOW); digitalWrite(cfg::btsLEnPin,LOW);
#endif
}
inline bool begin() {
  optionalPin(cfg::headlightPin); optionalPin(cfg::brakeLightPin);
  optionalPin(cfg::reverseLightPin); optionalPin(cfg::leftIndicatorPin); optionalPin(cfg::rightIndicatorPin);
#if RC_DRIVER != 0
  if (!ledcSetup(steeringChannel,50,16)) return false;
  // Set duty before attaching: first pulse is centered, never maximum travel.
  servo(steeringChannel,cfg::steeringCenterUs);
  ledcAttachPin(cfg::steeringPin,steeringChannel);
#endif
#if RC_DRIVER == 1
  if (!ledcSetup(escChannel,50,16)) return false;
  servo(escChannel,cfg::escNeutralUs); ledcAttachPin(cfg::escPin,escChannel);
#elif RC_DRIVER == 2
  optionalPin(cfg::btsREnPin); optionalPin(cfg::btsLEnPin);
  if (!ledcSetup(rPwmChannel,cfg::btsFrequency,10) || !ledcSetup(lPwmChannel,cfg::btsFrequency,10)) return false;
  ledcWrite(rPwmChannel,0); ledcWrite(lPwmChannel,0);
  ledcAttachPin(cfg::btsRPwmPin,rPwmChannel); ledcAttachPin(cfg::btsLPwmPin,lPwmChannel);
#endif
  neutral(); return true;
}
inline void write(const truck::State& state,bool healthy,uint32_t now) {
  bool allow=state.armed && state.fresh && healthy;
  float value=allow?state.throttle:0;
  if (cfg::motorReversed) value=-value;
#if RC_DRIVER != 0
  float steer=healthy && state.fresh ? state.steering : 0;
  if (cfg::steeringReversed) steer=-steer;
  int us=cfg::steeringCenterUs+int(steer*(steer>=0?
    cfg::steeringRightUs-cfg::steeringCenterUs:cfg::steeringCenterUs-cfg::steeringLeftUs));
  servo(steeringChannel,us);
#endif
#if RC_DRIVER == 1
  int pulse=cfg::escNeutralUs+int(value*(value>=0?
    cfg::escForwardUs-cfg::escNeutralUs:cfg::escNeutralUs-cfg::escReverseUs));
  servo(escChannel,pulse);
#elif RC_DRIVER == 2
  uint32_t duty=uint32_t(fabsf(value)*1023);
  // Clear inactive direction first; never drive both PWM inputs together.
  if (value>=0) { ledcWrite(lPwmChannel,0); ledcWrite(rPwmChannel,duty); }
  else { ledcWrite(rPwmChannel,0); ledcWrite(lPwmChannel,duty); }
  digitalWrite(cfg::btsREnPin,duty?HIGH:LOW); digitalWrite(cfg::btsLEnPin,duty?HIGH:LOW);
#endif
  bool blink=(now/400)%2==0;
  optionalWrite(cfg::headlightPin,state.headlights);
  optionalWrite(cfg::brakeLightPin,state.braking || !allow);
  optionalWrite(cfg::reverseLightPin,value<0);
  optionalWrite(cfg::leftIndicatorPin,blink&&(state.left||state.hazard));
  optionalWrite(cfg::rightIndicatorPin,blink&&(state.right||state.hazard));
}
}
