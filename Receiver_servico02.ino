#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>

// ---------------- MATRIZ 8x8 ----------------
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 1
#define CS_PIN 15   // Pino CS do MAX7219

MD_Parola display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// ---------------- LEDs ----------------
#define LED_VERDE 25
#define LED_VERMELHO 26

// ---------------- STRUCT ----------------
typedef struct struct_message {
  float nivel;
  float temperatura;
  float umidade;
  int luminosidade;
  int presenca;
  unsigned long timestamp;
} struct_message;

struct_message dados;

// Texto para o letreiro
String mensagem_scroll = "Aguardando...";


// ============================================================
// CALLBACK DE RECEPÇÃO
// ============================================================
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&dados, incomingData, sizeof(dados));

  Serial.println("\n📩 Pacote recebido!");
  Serial.printf("Distancia: %.1f cm\n", dados.nivel);
  Serial.printf("Temp: %.1f°C\n", dados.temperatura);
  Serial.printf("Umid: %.1f%%\n", dados.umidade);
  Serial.printf("Luz: %d\n", dados.luminosidade);
  Serial.printf("Presença: %d\n", dados.presenca);

  // -------- mensagem do letreiro ----------
  mensagem_scroll =
    "Dist: " + String(dados.nivel, 0) + " cm | "
    "T: " + String(dados.temperatura, 1) + "C | "
    "U: " + String(dados.umidade, 0) + "% | "
    "L: " + String(dados.luminosidade) + " | "
    "P: " + String(dados.presenca);

  // Restart do letreiro
  display.displayClear();
  display.displayScroll(mensagem_scroll.c_str(), PA_LEFT, PA_SCROLL_LEFT, 50);
}



// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);

  // ---- LEDs ----
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);

  // ---- MATRIZ ----
  display.begin();
  display.setIntensity(4);
  display.displayClear();

  // ---- WIFI/ESPNOW ----
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);  // mesmo canal do sender

  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ Erro ao iniciar ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("✔ Receiver pronto!");
}



// ============================================================
// LOOP
// ============================================================
void loop() {

  // ---- LEDS ----
  if (dados.nivel < 20) {
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_VERMELHO, HIGH);
  } else {
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_VERMELHO, LOW);
  }

  // ---- LETREIRO ----
  if (display.displayAnimate()) {
    display.displayReset();
  }

  delay(20);
}