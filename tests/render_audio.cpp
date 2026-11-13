#include <stdio.h>
#include <stdint.h>
#include "SoundBank.h"
static void u16(FILE*f,uint16_t v){fputc(v&255,f);fputc(v>>8,f);}
static void u32(FILE*f,uint32_t v){u16(f,v&65535);u16(f,v>>16);}
int main(int argc,char**argv) {
  if(argc!=2)return 1;
  FILE*f=fopen(argv[1],"wb");if(!f)return 2;
  uint32_t samples=cfg::sampleRate*12;
  fwrite("RIFF",1,4,f);u32(f,36+samples*2);fwrite("WAVEfmt ",1,8,f);
  u32(f,16);u16(f,1);u16(f,1);u32(f,cfg::sampleRate);u32(f,cfg::sampleRate*2);
  u16(f,2);u16(f,16);fwrite("data",1,4,f);u32(f,samples*2);
  audio::Mixer mixer(sounds::bank);audio::Command c;c.volume=60;
  for(uint32_t i=0;i<samples;i++) {
    float sec=float(i)/cfg::sampleRate;
    c.engine=sec<11.5f;c.load=sec>2.5f&&sec<7?(sec-2.5f)/4.5f:0;
    c.horn=sec>=7.5f&&sec<8.5f;c.reverse=sec>=9&&sec<11.5f;
    mixer.command(c);u16(f,uint16_t(mixer.next()));
  }
  fclose(f);return 0;
}
