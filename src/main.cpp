/**
 * @file main.cpp
 * @brief ESP32 Autonomous MP3 Player & Bluetooth A2DP Source
 * @details Decodes MP3 from internal Flash (FFat/LittleFS), applies DSP effects,
 *          streams audio directly to Bluetooth headphones without requiring a phone.
 *          Includes embedded Wi-Fi Web Server for wireless file management.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "FS.h"
#include "FFat.h"
#include "BluetoothA2DPSource.h"
#include "AudioTools.h"
#include "AudioCodecs/CodecMP3Helix.h"

// ================= PINOUT & HARDWARE CONFIG =================
#define PIN_MODE_SELECT     0    // Boot button (GPIO 0): Hold on boot for Wi-Fi Server
#define PIN_BTN_NEXT        12   // Button Next Track (Internal Pullup)
#define PIN_BTN_PREV        13   // Button Prev Track (Internal Pullup)
#define PIN_BTN_PLAY_PAUSE  14   // Button Play/Pause (Internal Pullup)
#define PIN_BTN_EFFECT      27   // Toggle Distortion / Boost (Internal Pullup)

#define DEFAULT_BT_HEADPHONE_NAME "JBL TUNE500BT" // Change to your headphone Bluetooth name

// Operating Modes
enum AppMode {
    MODE_WIFI_SERVER,
    MODE_BT_PLAYER
};

AppMode currentMode = MODE_BT_PLAYER;
WebServer server(80);
BluetoothA2DPSource a2dp_source;

// Audio Streams & Decoders
File mp3File;
MP3DecoderHelix mp3Decoder;
std::vector<String> playlist;
int currentTrackIndex = 0;
bool isPlaying = true;

// DSP & Audio Effects
float volumeGain = 1.2f;        // Digital pre-amp gain
bool enableDistortion = false;   // Overdrive / soft-clipping effect
int distortionDrive = 20000;    // Threshold for saturation

// ================= DSP PROCESSING FUNCTION =================
void processAudioDSP(int16_t* samples, int sampleCount) {
    for (int i = 0; i < sampleCount; i++) {
        float sample = (float)samples[i] * volumeGain;

        // Soft-clipping distortion algorithm
        if (enableDistortion) {
            if (sample > distortionDrive) {
                sample = distortionDrive + (sample - distortionDrive) * 0.25f;
            } else if (sample < -distortionDrive) {
                sample = -distortionDrive + (sample + distortionDrive) * 0.25f;
            }
        }

        // Hard clamp to prevent 16-bit signed integer overflow
        if (sample > 32767.0f) sample = 32767.0f;
        if (sample < -32768.0f) sample = -32768.0f;

        samples[i] = (int16_t)sample;
    }
}

// Forward declarations
void playTrack(int index);
void playNextTrack();
void playPrevTrack();

// ================= A2DP DATA STREAM CALLBACK =================
int32_t a2dp_data_callback(uint8_t *data, int32_t len) {
    if (!isPlaying || !mp3File || !mp3File.available()) {
        if (isPlaying && !playlist.empty()) {
            playNextTrack();
        }
        memset(data, 0, len); // Silence
        return len;
    }

    // Decode MP3 frames into PCM
    size_t bytesDecoded = mp3Decoder.readBytes(data, len);

    if (bytesDecoded == 0) {
        playNextTrack();
        memset(data, 0, len);
        return len;
    }

    // Apply real-time DSP
    processAudioDSP((int16_t*)data, bytesDecoded / 2);

    return bytesDecoded;
}

// ================= PLAYLIST MANAGEMENT =================
void loadPlaylist() {
    playlist.clear();
    File root = FFat.open("/");
    File file = root.openNextFile();
    while (file) {
        String name = String(file.name());
        if (name.endsWith(".mp3") || name.endsWith(".MP3")) {
            if (!name.startsWith("/")) name = "/" + name;
            playlist.push_back(name);
            Serial.printf("[PLAYLIST] Found: %s (%u bytes)\n", name.c_str(), file.size());
        }
        file = root.openNextFile();
    }
    Serial.printf("[PLAYLIST] Total tracks: %d\n", playlist.size());
}

void playTrack(int index) {
    if (playlist.empty()) {
        Serial.println("[PLAYER] No tracks found in flash storage.");
        return;
    }

    if (mp3File) {
        mp3File.close();
    }

    currentTrackIndex = (index + playlist.size()) % playlist.size();
    String currentPath = playlist[currentTrackIndex];
    
    mp3File = FFat.open(currentPath, FILE_READ);
    if (!mp3File) {
        Serial.printf("[PLAYER] Error opening file: %s\n", currentPath.c_str());
        return;
    }

    mp3Decoder.begin();
    mp3Decoder.setStream(&mp3File);
    isPlaying = true;
    Serial.printf("[PLAYER] Now playing [%d/%d]: %s\n", currentTrackIndex + 1, playlist.size(), currentPath.c_str());
}

void playNextTrack() {
    playTrack(currentTrackIndex + 1);
}

void playPrevTrack() {
    playTrack(currentTrackIndex - 1);
}

// ================= EMBEDDED WEB SERVER (WIFI UPLOAD MODE) =================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 MP3 Manager</title>
  <style>
    * { box-sizing: border-box; }
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; background: #0f172a; color: #f8fafc; padding: 20px; display: flex; justify-content: center; margin: 0; }
    .card { background: #1e293b; border-radius: 16px; padding: 24px; width: 100%; max-width: 480px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); border: 1px solid #334155; }
    h1 { font-size: 1.5rem; margin-top: 0; color: #38bdf8; display: flex; align-items: center; gap: 8px; }
    .stats { background: #0f172a; padding: 12px; border-radius: 8px; font-size: 0.9rem; margin-bottom: 20px; color: #94a3b8; }
    .stats b { color: #f1f5f9; }
    .upload-box { border: 2px dashed #475569; border-radius: 12px; padding: 20px; text-align: center; margin-bottom: 24px; transition: border-color 0.2s; }
    .upload-box:hover { border-color: #38bdf8; }
    input[type=file] { margin: 10px 0; color: #94a3b8; width: 100%; }
    button { background: #0284c7; color: white; border: none; padding: 12px 20px; border-radius: 8px; font-weight: 600; cursor: pointer; width: 100%; font-size: 1rem; transition: background 0.2s; }
    button:hover { background: #0369a1; }
    ul { list-style: none; padding: 0; margin: 0; }
    li { background: #0f172a; margin-bottom: 8px; padding: 12px; border-radius: 8px; display: flex; justify-content: space-between; align-items: center; }
    .track-name { word-break: break-all; font-size: 0.95rem; }
    .del-btn { color: #f43f5e; text-decoration: none; font-weight: bold; font-size: 0.85rem; padding: 6px 12px; border-radius: 6px; background: rgba(244,63,94,0.1); }
    .del-btn:hover { background: rgba(244,63,94,0.2); }
  </style>
</head>
<body>
  <div class="card">
    <h1>🎵 ESP32 MP3 Manager</h1>
    <div class="stats">
      Espaço Livre: <b>%FREE_SPACE% MB</b> / Total: <b>%TOTAL_SPACE% MB</b>
    </div>
    <form class="upload-box" method="POST" action="/upload" enctype="multipart/form-data">
      <input type="file" name="file" accept=".mp3" required><br>
      <button type="submit">Enviar Música (.mp3)</button>
    </form>
    <h3>Músicas Gravadas</h3>
    <ul>%TRACK_LIST%</ul>
  </div>
</body>
</html>
)rawliteral";

void startWiFiServer() {
    Serial.println("[WIFI] Starting SoftAP mode...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-MP3-Manager", "12345678");

    IPAddress IP = WiFi.softAPIP();
    Serial.print("[WIFI] Access Point ready. IP address: http://");
    Serial.println(IP);

    server.on("/", HTTP_GET, []() {
        size_t total = FFat.totalBytes();
        size_t free = total - FFat.usedBytes();
        float freeMB = (float)free / (1024.0f * 1024.0f);
        float totalMB = (float)total / (1024.0f * 1024.0f);

        String trackListHtml = "";
        for (const auto &track : playlist) {
            trackListHtml += "<li><span class='track-name'>" + track + "</span>";
            trackListHtml += "<a class='del-btn' href='/delete?f=" + track + "'>Excluir</a></li>";
        }
        if (playlist.empty()) {
            trackListHtml = "<li style='color: #64748b;'>Nenhuma música gravada ainda.</li>";
        }

        String page = FPSTR(INDEX_HTML);
        page.replace("%FREE_SPACE%", String(freeMB, 2));
        page.replace("%TOTAL_SPACE%", String(totalMB, 2));
        page.replace("%TRACK_LIST%", trackListHtml);

        server.send(200, "text/html", page);
    });

    server.on("/delete", HTTP_GET, []() {
        if (server.hasArg("f")) {
            String filename = server.arg("f");
            FFat.remove(filename);
            loadPlaylist();
        }
        server.sendHeader("Location", "/");
        server.send(303);
    });

    server.on("/upload", HTTP_POST, []() {
        server.sendHeader("Location", "/");
        server.send(303);
    }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            String filename = upload.filename;
            if (!filename.startsWith("/")) filename = "/" + filename;
            Serial.printf("[UPLOAD] Starting: %s\n", filename.c_str());
            FFat.remove(filename);
            mp3File = FFat.open(filename, FILE_WRITE);
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (mp3File) {
                mp3File.write(upload.buf, upload.currentSize);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (mp3File) {
                mp3File.close();
                Serial.printf("[UPLOAD] Success: %u bytes\n", upload.totalSize);
            }
            loadPlaylist();
        }
    });

    server.begin();
}

// ================= HARDWARE BUTTON HANDLERS =================
void handleButtons() {
    static unsigned long lastDebounce = 0;
    if (millis() - lastDebounce < 250) return;

    if (digitalRead(PIN_BTN_NEXT) == LOW) {
        Serial.println("[BTN] Next Track");
        playNextTrack();
        lastDebounce = millis();
    } else if (digitalRead(PIN_BTN_PREV) == LOW) {
        Serial.println("[BTN] Prev Track");
        playPrevTrack();
        lastDebounce = millis();
    } else if (digitalRead(PIN_BTN_PLAY_PAUSE) == LOW) {
        isPlaying = !isPlaying;
        Serial.printf("[BTN] Play/Pause: %s\n", isPlaying ? "PLAYING" : "PAUSED");
        lastDebounce = millis();
    } else if (digitalRead(PIN_BTN_EFFECT) == LOW) {
        enableDistortion = !enableDistortion;
        Serial.printf("[BTN] Distortion Effect: %s\n", enableDistortion ? "ON" : "OFF");
        lastDebounce = millis();
    }
}

// ================= SETUP =================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== ESP32 Autonomous MP3 Player ===");

    pinMode(PIN_MODE_SELECT, INPUT_PULLUP);
    pinMode(PIN_BTN_NEXT, INPUT_PULLUP);
    pinMode(PIN_BTN_PREV, INPUT_PULLUP);
    pinMode(PIN_BTN_PLAY_PAUSE, INPUT_PULLUP);
    pinMode(PIN_BTN_EFFECT, INPUT_PULLUP);

    // Initialize Flash filesystem
    if (!FFat.begin(true)) {
        Serial.println("[FATAL] Failed to mount FFat storage partition!");
        return;
    }

    loadPlaylist();

    // Check startup mode (Hold BOOT / Mode pin to start Web Server)
    if (digitalRead(PIN_MODE_SELECT) == LOW || playlist.empty()) {
        currentMode = MODE_WIFI_SERVER;
        startWiFiServer();
    } else {
        currentMode = MODE_BT_PLAYER;
        Serial.println("[PLAYER] Starting Bluetooth A2DP Source...");
        WiFi.mode(WIFI_OFF); // Turn off Wi-Fi to save battery and reduce 2.4GHz interference
        
        a2dp_source.set_data_callback(a2dp_data_callback);
        a2dp_source.start(DEFAULT_BT_HEADPHONE_NAME);

        playTrack(0);
    }
}

// ================= MAIN LOOP =================
void loop() {
    if (currentMode == MODE_WIFI_SERVER) {
        server.handleClient();
    } else {
        handleButtons();
    }
    delay(10);
}
