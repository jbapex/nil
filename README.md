# RC Truck PS4 — caminhão 1/14 — v0.1 de bancada

Firmware novo para **ESP32 clássico (WROOM/WROVER)**, controle PS4 por Bluetooth e amplificador analógico PAM nos GPIOs **25 e 26**. Não usa a adaptação antiga. A lógica, o mixer e o controle dos motores foram escritos neste projeto; as amostras de motor/partida/buzina/freio são do TheDIYGuy999 e continuam creditadas a ele.

**Estado:** os três perfis foram compilados. A lógica e o áudio passaram em testes no computador. Ainda falta validar o seu controle genérico, o DAC/PAM e a movimentação no caminhão. Esta versão não implementa todos os recursos do projeto original: transmissão de três marchas, carreta ESP-NOW, painel, turbo e Jake brake ficam para etapas posteriores.

## Primeiro teste: som + PS4, sem movimentação

1. Preserve uma cópia do firmware/configuração que funciona. Use primeiro o perfil **bancada**; ele não gera sinal de ESC, BTS ou servo.
2. Mantenha a ligação de áudio que já funciona no original: GPIO25/26 para as entradas analógicas do PAM, com os resistores/potenciômetro/acoplamento existentes e GND comum. Ambos os pinos recebem o mix completo. Não junte as saídas de alto-falante do PAM.
3. Grave `firmware/RCTruckPS4_bancada.bin` (imagem completa, endereço **0x0**, flash **4 MB**). Esse binário supõe ESP32 clássico com 4 MB; confirme o módulo antes de gravar. Para outro tamanho, compile a configuração adequada.
4. Abra o monitor serial em **115200**. Envie `t`: deve soar um tom de 440 Hz por 2 segundos. Envie `d`: toca uma demonstração de partida, aceleração, buzina e ré por 12 segundos.
5. Segure **SHARE + PS** no controle até a luz piscar. O monitor informa modelo e VID/PID. Se o genérico não parear, anote o modelo/link e os logs; não há garantia para qualquer clone.
6. Aperte e solte **Triângulo** para dar partida. Use **R2** para ouvir o giro subir e **X** para buzinar ao mesmo tempo. O teste valida especificamente áudio contínuo enquanto o Bluetooth recebe comandos.
7. Envie `s`: confira `audio=1`, `erros=0` e `blocos` aumentando. Isso confirma alimentação do DMA, mas ouvir o som no alto-falante continua sendo necessário.

Para gravar pelo terminal, instale Python e `pip install esptool==4.5.1 pyserial`, depois:

```sh
python tools/flash.py --port COM5 --profile bancada
```

No Linux a porta pode ser `/dev/ttyUSB0`. Se a placa não entrar no carregador, use o botão BOOT no início da gravação. O teste de bancada precisa de USB e da alimentação normal do PAM, não da tração.

## Botões propostos

| Comando | Função |
|---|---|
| Analógico esquerdo, horizontal | Direção |
| R2 | Acelerar para frente; também sobe o giro com tração desarmada |
| L2 | Frear; ré após parar, soltar os dois gatilhos por 0,3 s e apertar novamente |
| Triângulo | Ligar/desligar motor sonoro quando parado; desarma a tração |
| OPTIONS | Armar/desarmar tração; exige partida concluída e ambos os gatilhos soltos por 0,5 s |
| Círculo | Parada imediata do comando de tração; exige novo armamento |
| X, mantido | Buzina com motor tocando em paralelo |
| L1 / R1 | Faróis / pisca-alerta (saídas opcionais) |
| D-pad esquerda/direita | Setas (saídas opcionais) |
| D-pad cima/baixo | Volume em passos de 5% |

Esses nomes correspondem ao mapeamento padrão DualShock 4 do Bluepad32. `p` no serial mostra eixos e máscaras para conferir um controle genérico. Na conexão, solte os botões antes do primeiro comando.

## ESC ou BTS7960

São perfis **alternativos para a tração**, não duas saídas independentes simultâneas. O BTS foi interpretado como **BTS7960 / IBT-2**; outro módulo exige revisão do driver. Nenhum perfil identifica automaticamente os fios ou o tipo de motor.

| Ligação | Perfil ESC | Perfil BTS7960 |
|---|---|---|
| Servo de direção, sinal | GPIO13 | GPIO13 |
| ESC, sinal | GPIO33 | Não usado |
| BTS RPWM / LPWM | Não usados | GPIO18 / GPIO19 |
| BTS R_EN / L_EN | Não usados | GPIO23 / GPIO22 |
| PAM, entrada de áudio | GPIO25 e/ou GPIO26 | GPIO25 e/ou GPIO26 |
| Referência dos sinais | GND comum | GND comum |

**Estes pinos de movimento são a configuração proposta para o firmware novo.** Não se presume que correspondam à sua placa ou à fiação existente. Confira antes de conectar e edite `Config.h` se necessário. Não conecte um motor diretamente ao ESP32. O BTS7960 controla motor DC escovado; não substitui um ESC de motor brushless. R_EN e L_EN precisam estar ligados aos GPIOs indicados, não presos permanentemente em nível alto. Use alimentação do motor e da lógica conforme os seus módulos; não alimente motor/servo pelo 3V3 do ESP32. Se o seu PAM for o PAM8403, conserve a alimentação adequada que já funcionava no projeto original.

O ESC deve aceitar neutro em 1500 µs e frente/ré em torno de 2000/1000 µs, preferencialmente modo crawler frente/ré. ESC com sequência própria de duplo freio precisa de adaptação específica. A versão não faz calibração automática de extremos. No BTS, neutro desabilita a ponte e deixa o motor em roda livre; a redução do PWM não equivale a freio mecânico ou medição da velocidade real.

Depois de validar o áudio, confira a fiação, suspenda as rodas e grave o perfil escolhido (`esc` ou `bts7960`). Dê partida, solte os gatilhos, espere a partida terminar e aperte OPTIONS. Os limites iniciais são 35% para frente e 25% para ré. Ajuste primeiro o sentido e os limites do servo. Uma mudança de direção exige parada e liberação dos gatilhos.

Perda de relatórios por mais de 350 ms, desconexão, falha da tarefa de áudio ou comandos vencidos colocam a saída em neutro. A tarefa de saída é separada do loop do controle. Isso é proteção de software, sem sensor de velocidade nem garantia contra defeito do hardware. Para começar novamente após perda de sinal, solte os gatilhos e arme de novo.

## Compilar / modificar

Use **Arduino IDE com a placa “ESP32 Dev Module” do pacote ESP32 + Bluepad32 4.1.0**. Não instale Bluepad32 como simples biblioteca sobre uma placa ESP32 comum: o Bluetooth vem integrado no pacote de placas. CPU 240 MHz, flash 4 MB, DIO 40 MHz, partição Huge APP. C3/S3 não servem para este firmware: faltam o DAC analógico 25/26 e o Bluetooth Classic exigido pelo PS4.

Os URLs do gerenciador de placas estão em `tools/build.py`. O projeto também pode ser compilado com Arduino CLI:

```sh
python tools/prepare_sounds.py
python tools/build.py --install --profile all
```

`prepare_sounds.py` baixa somente dados PCM de uma revisão fixa, confere SHA256 e gera `SoundBank.h`. Nenhum código remoto é executado. O som inicial escolhido é Scania V8 como demonstração; pode ser trocado depois. O pacote de entrega já contém `SoundBank.h` preparado. O repositório mantém o importador e os créditos.

No Arduino IDE, `RC_DRIVER` em `Config.h`: `0=bancada`, `1=ESC`, `2=BTS7960`. O script de compilação seleciona isso automaticamente. Saídas de luz vêm como `-1` (desativadas) até definirmos a fiação; usar acionamento elétrico adequado.

Arquitetura: Bluepad32 recebe relatórios; a lógica calcula o estado; uma fila entrega o estado à tarefa de saída; outra fila alimenta o mixer. O áudio é PCM a 22050 Hz via I2S0/DMA para os DACs internos, com motor, buzina e efeitos misturados, sem `tone()` e sem um timer por amostra. Não há Wi-Fi nesta versão.

## Comandos seriais

| Tecla | Ação |
|---|---|
| `t` | Tom de diagnóstico por 2 segundos, com tração desarmada |
| `d` | Demonstração de áudio por 12 segundos, com tração desarmada |
| `x` | Desarma tração e encerra testes de áudio |
| `s` / `?` | Estado, contadores de áudio e memória |
| `p` | Liga/desliga dados brutos do controle a cada 0,5 s |
| `f` | Remove chaves de pareamento quando desarmado |

## Testes

```sh
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -I RCTruckPS4 tests/test_core.cpp -o test_core
./test_core
```

Há testes de armamento em neutro, partida, aceleração, bloqueio de inversão, freio, timeout, reconexão, emergência, volume e rollover de `millis()`. O mixer é testado com partida/loop, motor+buzina, silêncio, volume e estresse. `tests/render_audio.cpp` gera a prévia usando exatamente o mixer do firmware e as amostras reais. Esses testes não substituem medir/ouvir o sinal nos GPIOs nem parear o seu controle.

`python tools/test.py` executa a suíte completa, incluindo saídas simuladas de bancada/ESC/BTS. O teste do BTS verifica a exclusão dos dois sentidos em cada escrita de PWM. O relatório da entrega está em `VALIDACAO.md`; `examples/previa_sons.wav` permite ouvir a renderização do mixer no computador.

## GitHub

Repositório do projeto: https://github.com/jbapex/nil

O GitHub contém o código, os testes e o importador dos sons. Ao baixar o repositório, execute `python tools/prepare_sounds.py` antes de abrir o sketch no Arduino IDE. `SoundBank.h`, a prévia e os binários que incorporam amostras de terceiros ficam fora do controle de versão; o pacote ZIP entregue na conversa já contém esses arquivos para o teste solicitado. Pelo código do repositório, compile e envie o sketch pelo Arduino IDE com o perfil selecionado em `Config.h`; as imagens completas citadas no primeiro teste são as do pacote ZIP.

A aba **Actions** executa testes e compila os três perfis a cada alteração. A existência do workflow não significa que houve teste físico no caminhão. Os créditos e as condições dos componentes estão em `THIRD_PARTY.md`.
