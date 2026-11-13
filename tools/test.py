#!/usr/bin/env python3
import os,pathlib,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
 with tempfile.TemporaryDirectory() as tmp:
  common=['g++','-std=c++11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-g','-I',str(ROOT/'RCTruckPS4')]
  env=dict(os.environ)
  # LeakSanitizer cannot run under some traced build containers. Optional switch;
  # address and undefined-behavior checks remain enabled.
  if os.environ.get('RC_TEST_NO_LSAN'):env['ASAN_OPTIONS']='detect_leaks=0'
  binary=str(pathlib.Path(tmp)/'core')
  subprocess.run(common+[str(ROOT/'tests/test_core.cpp'),'-o',binary],check=True)
  subprocess.run([binary],env=env,check=True)
  for mode in range(3):
   binary=str(pathlib.Path(tmp)/f'hardware-{mode}')
   subprocess.run(common+['-I',str(ROOT/'tests/fake'),f'-DRC_DRIVER={mode}',str(ROOT/'tests/test_hardware.cpp'),'-o',binary],check=True)
   subprocess.run([binary],env=env,check=True)
if __name__=='__main__':main()
