#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <Ultrasonic.h>
#include <HTTPClient.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <DHT.h>

// ======= PINOS =======
#define motionSensorPin 23
#define photoSensorPin 34
#define DHTPIN 4
#define DHTTYPE DHT11
#define pino_trigger 45
#define pino_echo 5

// ======= WIFI + INFLUXDB =======
const char* ssidName = "MAlves ";
const char* ssidPassword = "12345678";

#define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com"
#define INFLUXDB_TOKEN "rtZ6m91E9xaqtgghBjN18w7l_NE74RcMyljLFfBLAPS8hdV_wBnBbT07J4gG5ScpP9n43nLPaYUA-etg_rmB5w=="
#define INFLUXDB_ORG "6d9c24026c2bfd4a"
#define INFLUXDB_BUCKET "Servicos2"

// ======= OBJETOS =======
DHT dht(DHTPIN, DHTTYPE);
Ultrasonic ultrasonic(pino_trigger, pino_echo);
Point sensor("perception");

// ======= MAC destino ESP-NOW =======
uint8_t broadcastAddress[] = {0x94, 0x51, 0xDC, 0x4C, 0x77, 0xAC};

// ======= ESTRUTURA DOS DADOS =======
typedef struct PerceptionLayer {
  float distance;
  float temperature;
  float humidity;
  float lux;
  int motionSensorState;
} PerceptionLayer;

// ======= LEITURA DOS SENSORES =======
PerceptionLayer readPerceptionLayerValues() {
  PerceptionLayer out;

  // Movimento
  out.motionSensorState = digitalRead(motionSensorPin);

  // DHT
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) { t = 0; h = 0; }
  out.temperature = t;
  out.humidity = h;

  // Ultrassom
  float dist = ultrasonic.read(CM);
  if (dist <= 0 || dist > 400) dist = -1;
  out.distance = dist;

  // Fotoresistor
  int foto = analogRead(photoSensorPin);
  float volts = foto * 3.3 / 4095.0;
  if (volts >= 3.29) volts = 3.29;
  float R = 1000 * volts / (3.3 - volts);
  if (R <= 0) R = 1;
  out.lux = pow((50 * 1000 * pow(10, 0.7)) / R, 1.0 / 0.7);

  return out;
}

// ======= INFLUX CLIENT =======
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

// ======= ENVIO AO INFLUXDB =======
bool postPerceptionLayerData(PerceptionLayer p) {
  sensor.clearFields();

  sensor.addField("temperature", p.temperature);
  sensor.addField("humidity", p.humidity);
  sensor.addField("distance", p.distance);
  sensor.addField("lux", p.lux);
  sensor.addField("motion", p.motionSensorState);

  bool ok = client.writePoint(sensor);

  if (!ok) {
    Serial.print("Erro Influx: ");
    Serial.println(client.getLastErrorMessage());
    return false;
  }

  Serial.println("📡 Dados enviados ao Influx!");
  return true;
}

// ======= CALLBACK ESP-NOW (compatível com ESP-IDF 5.x) =======
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("ESP-NOW: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sucesso" : "Falhou");

  // Exibe MAC do peer (já conhecido)
  Serial.print("Destino: ");
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
                broadcastAddress[0], broadcastAddress[1], broadcastAddress[2],
                broadcastAddress[3], broadcastAddress[4], broadcastAddress[5]);
}

// ======= SETUP =======
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(motionSensorPin, INPUT);
  pinMode(photoSensorPin, INPUT);

  dht.begin();

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidName, ssidPassword);
  Serial.print("Conectando ao WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWiFi conectado!");

  // Influx tags fixas
  sensor.addTag("device", "esp32-lab");
  sensor.addTag("location", "sala");

  client.setInsecure();

  if (client.validateConnection()) {
    Serial.println("Conectado ao InfluxDB!");
  } else {
    Serial.println(client.getLastErrorMessage());
  }

  // ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao iniciar ESP-NOW");
    return;
  }

  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peerInfo{};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.println("Peer adicionado.");
  }
}

// ======= LOOP =======
void loop() {
  PerceptionLayer p = readPerceptionLayerValues();

  // Envio ESP-NOW
  esp_now_send(broadcastAddress, (uint8_t*)&p, sizeof(p));

  // Envio InfluxDB
  if (WiFi.status() == WL_CONNECTED) {
    postPerceptionLayerData(p);
  }

  delay(2000);
}
