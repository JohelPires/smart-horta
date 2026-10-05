# Smart Horta — Roadmap / TODO

Controlador de irrigação ESP32. Escrito para um dev backend estudando
eletrônica, C e embarcado. As fases são executadas de cima para baixo; cada
uma tem **Entrega** e **Critério de saída** verificáveis no Wokwi antes de
usar hardware real. A arquitetura final está em
[`docs/architecture.md`](docs/architecture.md).

Legenda: `[ ]` pendente · `[x]` feito · ⚠️ = erro comum de iniciante · 🧠 = conceito para aprender

Marcos:

- [x] M1 — Acender LED no Wokwi (fases 0–2)
- [ ] M2 — Ler sensor real e logar (fases 3–4)
- [ ] M3 — Irrigação com gatilhos, fail-safe, hora e display (fases 5–7)
- [ ] M4 — Publicar telemetria MQTT no Wokwi (fase 8)
- [ ] M5 — Host armazena + exibe telemetria (fase 9)
- [ ] M6 — Mesmo firmware rodando em hardware real (fase 10)
- [ ] M7 — Irrigação fail-safe + CI verde (fase 11)

---

## Fase 0 — Ambiente & tooling

🧠 ESP-IDF é um toolchain + sistema de build; `idf.py` encapsula CMake/ninja/esptool.
**Entrega:** `idf.py --version` funciona num shell novo.

- [x] Instalar ESP-IDF v5.5.5 (`~/esp-idf`), rodar `install.sh esp32`
- [x] Instalar VS Code + extensões: Espressif IDF, Wokwi Simulator, Serial Monitor
- [x] Usar `wokwi-cli` standalone (instalado em `~/bin/wokwi-cli`) — `idf.py wokwi`
      precisa IDF ≥ 6.0, descartado
- [ ] Criar conta Wokwi; licença da extensão (F1 -> "Wokwi: Request a new License")
- [ ] `export WOKWI_CLI_TOKEN=...` (para CI depois; no `~/.bashrc`, nunca commitar)
- [x] Instalar Python 3.11+, `git` (Mosquitto adiada para a fase 8)
- [x] Estruturar dirs do repo: `firmware/`, `host/`, `docs/`, `emulation/`
- [x] `cd firmware && idf.py set-target esp32`

---

## Fase 1 — Fundamentos de eletrônica (breadboard + teoria)

🧠 Começar no Wokwi, sem risco de queimar peças. Aprender medindo.
**Entrega:** circuito na breadboard com LED + resistor + botão, sabendo explicar.

- [x] 🧠 Tensão (V), corrente (A), resistência (Ω), lei de Ohm `V = I·R` → `docs/electronics-notes.md` §1
- [x] 🧠 Potência `P = V·I`; por que um resistor limita corrente e protege o LED → §2
- [x] 🧠 Lógica digital: HIGH/LOW. **GPIO do ESP32 = 3,3V, NÃO tolera 5V** ⚠️ → §3
- [x] 🧠 GND: todas as peças precisam de massa comum ⚠️ → §4
- [ ] Montar LED no wokwi.com: GPIO 4 → 330Ω → LED vermelho → GND (`wokwi-led`, `wokwi-resistor`)
- [ ] Montar botão no wokwi.com: pushbutton GPIO 23 → GND, com pull-up interno (solto=1, apertado=0) ⚠️ aprender pin flutuando
- [x] 🧠 Limites de corrente: ~12mA seguros / 40mA absoluto por GPIO; nunca ligar motor direto num GPIO ⚠️ → §5
- [x] 🧠 Divisor de tensão (usado depois por sensores resistivos) → §6
- [x] 🧠 Por que cargas indutivas (bombas, solenoides) precisam de **diodo flyback** → §8
- [x] 🧠 Isolamento: optoacopladores/relés afastam alta tensão do ESP32 → §9
- [x] ⚠️ Regra de segurança: nunca chavear 110/220V em breadboard → §10

**Critério de saída:** dois circuitos montados no wokwi.com + reler
`docs/electronics-notes.md` §2 + §7.

---

## Fase 2 — C & ESP-IDF (blink no Wokwi)

🧠 C para devs backend: sem GC, memória manual, ponteiros, `struct`, tipos de tamanho fixo.
**Entrega:** app blink rodando no Wokwi.

- [x] 🧠 C crash course: tipos, arrays, ponteiros, `struct`, `enum`, headers, `static`
- [x] 🧠 `esp_err_t` + `ESP_ERROR_CHECK`; logs `ESP_LOGI/W/E` (usados em `main/main.c`)
- [x] Anatomia de projeto ESP-IDF: `main/CMakeLists.txt`, `CMakeLists.txt` raiz, `Kconfig`, `sdkconfig.defaults`
- [x] 🧠 FreeRTOS básico: tasks (`xTaskCreate`), `vTaskDelay`, queues, semáforos
- [x] Criar `firmware/wokwi.toml` apontando para `build/flasher_args.json` + `build/smart_horta.elf`
- [x] Criar `firmware/diagram.json` (DevKit + LED + resistor)
- [x] Escrever blink com `gpio_set_level`/`gpio_set_direction`; `idf.py build`
- [x] Rodar no VS Code "Wokwi: Start Simulator" e ver o LED piscar
- [x] Rodar `wokwi-cli . --timeout 10000 --expect-text "..."` passando (`LED ON`)
      **Critério de saída:** M1 ok; build limpo + blink simulado. ✅

---

## Fase 3 — Sensores digitais & GPIO (DHT22 + botão)

🧠 "Protocolos" de sensor: one-wire (DHT22), I2C, SPI.
**Entrega:** temperatura + humidade no log serial a cada 2 s ✅.

- [x] Adicionar `wokwi-dht22` ao `diagram.json`; VCC→3.3V, GND→GND, SDA→GPIO15, + 10kΩ pull-up ⚠️
- [x] Adicionar driver DHT como componente (biblioteca "DHT sensor library for ESPx" via `idf_component.yml`) 🧠
- [x] Ler temperatura/humidade com `dht_read_float_data()`; validar comparando com os sliders do Wokwi
- [x] 🧠 Intervalo mínimo de amostragem do DHT22 ≈ 2 s ⚠️ (`DHT_INTERVAL_MS = 2000`)
- [x] (opcional) Botão como "manual override": GPIO 23 → GND com pull-up; edge-detect por polling de 10 ms
      *(sem debounce dedicado ainda — revisar na fase 5)*
- [ ] Float switch de nível: cobrir na fase 4 (entrada digital GPIO 33)

**Critério de saída:** leitura estável; valores mudam ao arrastar sliders. ✅

---

## Fase 4 — ADC, calibração & sensores analógicos

🧠 Analógico ≠ digital: ADC converte 0–3,3V em número (ESP32 12-bit: 0–4095).
**Entrega:** solo calibrado em %, chuva analógica + digital e bóia lidos.

- [ ] ⚠️ Regra-chave: **ADC2 indisponível com WiFi ativo** → usar **ADC1 (GPIO32–39)**
- [ ] 🧠 ADC oneshot com atenuação 11dB para faixa completa 0–3,3V
- [ ] 🧠 GPIO34–39 são input-only (sem pull-ups); GPIO32/33 têm pull-ups
- [ ] Adicionar sensor de humidade do solo (capacitivo no HW real; `wokwi-potentiometer`/`wokwi-slide-potentiometer` como proxy) no GPIO 34
- [ ] Adicionar chuva analógica no GPIO 35 (proxy: potenciômetro) e chuva digital no GPIO 32 (módulo com pot-calibração; no Wokwi simular via pot/log)
- [ ] Adicionar bóia de nível no GPIO 33 (entrada digital com pull-up)
- [ ] Ler valores brutos; mapear para 0–100% com clamp
- [ ] 🧠 Direção da leitura varia por modelo: calibrar sempre com "ar/seco" e "água/molhada" para definir o mapeamento
- [ ] Calibrar: registrar raw de "ar/seco" e "água/molhado" → salvar em Kconfig
- [ ] Opcional: `wokwi-ntc-temperature-sensor` em outro pino ADC1
      **Critério de saída:** M2 ok; % do solo acompanha o slider e está calibrado.

---

## Fase 5 — Atuadores, fail-safe & motor de gatilhos da irrigação

🧠 GPIOs controlam *sinais*; fonte externa alimenta *cargas* — separar sempre.
**Entrega:** máquina de estados da irrigação com os 4 gatilhos de LIGAR e os
4 de DESLIGAR, fail-safe garantido.

- [x] 🧠 Relé vs MOSFET vs SSR; por que relé/SSR isola a carga
- [ ] Usar `wokwi-relay-module` (default `npn` = **ativo-alto**; NO → LED "solenoide")
- [ ] ⚠️ Confirmar lógica do módulo real depois (alguns são ativo-BAIXO)
- [ ] 🧠 GPIO 3.3V não aciona bobina de relé 5V direto → usar o driver do módulo
- [ ] Refatorar `main.c` em `components/` (`hal` + `board.h` primeiro; faltam o `hal`/`board.h` (fase 6) e os drivers de sensor (fase 4))
- [ ] Máquina de estados: IDLE → IRRIGATING → COOLDOWN (`irrigation` = dono único do relé)
- [ ] Motor de arbitragem — LIGAR: botão (T1), agenda (T2), limiar sensor (T3), comando MQTT (T4);
      DESLIGAR: timer do usuário (S1), botão de novo (**S2 = prioridade máxima**),
      evento de chuva (S3), comando MQTT (S4)
- [ ] Rain-hold: evento de chuva fecha válvula e adia regas automáticas por N h (default 12 h)
- [ ] Cooldown/histerese após cada rega (evita religar em cascata no limiar)
- [ ] 🧠 Watchdog + timeout: fecha válvula se tarefa travar ⚠️
- [ ] 🧠 Fail-safe no init com erro: válvula fecha (relé LOW)
- [ ] Guardar fonte da rega (`btn/sched/sensor/cmd`) — usada no `event/irrigation` e no OLED
- [ ] Thresholds em Kconfig: `soil_trig_pct`, `air_hum_trig_pct`, `max_run_s`
      **Critério de saída:** M3 ok; limiar abre válvula e toda falha fecha.

---

## Fase 6 — Núcleo do firmware: NVS, agenda, SNTP & componentes

🧠 Agenda semanal precisa de hora confiável: SNTP ao conectar WiFi; sem hora →
agenda fica "aguardando". Construir os componentes `config`, `network`.
**Entrega:** agenda sobrevive a reboot e roda sem WiFi.

- [ ] Criar `components/config`: NVS com CRC — agenda (dias/hora/duração),
      limiares, `max_run_s`, `rain_skip_h`, período de telemetria
- [ ] Comando do botão pressionado por 10 s = reset de fábrica (valores default + NVS apagada)
- [ ] Criar `components/network`: WiFi STA (SSID `Wokwi-GUEST` no Wokwi) com auto-reconnect
- [ ] SNTP: sincronizar hora ao conectar; firmware segue rodando, com a agenda desabilitada até `sntp == ok`
- [ ] Refatorar `main/main.c` para só init + coordenação lógica da aplicação nos components)
      **Critério de saída:** reboot mantém agenda; sem WiFi a rega programada continua.

---

## Fase 7 — Display OLED SSD1306 (I2C)

**Entrega:** OLED mostra temp/humidade, solo, estado da válvula e contagem regressiva.

- [ ] Adicionar `wokwi-ssd1306` (I2C, SDA=21/SCL=22) ao `diagram.json`
- [ ] Criar `components/ui_oled` (driver SSD1306, ex.: função `ui_oled_render`)
- [ ] Tela 1 (normal): temp/humidade/solo + estado ("REGANDO 03:32" / "Aguardando hora")
- [ ] Atualizar a ~1 Hz (I2C é lento; não bloquear tasks de leitura)
- [ ] Mostrar erros: código piscando no LED de status + mensagem no OLED
      **Critério de saída:** dados ao vivo no OLED durante simulação.

---

## Fase 8 — Networking, WiFi & MQTT (payoff da simulação)

🧠 MQTT = pub/sub via broker; níveis de QoS; mensagens retained.
**Entrega:** telemetria publicada + comandos recebidos no Wokwi.

- [ ] 🧠 WiFi STA; conectar `Wokwi-GUEST`, senha `""`, canal 6
- [ ] Adicionar `esp-mqtt` (in-tree no ESP-IDF); configurar URI do broker
- [ ] Wokwi: broker público (`broker.hivemq.com:1883` ou `test.mosquitto.org:1883`)
- [ ] Implementar tópicos conforme [`docs/mqtt-topics.md`](docs/mqtt-topics.md):
      `telemetry` (QoS0), `status` + `available`/last-will (retained),
      `event/rain|irrigation|alert`, `cmd/irrigation|schedule|config|get`, `ack`
- [ ] Evento de chuva → fecha válvula via state machine + publica `event/rain`
- [ ] 🧠 Auto-reconnect: tratar queda de WiFi e desconexão MQTT
- [ ] JSON pequeno e estável; tokens de comando com `req_id` + ack (~3 s timeout)
- [ ] Testar com Wireshark: baixar `wokwi.pcap` e inspecionar os pacotes MQTT
      **Critério de saída:** M4 ok; broker mostra telemetria; comandos acionam válvula.

---

## Fase 9 — Host/Raspberry Pi (Python): subscriber + dashboard web

🧠 Separar ingestão, armazenamento e apresentação.
**Entrega:** host armazena telemetria e serve dashboard web do celular/PC.

- [ ] `python -m venv .venv && pip install paho-mqtt flask` → `host/requirements.txt`
- [ ] `host/`: subscriber QoS1 → SQLite (`readings(ts, device_id, metric, value)`,
      `events(ts, type, detail)`)
- [ ] Dashboard Flask: dados ao vivo + histórico (gráficos Chart.js)
- [ ] Agenda no web: dias × hora × duração → POST → publish MQTT `cmd/schedule`
- [ ] Config no web: limiares/duração → publish `cmd/config`
- [ ] Botões manual on/off (com `duration_s`) → publish `cmd/irrigation`
- [ ] CLI (`python -m host.main`: `valve on|off`, `status`, `schedule set ...`)
- [ ] Apontar subscriber para o broker público do Wokwi (co-simulação)
- [ ] Loop 100% local: rodar Mosquitto (`emulation/mosquitto.conf`), ativar
      **Wokwi Private Gateway**, broker do fw = `host.wokwi.internal:1883`
- [ ] Segurança: Mosquitto escuta apenas na LAN; acesso externo só depois com TLS/VPS (ver mqtt-topics.md §6)
- [ ] `pytest host/` com publisher/subscriber fake: parsing + gravação + rotas da web
      **Critério de saída:** M5 ok; `pytest` verde e telemetria persistida.

---

## Fase 10 — Hardware real

🧠 A simulação esconde ruído, limites de alimentação, solda fria, calor.
**Entrega:** mesmo firmware no ESP32 físico.

- [ ] Lista de compras em `docs/bom.md`:
   - [ ] ESP32 DevKitC (ou DevKit v1)
   - [ ] OLED SSD1306, DHT22, sensor capacitivo de solo
   - [ ] Sensor de chuva (módulo de trilha, saída AO + DO)
   - [ ] Bóia de nível do reservatório
   - [ ] (opcional, backlog) Sensor de vazão YF-S201
   - [ ] Módulo relé/SSR adequado à válvula (12/24V DC); solenoide + **diodo flyback**
   - [ ] Fonte 12/24V separada + buck p/ ESP32 (5V) + **bomba p/ reservatório**
   - [ ] Resistores, jumpers, breadboard, multímetro
- [ ] ⚠️ Confirmar lógica de ativação do módulo relé real e corrente da carga
- [ ] 🧠 Arquitetura de energia: fonte da lógica separada da carga; **massa comum**
- [ ] ⚠️ Nunca puxar bomba/válvula pelo regulador do ESP32; brownout = resets aleatórios
- [ ] Ligar um sensor por vez; medir com multímetro energizando primeiro ⚠️
- [ ] `idf.py -p /dev/ttyUSB0 flash monitor`; conferir boot + WiFi
- [ ] Recalibrar os limiares no HW real (solo, chuva, água) e commitar em Kconfig
- [ ] `board.h` conferido contra o `diagram.json` real
- [ ] Teste de resistência de 24 h logando umidade + eventos de válvula
      **Critério de saída:** M6 ok; rega real disparada pelo solo real.

---

## Fase 11 — Confiabilidade, CI & docs

**Entrega:** teste de simulação automatizado + docs que um estranho entenda.

- [ ] Cenário Wokwi (`.scenario.yaml`) afirmando boot + WiFi + 1 telemetria
- [ ] GitHub Actions: build + `wokwi-cli --expect-text` (free tier 50 sim-min/mês; testes curtos)
- [ ] 🧠 Testes de fail-safe: forçar falha de sensor, afirmar válvula fechada
- [ ] Documentar wiring/calibração/contrato MQTT em `docs/`
- [ ] "Factory reset" fechando válvulas em reboot inesperado
- [ ] `WOKWI_CLI_TOKEN` como secret de CI (nunca commitar) ⚠️
      **Critério de saída:** M7 ok; CI verde e docs completos.

---

## Backlog / funcionalidades futuras

Não bloqueia nenhuma fase acima; captadas na revisão da arquitetura
(2026-10-05) — detalhes em [`docs/architecture.md` §11](docs/architecture.md#11-backlog--funcionalidades-futuras).

- [ ] Múltiplas zonas (2–4 válvulas fracionadas + agenda por zona)
- [ ] Sensor de vazão (YF-S201): litros por rega; alerta de vazamento
- [ ] Integração Home Assistant (MQTT discovery)
- [ ] Notificações via host (Telegram: reservatório baixo, falha, geada < ~5 °C)
- [ ] OTA update por WiFi (esp_https_ota)
- [ ] Histórico/gráficos em profundidade (evapotranspiração, curvas de humidade)
- [ ] Autenticação na web dashboard (senha; necessário para acesso externo)
- [ ] Modo noturno/fora de pico (irrigar de madrugada, água/energia mais barata)

---

## Apêndice A — Guia rápido de pinos ESP32

- ⚠️ **Não tolera 5V** — lógica é 3,3V.
- ⚠️ GPIO6–11 ligados à SPI flash — **não usar**.
- ⚠️ Input-only: GPIO34–39 (sem pull-ups). ADC1 = GPIO32–39 (analógico com WiFi on).
- ⚠️ Pinos de ADC2 conflitam com WiFi.
- ⚠️ Strapping pins (GPIO0, 2, 4, 5, 12, 15) afetam o boot — cuidado.
- UART0: TX=GPIO1, RX=GPIO3 (serial monitor/flash).
- I2C default: SDA=GPIO21, SCL=GPIO22. DAC default: GPIO25/26.

## Apêndice B — Recursos de estudo

- ESP-IDF Programming Guide + API Reference
- Docs do Wokwi (ESP32, WiFi, Parts, CLI)
- "DHT sensor library for ESPx"
- Eletrônica básica: lei de Ohm + divisores de tensão + tutoriais de relé/transistor
