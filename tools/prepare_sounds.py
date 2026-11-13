#!/usr/bin/env python3
"""Importa PCM do upstream fixado, sem executar codigo baixado."""
import argparse
import hashlib
import pathlib
import re
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[1]
REVISION = '5398ee3b9041d71a8b3f105576f17b433b3d010b'
SOURCE = 'https://raw.githubusercontent.com/TheDIYGuy999/Rc_Engine_Sound_ESP32/'
FILES = [
    ('start','ScaniaV8start.h','start','39e92f665157573f685580db4d7115b167e365ca0087ed8948745f6e4a203bc6'),
    ('idle','ScaniaV8idle.h','','38d84153d75f9622939b3d907dd9344165b18118b30366586b5e6922a49ad11f'),
    ('rev','ScaniaV8rev.h','rev','b1f665fbe3ad40078156758847fb1e591e14d603f0a70edafc942d610ae86573'),
    ('horn','ScaniaV8trainHorn.h','horn','de12b9cc7409c6936794eb9a94278f307a5d45d86bf1e22ef19cd1ecfed2140d'),
    ('brake','TruckAirBrakes2.h','brake','4f8eec0ecee671808fa76d8dac45d2400013a51f003929e2dc9d7031ea42f4b8'),
]

def parse(data, prefix):
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', data.decode(), flags=re.S)
    name = lambda s: prefix+s if prefix else s[0].lower()+s[1:]
    def value(s, default=None):
        m = re.search(r'\b'+name(s)+r'\s*=\s*(\d+)\s*;', text)
        if m: return int(m.group(1))
        if default is None: raise ValueError(f'Metadado ausente: {name(s)}')
        return default
    m = re.search(r'\b'+name('Samples')+r'\s*\[\s*\]\s*=\s*\{([^}]*)\}', text, re.S)
    if not m: raise ValueError('Array PCM ausente')
    values = [int(x) for x in m.group(1).replace('\n','').split(',') if x.strip()]
    rate,count = value('SampleRate'),value('SampleCount')
    # Some upstream headers have an extra last sample. Respect declared count.
    if len(values)<count or any(v < -128 or v > 127 for v in values):
        raise ValueError('PCM invalido')
    begin,end = value('LoopBegin',0),value('LoopEnd',0)
    if end and not 0<=begin<end<=count: raise ValueError('Loop invalido')
    return values[:count],rate,begin,end

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--upstream-dir',type=pathlib.Path,help='pasta local vehicles/sounds (opcional)')
    args=parser.parse_args()
    lines=['#pragma once','#include "AudioMixer.h"',
           '// PCM original: TheDIYGuy999/Rc_Engine_Sound_ESP32, revisao '+REVISION,
           '// Veja THIRD_PARTY.md. Este arquivo e gerado por tools/prepare_sounds.py.',
           'namespace sounds {']
    banks=[]
    for label,filename,prefix,digest in FILES:
        data=(args.upstream_dir/filename).read_bytes() if args.upstream_dir else urllib.request.urlopen(
            SOURCE+REVISION+'/src/vehicles/sounds/'+filename,timeout=60).read()
        if hashlib.sha256(data).hexdigest()!=digest: raise ValueError('SHA256 incorreto: '+filename)
        values,rate,begin,end=parse(data,prefix)
        lines+=['// '+filename,'static const int8_t '+label+'Pcm[] = {']
        lines += [','.join(map(str,values[i:i+32]))+',' for i in range(0,len(values),32)]
        lines+=['};']
        banks.append('{'+f'{label}Pcm,{len(values)},{rate},{begin},{end}'+'}')
        print(f'{label}: {len(values)} amostras, {rate} Hz, SHA256 OK')
    lines+=['static const audio::Bank bank = {'+','.join(banks)+'};','}']
    target=ROOT/'RCTruckPS4/SoundBank.h'
    target.write_text('\n'.join(lines)+'\n')
    print('Gerado:',target)

if __name__=='__main__': main()
