# 🎧 ESP32 Autonomous MP3 Player & Bluetooth Streamer

Projeto de reprodutor de MP3 autônomo baseado no **ESP32-WROVER**, projetado para prática de esportes e corridas ao ar livre sem a necessidade de levar o celular.

---

## ✨ Recursos

* 📡 **Transmissão Direta Bluetooth A2DP:** Conecta direto em fones de ouvido Bluetooth (SBC codec).
* ⚡ **Streaming & Decodificação em Tempo Real:** Decodificação de arquivos `.mp3` usando a biblioteca otimizada Helix em ponto fixo (zero stuttering).
* 💾 **Sem Necessidade de Cartão SD Mecânico:** Salva as músicas diretamente na memória Flash interna via partição `FFat` de alta velocidade.
* 🌐 **Servidor Web Wi-Fi Embutido:** Carregamento sem fio de músicas direto pelo navegador do celular ou PC.
* 🎛️ **DSP & Efeitos de Áudio:** Pré-amplificação digital e efeito de saturação/overdrive em tempo real.
* 🔋 **Otimizado para Bateria:** Desliga o rádio Wi-Fi durante a reprodução para estender a autonomia.

---

## 🚀 Como Começar

1. Clone o repositório:
   ```bash
   git clone https://github.com/CaioHVectorA/mp3.git
   ```
2. Configure o nome do seu fone Bluetooth em [`src/main.cpp`](file:///home/usuario/develop/mp3/src/main.cpp):
   ```cpp
   #define DEFAULT_BT_HEADPHONE_NAME "SeuFoneBluetooth"
   ```
3. Compile e envie para o seu ESP32-WROVER usando PlatformIO ou Arduino IDE.

---

## 📖 Documentação Detalhada

* 🔌 **[Tabela de Conexões e Pinout](docs/connections.md)**
* 🧠 **[Documentação Técnica e Algoritmos DSP](docs/README.md)**

---

## 📄 Licença

Distribuído sob a licença MIT.
