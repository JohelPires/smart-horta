# Referência MQTT — smart-horta

Contrato de tópicos e payloads entre o **firmware (ESP32)** e o **host**
(subscriber + dashboard web). Qualquer mudança aqui é *breaking change* —
atualize firmware, host e este documento juntos.

- **Base**: `horta/<device_id>/...` — `<device_id>` default `horta-01`
  (configurável; derivado do MAC no reset de fábrica).
- **Formato**: JSON UTF-8, com campo `v` para versão do schema.
- **QoS**: telemetria = 0; eventos/comandos/acks = 1.
- **Retained**: `status` e `available` são retained; o resto não.

## Resumo dos tópicos

| Tópico | Direção | QoS | Retained | Conteúdo |
|---|---|---|---|---|
| `horta/<id>/telemetry` | ESP32 → host | 0 | não | leituras dos sensores + estado da válvula |
| `horta/<id>/status` | ESP32 → host | 1 | **sim** | snapshot vivo (uptime, wifi, versão do fw, config) |
| `horta/<id>/event/rain` | ESP32 → host | 1 | não | início/fim de chuva |
| `horta/<id>/event/irrigation` | ESP32 → host | 1 | não | válvula aberta/fechada + fonte |
| `horta/<id>/event/alert` | ESP32 → host | 1 | não | alertas (reservatório baixo, falha de sensor) |
| `horta/<id>/available` | ESP32 → host | 1 | **sim** | last-will: `online` / `offline` |
| `horta/<id>/cmd/irrigation` | host → ESP32 | 1 | não | abrir/fechar válvula manualmente |
| `horta/<id>/cmd/schedule` | host → ESP32 | 1 | não | definir agenda semanal |
| `horta/<id>/cmd/config` | host → ESP32 | 1 | não | definir limiares/durações |
| `horta/<id>/cmd/get` | host → ESP32 | 1 | não | pedir status/config imediatamente |
| `horta/<id>/ack` | ESP32 → host | 1 | não | resposta de cada comando (`req_id`) |

## 1. Telemetria — `horta/<id>/telemetry`

Periódica (default 10 s, configurável); publicada também imediatamente após
qualquer evento.

```json
{
  "v": 1,
  "ts": 1728123456,
  "temp_c": 24.3,
  "hum_air_pct": 61.5,
  "soil_pct": 38.2,
  "rain": 1,
  "rain_mm": 0.4,
  "water_level_pct": 74,
  "valve": 1,
  "valve_src": "cmd",
  "valve_remaining_s": 212,
  "rssi_dbm": -58,
  "fw": "0.1.0"
}
```

| Campo | Tipo | Nota |
|---|---|---|
| `ts` | int | epoch do ESP32; `0` antes do SNTP sincronizar |
| `valve` | 0/1 | estado atual da válvula |
| `valve_src` | string | `btn` · `sched` · `sensor` · `cmd` · `-` (fechada) |
| `valve_remaining_s` | int | contagem regressiva do timer; `0` se fechada |
| `rain` | 0/1 | binário do sensor de chuva (falha/dry) |
| `rain_mm` | float | 0 = sem chuva; preenchido só com sensor analógico calibrado |
| `water_level_pct` | int | 0–100 via bóia (0/100) ou sensor analógico |

## 2. Snapshot vivo — `horta/<id>/status` (retained)

```json
{
  "v": 1,
  "ts": 1728123456,
  "device_id": "horta-01",
  "up_s": 86200,
  "wifi": "ok",
  "sntp": "ok",
  "fw": "0.1.0",
  "heap_min": 18214,
  "errors": [],
  "schedule": [
    {"days": [1, 3, 6], "time": "06:00", "duration_s": 300, "enabled": true}
  ],
  "config": {
    "max_run_s": 300,
    "soil_trig_pct": 30,
    "air_hum_trig_pct": 40,
    "rain_skip_h": 12,
    "telemetry_s": 10
  }
}
```

Exemplos de `errors`: `"dht"`, `"adc"`, `"nvs"`, `"sntp"`. Falha grave fecha
a válvula; falha de sensor só desativa o gatilho automático correspondente.

## 3. Eventos — `horta/<id>/event/...`

```json
// event/rain
{"v": 1, "ts": 1728123456, "state": "start", "intensity": "light"}
```

`state`: `start` (chuva começou — fecha válvula) / `stop` (parou).
`intensity`: `drizzle` · `light` · `heavy` (faixas do sensor analógico).

```json
// event/irrigation
{"v": 1, "ts": 1728123456, "state": "on", "source": "btn", "duration_s": 300}
{"v": 1, "ts": 1728123500, "state": "off", "reason": "rain"}
```

`source`: `btn` · `sched` · `sensor` · `cmd`.
`reason`: `timer` · `button` · `rain` · `cmd` · `fault` · `reservoir_low`.

```json
// event/alert
{"v": 1, "ts": 1728123456, "type": "reservoir_low", "msg": "boia baixa"}
```

## 4. Last-will — `horta/<id>/available` (retained)

- Ao conectar ao broker o firmware publica `online` (retained).
- A *last-will message* do cliente é `offline`: o broker a publica
  automaticamente se o ESP32 desconectar sem avisar.

## 5. Comandos — `horta/<id>/cmd/...`

Todo comando tem `req_id` (string única, gerada pelo host). O firmware
responde em `horta/<id>/ack`:

```json
{"v": 1, "req_id": "a1b2", "cmd": "irrigation", "ok": true}
{"v": 1, "req_id": "a1b3", "cmd": "schedule", "ok": false, "error": "invalid_duration"}
```

- O host espera o ack com timeout de ~3 s; sem ack → marca o comando como
  falho na UI.
- Comandos não são retained: se o ESP32 estiver offline, o host avisa o
  usuário (via `available`) em vez de enfileirar.

### 5.1 `cmd/irrigation` — manual

```json
{"v": 1, "req_id": "a1b2", "action": "on",  "duration_s": 300}
{"v": 1, "req_id": "a1b3", "action": "off"}
```

- `duration_s` é limitado por `max_run_s` (fail-safe); default =
  `max_run_s` do config.
- `on` **durante chuva é aceito** (intenção explícita do usuário), mas a
  sessão fica limitada ao `max_run_s`.

### 5.2 `cmd/schedule` — agenda semanal

Substitui a agenda inteira (operação "set" atômica).

```json
{
  "v": 1,
  "req_id": "a1b4",
  "entries": [
    {"days": [1, 3, 6], "time": "06:00", "duration_s": 300, "enabled": true},
    {"days": [0],       "time": "07:30", "duration_s": 600, "enabled": true}
  ]
}
```

- `days`: 0=domingo … 6=sábado (convenção `date.weekday()` do Python).
- Máximo 32 entradas; `time` no formato `HH:MM` (horário local configurado
  no host).
- `duration_s` opcional; ausente = `max_run_s`.
- A agenda **não roda** enquanto `sntp != ok` (hora desconhecida) — o OLED
  mostra "aguardando hora".

### 5.3 `cmd/config` — limiares e parâmetros

```json
{
  "v": 1,
  "req_id": "a1b5",
  "max_run_s": 300,
  "soil_trig_pct": 30,
  "air_hum_trig_pct": 40,
  "rain_skip_h": 12,
  "telemetry_s": 10
}
```

Todos os campos são opcionais: só o que vier no payload é alterado.
Semântica do gatilho automático T3 (limiar):

- `soil_trig_pct > 0` → irriga quando a humidade do solo cair abaixo do valor;
- `air_hum_trig_pct > 0` → irriga quando a humidade do ar cair abaixo;
- `0` desativa aquele gatilho;
- histerese: após qualquer rega um *cooldown* mínimo evita religar em cascata.

### 5.4 `cmd/get` — pedir dados imediatamente

```json
{"v": 1, "req_id": "a1b6", "what": "status"}
```

`what`: `status` (re-publica `/status`) ou `config`.

## 6. Segurança

- **Rede local**: Mosquitto escutando apenas na LAN, sem autenticação;
  os dados não atravessam a internet.
- **Acesso fora de casa (futuro)**: broker em VPS com TLS na porta 8883 +
  usuário/senha por dispositivo.
- O firmware só executa comandos de `horta/<seu id>/cmd/#` (filtro no
  client MQTT do firmware) e ignora tópicos de outros dispositivos.
- O host assina com wildcard (ex.: `horta/+/telemetry`) para suportar
  vários dispositivos na mesma rede.

---

*Última atualização: 2026-10-05.*
