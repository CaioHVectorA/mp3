# 🔌 Tabela de Conexões e Pinout de Hardware

Este documento detalha o mapa de pinos (pinout), conexões elétricas e especificações de hardware para o **ESP32 Autonomous MP3 Player**.

---

## 📌 Tabela de Conexões (ESP32-WROVER)

| Função | Pino no ESP32 (GPIO) | Tipo | Descrição / Ligação |
| :--- | :--- | :--- | :--- |
| **Modo Wi-Fi / Web Server** | **GPIO 0 (BOOT)** | Entrada (Pull-up interno) | Pressione no boot para abrir o servidor de upload via Wi-Fi |
| **Botão Próxima Faixa** | **GPIO 12** | Entrada (Pull-up interno) | Botão táctil push-button conectado ao GND |
| **Botão Faixa Anterior** | **GPIO 13** | Entrada (Pull-up interno) | Botão táctil push-button conectado ao GND |
| **Botão Play / Pause** | **GPIO 14** | Entrada (Pull-up interno) | Botão táctil push-button conectado ao GND |
| **Botão Efeito / Distorção** | **GPIO 27** | Entrada (Pull-up interno) | Alterna efeito de saturação/DSP ou Boost de volume |
| **Alimentação VCC** | **VIN (5V) / 3.3V** | Alimentação | Conectado à saída do carregador TP4056 ou regulador LDO |
| **GND (Terra Comum)** | **GND** | Alimentação | Terminal negativo comum para todos os botões e bateria |

---

## ⚡ Esquema Elétrico Recomendado

```
                    ┌─────────────────────────┐
                    │       ESP32-WROVER      │
                    │                         │
  [ Botão NEXT ] ───┤ GPIO 12                 │
  [ Botão PREV ] ───┤ GPIO 13                 │
  [ Botão PLAY ] ───┤ GPIO 14                 │
  [ Botão FX   ] ───┤ GPIO 27                 │
                    │                         │
  [ Botão BOOT ] ───┤ GPIO 0                  │
                    │                         │
  [ Bateria (+) ]───┤ VIN (via TP4056/LDO)    │
  [ Bateria (-) ]───┤ GND ────────────────────┴───── (GND comum de todos os botões)
```

> **Nota sobre os botões:** Todos os botões utilizam resistores de **Pull-up interno** configurados via software (`INPUT_PULLUP`). Basta ligar um terminal do botão no pino GPIO do ESP32 e o outro terminal diretamente no pino **GND**.

---

## 🔋 Circuito de Alimentação Portátil (Corrida / Uso Externo)

Para máxima portabilidade sem risco de solavancos durante a corrida:

1. **Bateria:** Bateria LiPo ou Li-Ion 3.7V (ex: 500mAh a 1200mAh 18650 / flat cell).
2. **Carregador com Proteção:** Módulo **TP4056 com proteção de subtensão** (USB-C).
3. **Regulador LDO de Baixo Ruído:** Regulador 3.3V com suporte a picos de corrente de rádio (ex: `HT7333`, `ME6211` ou `AMS1117-3.3`).

---

## ⚠️ Pinos Reservados no ESP32-WROVER (NÃO UTILIZAR)

No ESP32-WROVER, os seguintes pinos são dedicados à **PSRAM integrada** e **Flash SPI** e **NÃO** devem ser conectados a botões ou periféricos:

* **GPIO 6, 7, 8, 9, 10, 11** (Barramento SPI Flash integrado)
* **GPIO 16, 17** (Controle da memória PSRAM interna)
