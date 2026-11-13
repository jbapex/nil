#!/usr/bin/env python3
"""Compila perfis para ESP32 classico usando o pacote oficial Bluepad32 4.1.0."""
import argparse,json,pathlib,shutil,subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
URLS=','.join([
 'https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json',
 'https://raw.githubusercontent.com/ricardoquesada/esp32-arduino-lib-builder/master/bluepad32_files/package_esp32_bluepad32_index.json'])
FQBN='esp32-bluepad32:esp32:esp32:PartitionScheme=huge_app,CPUFreq=240,FlashMode=dio,FlashFreq=40'
PROFILES={'bancada':0,'esc':1,'bts7960':2}
def main():
 p=argparse.ArgumentParser();p.add_argument('--cli',default='arduino-cli')
 p.add_argument('--profile',choices=['all']+list(PROFILES),default='all')
 p.add_argument('--install',action='store_true');a=p.parse_args()
 if a.install:
  subprocess.run([a.cli,'core','update-index','--additional-urls',URLS],check=True)
  subprocess.run([a.cli,'core','install','esp32-bluepad32:esp32@4.1.0','--additional-urls',URLS],check=True)
 platforms=json.loads(subprocess.check_output([a.cli,'core','list','--format','json']))
 cores=platforms.get('platforms',[]) if isinstance(platforms,dict) else platforms
 if not any(x.get('id')=='esp32-bluepad32:esp32' and x.get('installed_version')=='4.1.0' for x in cores):
  raise SystemExit('Instale o core correto: python tools/build.py --install')
 if not (ROOT/'RCTruckPS4/SoundBank.h').exists():
  raise SystemExit('Execute primeiro: python tools/prepare_sounds.py')
 for name,value in PROFILES.items():
  if a.profile not in ('all',name):continue
  build=ROOT/'build'/name;build.mkdir(parents=True,exist_ok=True)
  subprocess.run([a.cli,'compile','--fqbn',FQBN,'--build-property',f'compiler.cpp.extra_flags=-DRC_DRIVER={value}',
    '--output-dir',str(build),str(ROOT/'RCTruckPS4')],check=True)
  # Upload via arduino-cli funciona com o projeto, sem precisar montar flash manual.
  # Somente o app e bootloader/particoes sao distribuidos; ELF/map ficam locais.
  for suffix in ('.elf','.map'):
   file=build/('RCTruckPS4.ino'+suffix)
   if file.exists():file.unlink()
if __name__=='__main__':main()
