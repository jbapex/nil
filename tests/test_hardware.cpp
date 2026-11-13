#include <assert.h>
#include <stdio.h>
#include "Hardware.h"
int main() {
  assert(hardware::begin());
  truck::State s;s.connected=s.fresh=s.armed=true;s.throttle=cfg::maxForward;s.steering=1;
  hardware::write(s,true,0);
#if RC_DRIVER == 0
  for(int n:attached)assert(!n);
#elif RC_DRIVER == 1
  assert(duties[hardware::escChannel]>hardware::servoDuty(cfg::escNeutralUs));
  hardware::write(s,false,0);
  assert(duties[hardware::escChannel]==hardware::servoDuty(cfg::escNeutralUs));
  assert(duties[hardware::steeringChannel]==hardware::servoDuty(cfg::steeringCenterUs));
  hardware::write(s,true,0);s.armed=false;hardware::write(s,true,0);
  assert(duties[hardware::escChannel]==hardware::servoDuty(cfg::escNeutralUs));
#elif RC_DRIVER == 2
  assert(duties[hardware::rPwmChannel]>0 && duties[hardware::lPwmChannel]==0);
  s.throttle=-cfg::maxReverse;hardware::write(s,true,0);
  assert(duties[hardware::rPwmChannel]==0 && duties[hardware::lPwmChannel]>0);
  hardware::write(s,false,0);
  assert(!duties[hardware::rPwmChannel] && !duties[hardware::lPwmChannel]);
  assert(levels[cfg::btsREnPin]==LOW && levels[cfg::btsLEnPin]==LOW);
  hardware::write(s,true,0);s.fresh=false;hardware::write(s,true,0);
  assert(!duties[hardware::rPwmChannel] && !duties[hardware::lPwmChannel]);
#endif
  printf("Saidas perfil %d: bancada/neutro/inibicao e exclusao de sentidos OK\n",RC_DRIVER);
}
