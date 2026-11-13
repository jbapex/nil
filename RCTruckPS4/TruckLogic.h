#pragma once
#include <stdint.h>
#include <math.h>
#include "Config.h"

namespace truck {
inline float limit(float x,float a,float b) { return x<a?a:x>b?b:x; }
inline float approach(float x,float target,float step) {
  return x<target ? (x+step>target?target:x+step) : (x-step<target?target:x-step);
}
struct Input {
  float steering = 0, forward = 0, reverse = 0;
  bool engine = false, arm = false, emergency = false, horn = false;
  bool lights = false, hazard = false, left = false, right = false;
  bool volumeUp = false, volumeDown = false;
};
struct State {
  bool connected = false, fresh = false, armed = false, engine = false;
  bool horn = false, braking = false, headlights = false, hazard = false;
  bool left = false, right = false;
  float throttle = 0, steering = 0, rpmLoad = 0;
  uint8_t volume = cfg::initialVolume;
  uint32_t brakeEvent = 0;
};
class Logic {
 public:
  const State& state() const { return s_; }
  void disarm() { stop(); }
  void connect(uint32_t now) { resetLink(now); s_.connected=true; }
  void disconnect(uint32_t now) { resetLink(now); }
  void input(const Input& value,uint32_t now) {
    if (!s_.connected) return;
    // Once a timeout occurs, only a new explicit arm command can restore drive.
    if (!haveReport_ || uint32_t(now-lastReport_)>cfg::reportTimeoutMs) {
      stop(); previous_=value; haveNeutral_=false;
    }
    in_=value; lastReport_=now; haveReport_=true; s_.fresh=true;
    const bool neutral = value.forward < cfg::triggerDeadzone && value.reverse < cfg::triggerDeadzone;
    if (!neutral) haveNeutral_=false;
    else if (!haveNeutral_) { haveNeutral_=true; neutralSince_=now; }
    if (value.emergency) { stop(); previous_=value; return; }
    if (value.engine && !previous_.engine && fabsf(s_.throttle)<0.005f) {
      s_.engine=!s_.engine; engineSince_=now; s_.armed=false;
    }
    if (value.arm && !previous_.arm) {
      if (s_.armed) stop();
      else if (s_.engine && uint32_t(now-engineSince_)>=cfg::startupMs &&
        neutral && haveNeutral_ && uint32_t(now-neutralSince_)>=cfg::neutralBeforeArmMs) {
        s_.armed=true; direction_=0;
      }
    }
    if (value.lights && !previous_.lights) s_.headlights=!s_.headlights;
    if (value.hazard && !previous_.hazard) s_.hazard=!s_.hazard;
    if (value.left && !previous_.left) { s_.left=!s_.left; s_.right=false; }
    if (value.right && !previous_.right) { s_.right=!s_.right; s_.left=false; }
    if (value.volumeUp && !previous_.volumeUp) s_.volume=uint8_t(s_.volume>=95?100:s_.volume+5);
    if (value.volumeDown && !previous_.volumeDown) s_.volume=uint8_t(s_.volume<=5?0:s_.volume-5);
    previous_=value;
  }
  void tick(uint32_t now) {
    float dt=haveTick_ ? limit(uint32_t(now-lastTick_)*0.001f,0,0.05f) : 0;
    lastTick_=now; haveTick_=true;
    s_.fresh=s_.connected && haveReport_ && uint32_t(now-lastReport_)<=cfg::reportTimeoutMs;
    if (!s_.fresh || in_.emergency) {
      stop(); s_.horn=false; s_.steering=0; s_.rpmLoad=0;
      haveNeutral_=false; return;
    }
    s_.horn=in_.horn;
    float steer=limit(in_.steering,-1,1);
    s_.steering=fabsf(steer)<cfg::steeringDeadzone?0:
      copysignf((fabsf(steer)-cfg::steeringDeadzone)/(1-cfg::steeringDeadzone),steer);
    float f=pedal(in_.forward),r=pedal(in_.reverse);
    bool neutral=f==0 && r==0;
    s_.rpmLoad=s_.engine ? (f>r?f:r) : 0;
    if (!s_.armed || !s_.engine || uint32_t(now-engineSince_)<cfg::startupMs) {
      s_.throttle=0; s_.braking=false; direction_=0; return;
    }
    float request = f>0 && r>0 ? 0 : f-r;
    int sign=request>0?1:request<0?-1:0;
    bool opposite=sign && direction_ && sign!=direction_;
    s_.braking=(f>0 && r>0) || opposite;
    // Direction is released only after stopping and releasing BOTH triggers.
    if (neutral && fabsf(s_.throttle)<0.005f && haveNeutral_ &&
      uint32_t(now-neutralSince_)>=cfg::directionNeutralMs) direction_=0;
    if (sign && !direction_) direction_=sign;
    float target=opposite ? 0 : request*(request>=0?cfg::maxForward:cfg::maxReverse);
    float rate=fabsf(target)>fabsf(s_.throttle)?cfg::rampUpPerSecond:cfg::rampDownPerSecond;
    float before=s_.throttle;
    s_.throttle=approach(before,target,rate*dt);
    if (fabsf(before)>0.015f && fabsf(s_.throttle)<=0.015f) ++s_.brakeEvent;
  }
 private:
  State s_; Input in_,previous_;
  bool haveReport_=false,haveTick_=false,haveNeutral_=false;
  uint32_t lastReport_=0,lastTick_=0,neutralSince_=0,engineSince_=0;
  int direction_=0;
  static float pedal(float v) {
    return limit((v-cfg::triggerDeadzone)/(1-cfg::triggerDeadzone),0,1);
  }
  void stop() { s_.armed=false; s_.throttle=0; s_.braking=false; direction_=0; }
  void resetLink(uint32_t now) {
    uint8_t volume=s_.volume; uint32_t event=s_.brakeEvent;
    s_=State{}; s_.volume=volume; s_.brakeEvent=event;
    in_=Input{}; previous_=Input{}; haveReport_=false; haveNeutral_=false;
    lastReport_=lastTick_=now; haveTick_=true; direction_=0;
  }
};
}
