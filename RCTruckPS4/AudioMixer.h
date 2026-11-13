#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include "TruckLogic.h"

namespace audio {
struct Sample {
  const int8_t* pcm; uint32_t count,rate,loopBegin,loopEnd;
};
struct Bank { Sample start,idle,rev,horn,brake; };
struct Command {
  bool engine=false,horn=false,reverse=false,test=false;
  float load=0;
  uint8_t volume=cfg::initialVolume;
  uint32_t brakeEvent=0;
};
class Voice {
 public:
  void play(const Sample& sample) { sample_=&sample; phase_=0; active_=sample.count>0; }
  void stop() { active_=false; }
  bool active() const { return active_; }
  float next(bool held=false) {
    if (!active_) return 0;
    const Sample& s=*sample_;
    float out=at(s,phase_,false);
    phase_+=float(s.rate)/cfg::sampleRate;
    if (held && s.loopEnd>s.loopBegin && phase_>=s.loopEnd)
      phase_=s.loopBegin+fmodf(phase_-s.loopBegin,float(s.loopEnd-s.loopBegin));
    if (phase_>=s.count) active_=false;
    return out;
  }
  static float at(const Sample& s,float phase,bool loop) {
    if (!s.count) return 0;
    uint32_t i=uint32_t(phase);
    if (i>=s.count) i=loop?i%s.count:s.count-1;
    uint32_t j=i+1<s.count?i+1:(loop?0:i);
    float f=phase-floorf(phase);
    return (float(s.pcm[i])+(float(s.pcm[j])-s.pcm[i])*f)*256.0f;
  }
 private:
  const Sample* sample_=nullptr; float phase_=0; bool active_=false;
};
class Mixer {
 public:
  explicit Mixer(const Bank& bank):bank_(bank) {}
  // Used only by the audio task. No Bluetooth/API calls or heap allocations.
  void command(const Command& c) {
    if (c.engine && !command_.engine) { start_.play(bank_.start); cycle_=0; }
    if (!c.engine) start_.stop();
    if (c.horn && !command_.horn) horn_.play(bank_.horn);
    if (c.brakeEvent!=command_.brakeEvent) brake_.play(bank_.brake);
    command_=c;
  }
  int16_t next() {
    const float dt=1.0f/cfg::sampleRate;
    rpm_=truck::approach(rpm_,truck::limit(command_.load,0,1),dt*(command_.load>rpm_?1.3f:0.7f));
    gain_=truck::approach(gain_,command_.engine?1.0f:0.0f,dt*20);
    volume_=truck::approach(volume_,command_.volume/100.0f,dt*4);
    float motor=0;
    if (start_.active()) motor=start_.next();
    else if (gain_>0) {
      float blend=truck::limit(rpm_*1.4f,0,1);
      motor=Voice::at(bank_.idle,cycle_*bank_.idle.count,true)*(1-blend)
        +Voice::at(bank_.rev,cycle_*bank_.rev.count,true)*blend;
      cycle_+=(1+2.5f*rpm_)*float(bank_.idle.rate)/(cfg::sampleRate*bank_.idle.count);
      cycle_-=floorf(cycle_);
    }
    float mix=motor*gain_*0.46f + horn_.next(command_.horn)*0.32f + brake_.next()*0.25f;
    // Synthesized reversing beep: zero DC during silence.
    reverseGain_=truck::approach(reverseGain_,command_.reverse?1.0f:0.0f,dt*50);
    if (reverseGain_>0 && time_%cfg::sampleRate < cfg::sampleRate*0.4f)
      mix+=sinf(float(time_%cfg::sampleRate)*6.283185307f*1000/cfg::sampleRate)*5500*reverseGain_;
    testGain_=truck::approach(testGain_,command_.test?1.0f:0.0f,dt*50);
    if (testGain_>0)
      mix+=sinf(float(time_%cfg::sampleRate)*6.283185307f*440/cfg::sampleRate)*7000*testGain_;
    ++time_;
    return int16_t(truck::limit(mix*volume_,-32768,32767));
  }
 private:
  Bank bank_; Command command_; Voice start_,horn_,brake_;
  float cycle_=0,rpm_=0,gain_=0,volume_=0,reverseGain_=0,testGain_=0;
  uint32_t time_=0;
};
}
