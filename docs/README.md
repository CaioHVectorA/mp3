# 📚 Documentação Técnica - ESP32 Autonomous MP3 Player

Esta documentação explica a arquitetura de software, pipeline de processamento de sinal (DSP), streaming Bluetooth A2DP e sistema de arquivos.

---

## 📑 Sumário

1. [Visão Geral e Arquitetura](#visão-geral-e-arquitetura)
2. [Como o Áudio é Transmitido (Streaming vs Chunks)](#como-o-áudio-é-transmitido)
3. [Pipeline de Áudio e Algoritmo DSP](#pipeline-de-áudio-e-algoritmo-dsp)
4. [Tabela de Conexões de Hardware](connections.md)
5. [Modos de Operação (Wi-Fi vs Player)](#modos-de-operação)
6. [Guia de Compilação e Gravação](#guia-de-compilação)

---

## 🧠 Visão Geral e Arquitetura

O projeto transforma um **ESP32-WROVER** em um tocador de música autônomo e portátil, ideal para atividades físicas e segurança urbana.

```
                      MODO EM CASA (Wi-Fi)
Celular / PC ──────► Web Server (HTTP 192.168.4.1) ──────► Flash FFat (.mp3)

                      MODO CORRIDA (Bluetooth)
Flash FFat (.mp3) ──► Helix Decoder (PCM) ──► DSP (FX/Gain) ──► Bluetooth A2DP ──► Fone BT
```

---

## 🎵 Como o Áudio é Transmitido

### O problema do array estático de chunks:
* Áudio não compactado em PCM (16-bit 44.1kHz estéreo) consome cerca de **10,5 MB por minuto**.
* Gravar arrays de áudio cru diretamente na memória esgota a capacidade do microcontrolador em poucos segundos.

### A solução adotada (Streaming em Tempo Real):
1. **Armazenamento:** O arquivo permanece compactado em `.mp3` na partição Flash `FFat` (~1MB por minuto a 128kbps).
2. **Leitura sob Demanda:** O ESP32 lê pequenos blocos de 512 a 1024 bytes conforme o Bluetooth solicita dados.
3. **Decodificação Helix MP3:** O decodificador em ponto fixo converte os quadros de MP3 para amostras PCM de 16 bits diretamente na memória RAM em microssegundos.
4. **Callback A2DP:** A biblioteca `ESP32-A2DP` envia o fluxo de áudio continuamente através do codec SBC para o fone pareado.

---

## 🎛️ Pipeline de Áudio e Algoritmo DSP

Antes de as amostras PCM serem empacotadas para o Bluetooth, elas passam pela função `processAudioDSP()`:

### 1. Pré-amplificação Digital (Ganho Extra):
$$S_{\text{gain}} = S_{\text{in}} \times G$$
Onde $G$ é o multiplicador de ganho (ex: $1.2$ a $2.0$).

### 2. Saturação Suave (Soft-Clipping / Overdrive):
Para evitar estalos e distorções ásperas causadas por clipping digital duro, o algoritmo aplica uma compressão cúbica/linear nas cristas das ondas:
```cpp
if (sample > drive) {
    sample = drive + (sample - drive) * 0.25f;
} else if (sample < -drive) {
    sample = -drive + (sample + drive) * 0.25f;
}
```

---

## 🔄 Modos de Operação

### 1. Modo Servidor Wi-Fi (Em Casa / Gerenciamento):
* **Como ativar:** Ligue o ESP32 segurando o botão **BOOT (GPIO 0)** ou caso não haja nenhuma música na memória.
* **Rede gerada:** `ESP32-MP3-Manager` (Senha: `12345678`).
* **Interface:** Abra `http://192.168.4.1` no navegador.
* **Funções:** Upload de faixas `.mp3`, visualização de espaço livre e exclusão de músicas.

### 2. Modo Player Bluetooth (Na Corrida):
* **Como ativar:** Ligue o dispositivo normalmente.
* **Comportamento:** Desliga o rádio Wi-Fi para poupar bateria e eliminar interferências de 2.4GHz, inicializa o perfil Bluetooth A2DP Source e conecta no fone configurado.

---

## 🛠️ Guia de Compilação

### Via PlatformIO (Recomendado):
```bash
pio run -t upload
```

### Via Arduino IDE:
1. Instale as bibliotecas:
   * `ESP32-A2DP` (Phil Schatzmann)
   * `arduino-audio-tools`
   * `arduino-libhelix`
2. Selecione a placa: **ESP32 Wrover Module**.
3. Em *Partition Scheme*, selecione uma opção com **Large FAT/SPIFFS** ou use o `partitions.csv` fornecido.
4. Habilite **PSRAM: Enabled**.
