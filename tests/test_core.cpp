#include <assert.h>
#include <stdio.h>
#include <limits>
#include "TruckLogic.h"
#include "AudioMixer.h"

using truck::Input;
struct Rig {
  truck::Logic logic; uint32_t now=100; Input in;
  Rig() { logic.connect(now); update(); }
  void update(uint32_t ms=10) { now+=ms; logic.input(in,now); logic.tick(now); }
  void wait(uint32_t ms) { for(uint32_t t=0;t<ms;t+=10) update(); }
  void engine() { in.engine=true;update();in.engine=false;wait(1500); }
  void arm() { in.arm=true;update();in.arm=false;update(); }
};
void logicTests() {
  Rig r;
  r.in.forward=1; r.wait(1000); assert(r.logic.state().throttle==0);
  r.in.forward=0;r.engine();r.arm();assert(r.logic.state().armed);
  r.in.forward=1;r.wait(1000);assert(r.logic.state().throttle>0);
  assert(r.logic.state().throttle<=cfg::maxForward);
  r.in.forward=0;r.in.reverse=1;r.wait(1500);
  assert(r.logic.state().throttle==0); // brake, must not reverse yet
  r.in.reverse=0;r.wait(400);r.in.reverse=1;r.wait(800);
  assert(r.logic.state().throttle<0 && r.logic.state().throttle>=-cfg::maxReverse);
  r.now+=cfg::reportTimeoutMs+1;r.logic.tick(r.now);
  assert(!r.logic.state().armed && r.logic.state().throttle==0);
  r.update();r.wait(700);assert(!r.logic.state().armed);
  r.in.reverse=0;r.wait(600);r.arm();r.in.forward=1;r.wait(700);
  assert(r.logic.state().throttle>0);
  r.in.emergency=true;r.update();assert(!r.logic.state().armed && r.logic.state().throttle==0);
  r.in.emergency=false;r.in.forward=0;r.wait(600);r.arm();
  r.logic.disconnect(r.now);assert(!r.logic.state().engine && r.logic.state().throttle==0);
  r.logic.connect(r.now);r.update();r.wait(500);assert(!r.logic.state().armed);
  // Arming cannot happen with a trigger pressed, including holding Options.
  Rig a;a.engine();a.in.forward=1;a.arm();assert(!a.logic.state().armed);
  a.in.arm=true;a.in.forward=0;a.wait(1000);assert(!a.logic.state().armed);
  a.in.arm=false;a.update();a.arm();assert(a.logic.state().armed);
  // Startup must finish before drive arming.
  Rig b;b.wait(600);b.in.engine=true;b.update();b.in.engine=false;b.update();b.arm();
  assert(!b.logic.state().armed);b.wait(1500);b.arm();assert(b.logic.state().armed);
  // Button holds toggle once, both triggers command braking.
  Rig c;c.in.engine=true;c.wait(1500);assert(c.logic.state().engine);
  c.in.engine=false;c.update();c.arm();c.in.forward=c.in.reverse=1;c.wait(500);
  assert(c.logic.state().throttle==0 && c.logic.state().braking);
  // Volume is bounded and horn is cleared on stale input.
  for(int i=0;i<30;i++) { c.in.volumeUp=true;c.update();c.in.volumeUp=false;c.update(); }
  assert(c.logic.state().volume==100);
  c.in.horn=true;c.update();assert(c.logic.state().horn);
  c.now+=cfg::reportTimeoutMs+1;c.logic.tick(c.now);assert(!c.logic.state().horn);
  // Freshness uses unsigned arithmetic across millis() rollover.
  truck::Logic wrap;wrap.connect(UINT32_MAX-100);wrap.input(Input{},UINT32_MAX-90);
  wrap.tick(20);assert(wrap.state().fresh);wrap.tick(500);assert(!wrap.state().fresh);
  puts("Logic: armamento, partida, rampas, re, freio, timeout, reconexao, emergencia, volume e rollover OK");
}
void audioTests() {
  const int8_t pcm[]={0,40,90,40,0,-40,-90,-40};
  const audio::Sample sample={pcm,8,cfg::sampleRate,2,6};
  const audio::Bank bank={sample,sample,sample,sample,sample};
  audio::Mixer silent(bank);
  for(int i=0;i<1000;i++) assert(silent.next()==0);
  audio::Mixer engine(bank),mixed(bank);
  audio::Command c;c.engine=true;c.volume=100;
  engine.command(c);c.horn=true;mixed.command(c);
  int different=0,nonzero=0;
  for(uint32_t i=0;i<cfg::sampleRate*3;i++) {
    int a=engine.next(),b=mixed.next();
    if(a!=b)++different;
    if(a)++nonzero;
  }
  assert(different>1000 && nonzero>1000); // horn+engine concurrently, never replaces engine
  c.engine=c.horn=false;c.brakeEvent=1;mixed.command(c);
  for(uint32_t i=0;i<cfg::sampleRate;i++) mixed.next();
  for(int i=0;i<1000;i++) assert(mixed.next()==0);
  c.test=true;mixed.command(c);int energy=0;
  for(int i=0;i<1000;i++) energy+=abs(mixed.next());
  assert(energy>100000);
  c.volume=0;mixed.command(c);for(uint32_t i=0;i<cfg::sampleRate;i++) mixed.next();
  for(int i=0;i<1000;i++) assert(mixed.next()==0);
  audio::Voice v;v.play(sample);for(int i=0;i<100;i++)v.next(true);assert(v.active());
  for(int i=0;i<20;i++)v.next(false);
  assert(!v.active());
  // Stress high pitch + simultaneous effects, including out-of-range requested load.
  c.engine=c.horn=c.reverse=c.test=true;c.load=20;c.volume=100;
  mixed.command(c);for(uint32_t i=0;i<cfg::sampleRate*20;i++)mixed.next();
  puts("Audio: silencio, partida/loop, mix motor+buzina, termino, volume, tom e stress OK");
}
int main() { logicTests();audioTests(); }
