// RC Truck PS4 v0.1 - firmware novo para ESP32 classico + PAM analogico.
// Bluepad32 4.1.0 / Arduino ESP32 2.0.17. Ver README e THIRD_PARTY.
#include <Arduino.h>
#include <Bluepad32.h>
#include <driver/i2s.h>
#include <atomic>
#include "Config.h"
#include "TruckLogic.h"
#include "AudioMixer.h"
#include "SoundBank.h"
#include "Hardware.h"

#if !defined(CONFIG_IDF_TARGET_ESP32)
#error "Use ESP32 classico (WROOM/WROVER), nao C3/S3: DAC 25/26 e Bluetooth Classic sao necessarios."
#endif

struct AudioMessage { audio::Command command; uint32_t timestamp; };
struct MotorMessage { truck::State state; uint32_t timestamp; };
static QueueHandle_t audioQueue=nullptr,motorQueue=nullptr;
static std::atomic<uint32_t> audioHeartbeat{0},audioBlocks{0},audioErrors{0};
static std::atomic<bool> audioReady{false},motorReady{false};
static truck::Logic vehicle;
static ControllerPtr controller=nullptr;
static uint32_t toneStart=0,demoStart=0;
static bool toneTest=false,demo=false,rawDebug=false;

static bool soundHealthy(uint32_t now) {
  return audioReady.load() && uint32_t(now-audioHeartbeat.load())<150;
}

static void audioTask(void*) {
  i2s_config_t config={};
  config.mode=i2s_mode_t(I2S_MODE_MASTER|I2S_MODE_TX|I2S_MODE_DAC_BUILT_IN);
  config.sample_rate=cfg::sampleRate;
  config.bits_per_sample=I2S_BITS_PER_SAMPLE_16BIT;
  config.channel_format=I2S_CHANNEL_FMT_RIGHT_LEFT;
  config.communication_format=I2S_COMM_FORMAT_STAND_MSB;
  config.intr_alloc_flags=ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count=6; config.dma_buf_len=128;
  config.use_apll=false; config.tx_desc_auto_clear=true;
  esp_err_t error=i2s_driver_install(I2S_NUM_0,&config,0,nullptr);
  if(error==ESP_OK) error=i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
  if(error!=ESP_OK) {
    audioErrors.fetch_add(1);
    Serial.printf("ERRO I2S: %s. Movimento bloqueado.\n",esp_err_to_name(error));
    vTaskDelete(nullptr); return;
  }
  audio::Mixer mixer(sounds::bank);
  AudioMessage message={}; message.timestamp=millis();
  uint16_t buffer[128*2];
  audioReady.store(true);
  for(;;) {
    xQueueReceive(audioQueue,&message,0);
    audio::Command c=message.command;
    if(uint32_t(millis()-message.timestamp)>cfg::reportTimeoutMs) {
      c.engine=c.horn=c.reverse=c.test=false; c.load=0;
    }
    mixer.command(c);
    for(size_t i=0;i<128;i++) {
      // DAC interno usa os 8 bits superiores. Mesmo mix nos dois pinos:
      // o PAM recebe motor e efeitos tanto em GPIO25 como em GPIO26.
      uint16_t value=uint16_t((int32_t(mixer.next())+32768)>>8)<<8;
      buffer[i*2]=value; buffer[i*2+1]=value;
    }
    size_t written=0;
    error=i2s_write(I2S_NUM_0,buffer,sizeof(buffer),&written,pdMS_TO_TICKS(100));
    if(error!=ESP_OK || written!=sizeof(buffer)) {
      audioErrors.fetch_add(1); audioReady.store(false); vTaskDelay(pdMS_TO_TICKS(5));
    } else {
      audioBlocks.fetch_add(1); audioHeartbeat.store(millis()); audioReady.store(true);
    }
  }
}

static void motorTask(void*) {
  bool ready=hardware::begin(); motorReady.store(ready);
  MotorMessage message={};
  TickType_t wake=xTaskGetTickCount();
  for(;;) {
    xQueueReceive(motorQueue,&message,0);
    uint32_t now=millis();
    bool healthy=ready && soundHealthy(now) && uint32_t(now-message.timestamp)<=cfg::reportTimeoutMs;
    if(ready) hardware::write(message.state,healthy,now);
    vTaskDelayUntil(&wake,pdMS_TO_TICKS(5));
  }
}

static void connected(ControllerPtr ctl) {
  if(!ctl->isGamepad() || controller) { ctl->disconnect(); return; }
  controller=ctl; vehicle.connect(millis());
  ControllerProperties p=ctl->getProperties();
  Serial.printf("Controle: %s | VID:PID %04x:%04x\n",ctl->getModelName().c_str(),p.vendor_id,p.product_id);
  Serial.println("Triangulo: partida. Depois de 0,5 s com gatilhos soltos, OPTIONS: armar.");
}
static void disconnected(ControllerPtr ctl) {
  if(controller!=ctl) return;
  controller=nullptr; vehicle.disconnect(millis());
  Serial.println("Controle desconectado: movimento em neutro, novo armamento necessario.");
}
static truck::Input readController() {
  truck::Input in;
  in.steering=truck::limit(controller->axisX()/512.0f,-1,1);
  in.forward=truck::limit(controller->throttle()/1023.0f,0,1);
  in.reverse=truck::limit(controller->brake()/1023.0f,0,1);
  in.engine=controller->y(); in.arm=controller->miscStart();
  in.emergency=controller->b(); in.horn=controller->a();
  in.lights=controller->l1(); in.hazard=controller->r1();
  in.left=controller->dpad()&DPAD_LEFT; in.right=controller->dpad()&DPAD_RIGHT;
  in.volumeUp=controller->dpad()&DPAD_UP; in.volumeDown=controller->dpad()&DPAD_DOWN;
  return in;
}
static void printStatus() {
  const auto& s=vehicle.state();
  Serial.printf("perfil=%d controle=%d recente=%d motorSonoro=%d armado=%d tracao=%.2f volume=%u audio=%d blocos=%u erros=%u heap=%u\n",
    cfg::driver,s.connected,s.fresh,s.engine,s.armed,s.throttle,s.volume,
    soundHealthy(millis()),audioBlocks.load(),audioErrors.load(),ESP.getFreeHeap());
}
static void serialCommands(uint32_t now) {
  while(Serial.available()) {
    char c=Serial.read();
    if(c=='?' || c=='s') printStatus();
    if(c=='p') { rawDebug=!rawDebug; Serial.println(rawDebug?"Dados brutos ligados":"Dados brutos desligados"); }
    if(c=='x') { toneTest=demo=false; vehicle.disarm(); Serial.println("Desarmado; testes de audio encerrados."); }
    if(c=='t' && !vehicle.state().armed) { toneTest=true; toneStart=now; demo=false; }
    if(c=='d' && !vehicle.state().armed) { demo=true; demoStart=now; toneTest=false; }
    if(c=='f' && !vehicle.state().armed) { BP32.forgetBluetoothKeys(); Serial.println("Chaves removidas. Pareie novamente com SHARE + PS."); }
  }
  if(toneTest && uint32_t(now-toneStart)>=2000) toneTest=false;
  if(demo && uint32_t(now-demoStart)>=12000) demo=false;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nRC Truck PS4 v0.1 | ESP32 classico | PAM em 25/26");
  Serial.printf("Perfil: %d (0=bancada, 1=ESC, 2=BTS7960)\n",cfg::driver);
  audioQueue=xQueueCreate(1,sizeof(AudioMessage)); motorQueue=xQueueCreate(1,sizeof(MotorMessage));
  if(!audioQueue || !motorQueue) { Serial.println("ERRO: sem memoria para filas."); for(;;) delay(1000); }
  // Nao apaga pareamentos em cada boot. 'f' no monitor serial faz o reset.
  BP32.setup(&connected,&disconnected);
  BP32.enableVirtualDevice(false);
  BP32.enableBLEService(false);
  BP32.enableNewBluetoothConnections(true);
  if(xTaskCreatePinnedToCore(audioTask,"audio-dma",6144,nullptr,4,nullptr,1)!=pdPASS ||
     xTaskCreatePinnedToCore(motorTask,"motor-output",3072,nullptr,3,nullptr,1)!=pdPASS) {
    Serial.println("ERRO: nao foi possivel iniciar tarefas. Reinicie.");
    for(;;) delay(1000);
  }
  Serial.println("SHARE + PS para parear. Serial: t=tom, d=demo, x=parar, s=status, p=dados, f=limpar chaves.");
}

void loop() {
  bool updated=BP32.update();
  uint32_t now=millis();
  if(updated && controller && controller->isConnected() && controller->hasData())
    vehicle.input(readController(),now);
  serialCommands(now);
  if(toneTest || demo || !soundHealthy(now) || !motorReady.load()) vehicle.disarm();
  vehicle.tick(now);
  const truck::State& s=vehicle.state();
  audio::Command command;
  command.engine=s.engine; command.load=s.rpmLoad; command.horn=s.horn;
  command.reverse=s.throttle<-0.01f; command.volume=s.volume; command.brakeEvent=s.brakeEvent;
  command.test=toneTest;
  if(demo) {
    uint32_t age=now-demoStart;
    command.engine=true; command.load=age>2500 && age<7000?float(age-2500)/4500:0;
    command.horn=age>=7500 && age<8500; command.reverse=age>=9000 && age<11500;
  }
  AudioMessage a={command,now}; xQueueOverwrite(audioQueue,&a);
  MotorMessage m={s,now}; xQueueOverwrite(motorQueue,&m);
  static uint32_t debugTime=0;
  if(rawDebug && controller && uint32_t(now-debugTime)>=500) {
    debugTime=now;
    Serial.printf("x=%ld R2=%ld L2=%ld botoes=%04x misc=%04x dpad=%02x\n",
      long(controller->axisX()),long(controller->throttle()),long(controller->brake()),
      controller->buttons(),controller->miscButtons(),controller->dpad());
  }
  delay(5); // cede CPU para Bluetooth e tarefas do sistema
}
