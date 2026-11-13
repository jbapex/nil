# Origem e direitos de componentes

## Código deste projeto

A lógica do caminhão, mixer, acionamento, diagnóstico, importador e testes foram escritos para este projeto. Não foram copiados do controlador de veículos original. Não foi escolhida uma licença pública em nome do proprietário.

## Amostras de áudio

Fonte: https://github.com/TheDIYGuy999/Rc_Engine_Sound_ESP32

Revisão fixada: `5398ee3b9041d71a8b3f105576f17b433b3d010b`.

Arquivos em `src/vehicles/sounds/`: `ScaniaV8start.h`, `ScaniaV8idle.h`, `ScaniaV8rev.h`, `ScaniaV8trainHorn.h`, `TruckAirBrakes2.h`. O importador preserva os valores PCM, identifica os arquivos de origem e verifica o hash de cada arquivo. O beep de ré e o tom de diagnóstico são sintetizados pelo código novo.

As amostras continuam pertencendo aos respectivos titulares. Não foi encontrado um arquivo de licença na raiz do checkout consultado. Isso não equivale a permissão irrestrita de redistribuição. `SoundBank.h`, prévia e binários de entrega incorporam esses sons para o teste solicitado; eles não recebem uma licença inventada neste projeto. Antes de publicação/redistribuição pública ou uso comercial, verificar as permissões aplicáveis com o upstream. O repositório fonte pode manter apenas o importador, com `SoundBank.h` fora do controle de versão.

## Bluepad32 e ESP32

Bluepad32: https://github.com/ricardoquesada/bluepad32 — seus componentes têm licenças próprias, incluindo Apache-2.0 / LGPL-2.1-or-later e dependências como BTstack. Consultar os arquivos de licença da versão usada.

Pacote oficial utilizado: https://github.com/ricardoquesada/esp32-arduino-lib-builder/releases/tag/4.1.0

Arduino-ESP32 / ESP-IDF: https://github.com/espressif/arduino-esp32 e https://github.com/espressif/esp-idf — licenças dos respectivos componentes mantidas no pacote de desenvolvimento. Bibliotecas não são revendidas como código novo deste projeto.
