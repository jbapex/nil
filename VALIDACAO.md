# Validação — RC Truck PS4 v0.1 — 12/11/2026

## Verificado nesta entrega

- Compilação real com Arduino CLI 1.6.0, pacote oficial ESP32 + Bluepad32 4.1.0, Arduino-ESP32 2.0.17, ESP32 Dev Module, 240 MHz, flash 4 MB DIO 40 MHz, Huge APP.
- Perfis bancada, ESC e BTS7960 compilados sem erros. Uso de programa: 784277 / 790365 / 792185 bytes; RAM global: 87196 / 87292 / 87300 bytes, respectivamente.
- Imagens completas combinam bootloader em 0x1000, tabela de partições em 0x8000, boot_app0 em 0xe000 e aplicação em 0x10000. Cada segmento foi comparado byte a byte com a imagem final. Gravar a imagem completa em **0x0**.
- Testes nativos C++11 com `-Wall -Wextra -Werror`, AddressSanitizer e UndefinedBehaviorSanitizer passaram. LeakSanitizer foi desabilitado porque o ambiente executa os processos sob rastreamento; checagens de endereços e comportamento indefinido permaneceram ativas.
- Lógica: armamento em neutro, recusa com gatilho pressionado, partida, rampa, limite de velocidade comandada, bloqueio de inversão sem liberação, freio, timeout de 350 ms, reconexão sem rearme automático, emergência, limites de volume, buzina e rollover do relógio.
- Mixer: silêncio quando desligado, partida seguida de loop, motor e buzina simultâneos, término da buzina, volume zero, tom de teste e estresse de efeitos em paralelo.
- Saídas simuladas: bancada sem PWM de movimentação, ESC em neutro por inibição/desarmamento, BTS com os dois PWMs nunca ativos ao mesmo tempo, enables baixos em neutro e inibição por dados vencidos.
- Dados de áudio: SHA256 verificado contra a revisão fixada do upstream. Importação sem execução de código remoto.
- Prévia real de 12 segundos gerada pelo mesmo mixer, com partida, aceleração, buzina e beep de ré. RMS não nulo nas partes ativas e silêncio no fim; sem saturação na prévia a 60% de volume.
- Scripts Python passaram na verificação de sintaxe.

## Ainda não verificado

Não houve acesso físico ao seu caminhão, placa, amplificador, ESC ou BTS, nem pareamento do seu controle genérico. Os testes não provam que o som já sai no seu alto-falante. A sequência recomendada é: perfil de bancada → `t` → `d` → pareamento → motor+buzina simultâneos → confirmação dos pinos → teste de tração com rodas suspensas.

Pinagem de movimento proposta, não confirmada na montagem existente. O controle genérico pode precisar de outro parser/mapeamento. O ESC pode ter outra faixa de pulsos ou sequência de frente/freio/ré; o perfil atual supõe um ESC bidirecional em modo crawler. A implementação BTS supõe BTS7960/IBT-2 para motor escovado. Não há controle por sensor de velocidade, freio mecânico, transmissão de três marchas, painel ou carreta sem fio.

## GitHub

Destino do código: https://github.com/jbapex/nil . Este relatório registra os testes locais da versão v0.1. O workflow disponível na aba Actions repete a suíte de software e a compilação dos três perfis. A execução do workflow não substitui a validação física e seus resultados devem ser consultados nos logs da execução correspondente.
