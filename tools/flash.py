#!/usr/bin/env python3
"""Grava imagem completa com offsets corretos. Nunca calibra ESC automaticamente."""
import argparse,hashlib,json,pathlib,subprocess,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser()
 p.add_argument('--port',required=True,help='Ex.: COM5 ou /dev/ttyUSB0')
 p.add_argument('--profile',choices=['bancada','esc','bts7960'],default='bancada')
 a=p.parse_args()
 image=ROOT/'firmware'/('RCTruckPS4_'+a.profile+'.bin')
 if not image.exists():raise SystemExit('Imagem completa nao encontrada em firmware/. Use o pacote de entrega ou compile.')
 manifest=json.loads((ROOT/'firmware/SHA256.json').read_text())
 if hashlib.sha256(image.read_bytes()).hexdigest()!=manifest[image.name]['sha256']:
  raise SystemExit('Checksum da imagem incorreto. Baixe o pacote novamente.')
 subprocess.run([sys.executable,'-m','esptool','--chip','esp32','--port',a.port,'--baud','115200',
   '--before','default_reset','--after','hard_reset','write_flash','--flash_mode','dio',
   '--flash_freq','40m','--flash_size','4MB','0x0',str(image)],check=True)
if __name__=='__main__':main()
