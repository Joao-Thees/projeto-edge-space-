# projeto-edge-space- — DATASHIELD ORBITAL

O projeto envolve a análise de temperatura de datacenters espaciais, que analisam a temperatura local, e a partir de uma certa temperatura, alertas são gerados via buzzer e leds. Tudo isso com um display LCD exibindo informações em tempo real para a missão.

---

## 📋 Relatório Técnico

### Visão geral

O **DATASHIELD ORBITAL** é um sistema embarcado autônomo, escrito em C++ para Arduino, que monitora em tempo real a **temperatura** e o **nível de carga** de um datacenter espacial. Com base na temperatura lida, o sistema classifica a situação em três estados operacionais (NORMAL, ALERTA e CRÍTICO) e aciona automaticamente atuadores de resfriamento e alarme, exibindo todas as informações em um display LCD e registrando logs pela porta serial.

### Hardware utilizado e mapeamento de pinos

| Componente | Pino(s) | Tipo | Função |
|---|---|---|---|
| Potenciômetro de temperatura | A0 | Entrada analógica | Simula o sensor de temperatura |
| Potenciômetro de carga | A1 | Entrada analógica | Simula o sensor de carga (%) |
| LED Verde | 2 | Saída digital | Indica estado NORMAL |
| LED Amarelo | 3 | Saída digital | Indica estado ALERTA |
| LED Vermelho | 4 | Saída digital | Indica estado CRÍTICO |
| LED Azul | 5 | Saída digital | Indica presença de carga (>0%) |
| Buzzer | 8 | Saída digital | Alarme sonoro |
| Motor (cooler) | 9 | Saída PWM | Ventilação/resfriamento |
| Display LCD 16x2 | 12, 11, 7, 6, 10, 13 | — | RS=12, E=11, D4=7, D5=6, D6=10, D7=13 |

> Biblioteca utilizada: **`LiquidCrystal.h`** (controle do display LCD).

### Parâmetros e limiares de configuração

| Parâmetro | Valor | Descrição |
|---|---|---|
| `TEMP_ALERTA` | 35.0 °C | Limite de entrada no estado ALERTA |
| `TEMP_CRITICO` | 45.0 °C | Limite de entrada no estado CRÍTICO |
| `MOTOR_DESLIGADO` | 0 (PWM) | Motor parado |
| `MOTOR_DEVAGAR` | 120 (PWM) | Velocidade reduzida (estado ALERTA) |
| `MOTOR_RAPIDO` | 255 (PWM) | Velocidade máxima (estado CRÍTICO) |

### Leitura dos sensores

Os valores analógicos (0–1023) são convertidos pela função `lerSensores()`:

- **Temperatura:** mapeada para a faixa de **20,0 °C a 60,0 °C** (`map(raw, 0, 1023, 200, 600) / 10.0`).
- **Carga:** mapeada para a faixa de **0% a 100%**.

### Máquina de estados

O coração do sistema é uma máquina de três estados, atualizada a cada ciclo pela função `atualizarEstado()`. Toda transição de estado é registrada no monitor serial com o marcador `[TRANSICAO]`.

| Estado | Condição (temperatura) | LED | Motor | Buzzer |
|---|---|---|---|---|
| **0 – NORMAL** | `< 35 °C` | 🟢 Verde | Desligado | Silencioso |
| **1 – ALERTA** | `35 °C ≤ T < 45 °C` | 🟡 Amarelo | Devagar (PWM 120) | Bipe intermitente (1500 Hz) |
| **2 – CRÍTICO** | `≥ 45 °C` | 🔴 Vermelho | Rápido (PWM 255) | Contínuo (2000 Hz) |

> O **LED Azul** acende independentemente, sempre que houver carga (`cargaAtual > 0`), nos estados NORMAL e ALERTA.

### Lógica do buzzer

- **ALERTA:** a função `buzzerIntermitente()` produz bipes **não bloqueantes** usando `millis()` — toca por 100 ms a cada 1000 ms, sem travar o restante do programa.
- **CRÍTICO:** som **contínuo** de 2000 Hz até a temperatura cair.

### Display LCD

Atualizado pela função `atualizarLCD()` a cada **500 ms** (controle temporal não bloqueante). Exibe:

- **Linha 1:** `DATASHIELD ORBIT`
- **Linha 2:** temperatura (`T:`) e carga em porcentagem (`C:`), com alinhamento/padding para manter o layout estável.

Há também um **caractere customizado de termômetro** (`charTermometro`) criado no `setup()`.

### Sequência de inicialização (`setup()`)

1. Inicializa a comunicação serial a **9600 bps**.
2. Configura os pinos de saída e zera o motor.
3. Exibe tela de boot (`DATASHIELD ORBIT / INICIALIZANDO`) por 1,5 s.
4. Mostra uma **barra de progresso** animada (`BOOT: ||||||||||`).
5. Confirma `SISTEMA OK! / AUTONOMO ATIVO`.
6. Executa um **teste visual** piscando os 4 LEDs em sequência (3 vezes).
7. Emite um **bipe duplo** de confirmação (1000 Hz + 1500 Hz).

### Loop principal (`loop()`)

A cada ciclo de **100 ms**, executa em sequência:

```
lerSensores()  →  atualizarEstado()  →  controlarAtuadores()  →  atualizarLCD()  →  logSerial()
```

### Log via porta serial

A função `logSerial()` registra a cada **2 segundos** uma linha de telemetria no formato:

```
ESTADO=NORMAL | TEMP=25.0C | CARGA=42% | MOTOR=DESLIGADO
```

### Destaques técnicos

- **Programação não bloqueante:** uso de `millis()` em vez de `delay()` para o buzzer, LCD e logs, permitindo que o sistema continue respondendo durante os alertas.
- **Arquitetura modular:** o `loop()` é dividido em funções de responsabilidade única (leitura, decisão, atuação, exibição, log).
- **Histerese de estados:** comparação entre `estadoAtual` e `estadoAnterior` evita logs repetidos e detecta transições.
- **Feedback multimodal:** o operador recebe informações por três canais simultâneos — visual (LEDs + LCD), sonoro (buzzer) e dados (serial).
