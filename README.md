# smart-horta

Controlador de irrigação para horta/jardim com ESP32: lê sensores de
temperatura/humidade do ar, humidade do solo, chuva e nível de água; controla
válvulas/bomba; mostra os dados num display OLED e publica telemetria via
MQTT para um host (computador/Raspberry Pi) com dashboard web. Monorepo em
construção.

**Documentação:**

- [Arquitetura final](docs/architecture.md) — visão completa com diagramas
- [Referência MQTT](docs/mqtt-topics.md) — tópicos e payloads
- [Notas de eletrônica](docs/electronics-notes.md)
- [Roadmap](TODO.md)

## Funcionalidades (desenho final)

Liga a irrigação por: **(1)** botão físico (imediato), **(2)** agenda semanal
(dias da semana × hora), **(3)** limiar de sensor (ex.: humidade do solo < 30%),
**(4)** comando via WiFi. Desliga por: **(1)** temporizador do usuário, **(2)**
o mesmo botão (prioridade máxima, cancela tudo), **(3)** evento do sensor de
chuva (começou a chover → desliga), **(4)** comando via WiFi. O dispositivo é
autônomo: agenda e config ficam na NVS e continuam rodando sem WiFi.

## Layout do repositório (alvo)

| Caminho     | Conteúdo |
| ----------- | -------- |
| `firmware/` | Projeto ESP-IDF: `main/`, `components/` (hal, sensors, irrigation, ui_oled, config, network, mqtt), `wokwi.toml`, `diagram.json` |
| `host/`     | Python: subscriber MQTT (paho), SQLite, dashboard/agenda web (Flask), CLI |
| `docs/`     | Arquitetura, tópicos MQTT, wiring, calibração, notas de eletrônica |

## Status atual (out/2026)

Fase 3 concluída no firmware: DHT22 lendo temp/humidade a cada 2 s e botão de
irrigação pulsando o relé por 5 s, ambos rodando no Wokwi
(`firmware/main/main.c` — ainda monolítico; a migração para `components/` é o
próximo passo). Veja a tabela "Estado atual vs. alvo" na
[arquitetura](docs/architecture.md).

## Ativação do ESP-IDF — por que é manual

O ESP-IDF está instalado em `~/esp-idf` com toolchain em `~/.espressif`, mas
**não** é carregado automaticamente pelo `~/.bashrc`. Cada shell novo fica
limpo até você ativar.

Para trabalhar no firmware, rode **uma vez por shell novo**:

```bash
idfenv          # alias definido no ~/.bashrc; equivale a:
                # source ~/esp-idf/export.sh
```

Depois, a partir de `firmware/`:

```bash
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
wokwi-cli . --timeout 10000   # simulação Wokwi (não `idf.py wokwi`)
```

## Por que não carregar o ESP-IDF automaticamente

Carregar `~/esp-idf/export.sh` em todo shell causou problemas reais para o
trabalho em outros projetos (backend):

- **`python3`/`pip` ficam sombreados** pelo virtualenv do IDF
  (`~/.espressif/python_env/idf5.5_py3.12_env`) em *todos* os projetos — um
  `pip install` fora de um venv acabava poluindo o ambiente do IDF.
- **Inchaço do `PATH`**: ~15 entradas de tools do ESP (xtensa-esp-elf,
  openocd, esp-gdb, …) são antepostas em todo shell, e qualquer colisão de
  nome vence silenciosamente.
- **Custo de startup**: o banner do export.sh adiciona ~1–2 s a cada terminal
  novo.

A ativação manual via `idfenv` mantém os outros projetos intactos: `PATH`
limpo, seus Python/pip normais e sem banner. Tools e agentes que trabalharem
neste repositório devem rodar `idfenv` (ou `source ~/esp-idf/export.sh`) no
mesmo shell antes de qualquer comando `idf.py`/`wokwi`.
