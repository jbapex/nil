#pragma once
#include <stdint.h>

// 0 = bancada (som, sem saidas de movimento), 1 = ESC, 2 = BTS7960.
// O perfil padrao e bancada. Mude somente depois de conferir a fiacao.
#ifndef RC_DRIVER
#define RC_DRIVER 0
#endif
#if RC_DRIVER < 0 || RC_DRIVER > 2
#error "RC_DRIVER deve ser 0, 1 ou 2"
#endif

namespace cfg {
constexpr int driver = RC_DRIVER;
constexpr uint32_t sampleRate = 22050;
constexpr uint32_t reportTimeoutMs = 350;
constexpr uint32_t neutralBeforeArmMs = 500;
constexpr uint32_t directionNeutralMs = 300;
constexpr uint32_t startupMs = 1230;
constexpr float maxForward = 0.35f; // primeiro teste: limite de 35%
constexpr float maxReverse = 0.25f;
constexpr float rampUpPerSecond = 0.55f;
constexpr float rampDownPerSecond = 1.8f;
constexpr float triggerDeadzone = 0.06f;
constexpr float steeringDeadzone = 0.08f;
constexpr bool steeringReversed = false;
constexpr bool motorReversed = false;
constexpr uint8_t initialVolume = 35;
constexpr int steeringPin = 13;
constexpr int steeringLeftUs = 1100;
constexpr int steeringCenterUs = 1500;
constexpr int steeringRightUs = 1900;
constexpr int escPin = 33;
constexpr int escNeutralUs = 1500;
constexpr int escForwardUs = 2000;
constexpr int escReverseUs = 1000;
// BTS7960 / IBT-2: duas entradas PWM e dois enables separados.
constexpr int btsRPwmPin = 18;
constexpr int btsLPwmPin = 19;
constexpr int btsREnPin = 23;
constexpr int btsLEnPin = 22;
constexpr uint32_t btsFrequency = 18000;
// LEDs opcionais: -1 = desligado. Usar transistor/resistor apropriado.
constexpr int headlightPin = -1;
constexpr int brakeLightPin = -1;
constexpr int reverseLightPin = -1;
constexpr int leftIndicatorPin = -1;
constexpr int rightIndicatorPin = -1;
}

// Proibe saidas em GPIOs de entrada, flash e nos DACs reservados ao PAM.
constexpr bool validPin(int p) {
  return p == -1 || (p >= 0 && p <= 33 && p != 25 && p != 26 &&
    !(p >= 6 && p <= 11) && p != 20 && p != 24 && p != 28 &&
    p != 29 && p != 30 && p != 31);
}
constexpr bool uniquePins(const int* pins, unsigned n, unsigned i = 0, unsigned j = 1) {
  return i >= n ? true : j >= n ? uniquePins(pins,n,i+1,i+2) :
    ((pins[i] == -1 || pins[j] == -1 || pins[i] != pins[j]) && uniquePins(pins,n,i,j+1));
}
constexpr int usedPins[] = {
  cfg::driver ? cfg::steeringPin : -1,
  cfg::driver == 1 ? cfg::escPin : -1,
  cfg::driver == 2 ? cfg::btsRPwmPin : -1, cfg::driver == 2 ? cfg::btsLPwmPin : -1,
  cfg::driver == 2 ? cfg::btsREnPin : -1, cfg::driver == 2 ? cfg::btsLEnPin : -1,
  cfg::headlightPin, cfg::brakeLightPin, cfg::reverseLightPin,
  cfg::leftIndicatorPin, cfg::rightIndicatorPin
};
constexpr bool allPinsValid(const int* p,unsigned n) {
  return n == 0 || (validPin(p[n-1]) && allPinsValid(p,n-1));
}
static_assert(allPinsValid(usedPins,sizeof(usedPins)/sizeof(int)),"GPIO invalido ou reservado ao audio");
static_assert(uniquePins(usedPins,sizeof(usedPins)/sizeof(int)),"GPIOs duplicados");
static_assert(cfg::initialVolume <= 100,"Volume invalido");
static_assert(cfg::maxForward > 0 && cfg::maxForward <= 1 && cfg::maxReverse > 0 && cfg::maxReverse <= 1,"Limite do motor invalido");
static_assert(cfg::driver == 0 || cfg::steeringPin>=0,"Defina o GPIO do servo");
static_assert(cfg::driver != 1 || cfg::escPin>=0,"Defina o GPIO do ESC");
static_assert(cfg::driver != 2 || (cfg::btsRPwmPin>=0 && cfg::btsLPwmPin>=0 && cfg::btsREnPin>=0 && cfg::btsLEnPin>=0),"Defina os quatro GPIOs do BTS");
static_assert(cfg::steeringLeftUs>=900 && cfg::steeringLeftUs<cfg::steeringCenterUs && cfg::steeringCenterUs<cfg::steeringRightUs && cfg::steeringRightUs<=2100,"Limites do servo invalidos; use steeringReversed para inverter");
static_assert(cfg::escReverseUs>=900 && cfg::escReverseUs<cfg::escNeutralUs && cfg::escNeutralUs<cfg::escForwardUs && cfg::escForwardUs<=2100,"Limites ESC invalidos; use motorReversed para inverter");
