/**
 * @file test_wroom_mp3_bt.cpp
 * @brief Teste de Decodificação MP3 + Streaming Bluetooth A2DP no ESP32 WROOM-32 (Sem PSRAM)
 * @details Este código toca um áudio MP3 real comprimido (Flash PROGMEM) para o seu fone Bluetooth
 *          e monitora o uso exato de memória RAM interna (Heap) em tempo real.
 */

#include <Arduino.h>
#include "BluetoothA2DPSource.h"
#include "AudioTools.h"
#include "AudioCodecs/CodecMP3Helix.h"
#include "test_mp3.h" // Inclui o áudio MP3 de teste (48 KB)

// ================= COLOQUE O NOME EXATO DO SEU FONE AQUI =================
#define FONE_BLUETOOTH_NOME "JBL TUNE500BT" 

BluetoothA2DPSource a2dp_source;
MP3DecoderHelix mp3Decoder;
MemoryStream mp3Stream(test_mp3_data, test_mp3_len);

// Efeitos de Áudio
float volumeGain = 1.2f;        // Ganho digital
bool enableDistortion = false;   // Ative para testar saturação

// ================= PROCESSAMENTO DSP (EM TEMPO REAL) =================
void processDSP(int16_t* samples, int sampleCount) {
    for (int i = 0; i < sampleCount; i++) {
        float s = (float)samples[i] * volumeGain;
        if (enableDistortion) {
            if (s > 18000.0f) s = 18000.0f + (s - 18000.0f) * 0.2f;
            else if (s < -18000.0f) s = -18000.0f + (s + 18000.0f) * 0.2f;
        }
        if (s > 32767.0f) s = 32767.0f;
        if (s < -32768.0f) s = -32768.0f;
        samples[i] = (int16_t)s;
    }
}

// ================= CALLBACK DO BLUETOOTH A2DP =================
// O Bluetooth chama esta função para pedir mais amostras de som
int32_t a2dp_data_callback(uint8_t *data, int32_t len) {
    // Se o áudio chegar ao fim, volta para o início (Loop contínuo)
    if (!mp3Stream.available()) {
        mp3Stream.seek(0);
        mp3Decoder.begin();
        mp3Decoder.setStream(&mp3Stream);
    }

    // Decodifica o pedaço de MP3 para PCM diretamente no buffer do Bluetooth
    size_t bytesDecoded = mp3Decoder.readBytes(data, len);

    if (bytesDecoded > 0) {
        processDSP((int16_t*)data, bytesDecoded / 2);
    } else {
        memset(data, 0, len);
        return len;
    }

    return bytesDecoded;
}

// ================= MONITOR DE MEMÓRIA =================
void imprimirRelatorioMemoria() {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    uint32_t maxAlloc = ESP.getMaxAllocHeap();

    Serial.printf("[MEMORIA RAM] Livre: %5u KB | Minimo Historico: %5u KB | Maior Bloco: %5u KB\n",
                  freeHeap / 1024, minFreeHeap / 1024, maxAlloc / 1024);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=======================================================");
    Serial.println("   TESTE MP3 + BLUETOOTH A2DP NO ESP32 WROOM-32");
    Serial.println("=======================================================");
    
    Serial.printf("Tamanho do arquivo MP3 na Flash: %u bytes (%u KB)\n", 
                  test_mp3_len, test_mp3_len / 1024);

    Serial.println("\n1. Estado da Memória ANTES de ligar o Bluetooth:");
    imprimirRelatorioMemoria();

    // Inicializa o decodificador Helix MP3
    mp3Decoder.begin();
    mp3Decoder.setStream(&mp3Stream);

    // Inicializa o transmissor Bluetooth A2DP
    Serial.printf("\n2. Conectando ao fone Bluetooth '%s'...\n", FONE_BLUETOOTH_NOME);
    Serial.println("   (Certifique-se de que o fone esta em modo de pareamento)");
    
    a2dp_source.set_data_callback(a2dp_data_callback);
    a2dp_source.start(FONE_BLUETOOTH_NOME);

    Serial.println("\n3. Estado da Memória DEPOIS de ligar o Bluetooth:");
    imprimirRelatorioMemoria();
}

void loop() {
    // Imprime telemetria de memória a cada 2 segundos enquanto a música toca
    static unsigned long lastTime = 0;
    if (millis() - lastTime > 2000) {
        lastTime = millis();
        imprimirRelatorioMemoria();
    }
    delay(50);
}
