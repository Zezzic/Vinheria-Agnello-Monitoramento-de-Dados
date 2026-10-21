/*
  FIAP - Engenharia de Software - Edge Computing & Computer Systems
  Checkpoint 05 - O Caso da Vinheria Agnello (IoT)
  Grupo: DevX
  Integrantes:
    Diego Candido Stoianof / RM: 570748
    Felipe Moreira Mendes  / RM: 570807
    Lucas Zezzi Custodio   / RM: 571161
    Romulo Mendes Sousa    / RM: 570620

  O que este codigo faz:
    1) Le temperatura e umidade (DHT11) e luminosidade (LDR)
    2) Conecta no Wi-Fi e sincroniza a hora (NTP) para gerar timestamp
    3) Publica os dados via MQTT (topicos individuais + JSON completo)
    4) Escuta o topico de comando para ligar/desligar o LED (escrita via MyMQTT)
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <time.h>

#define USAR_WOKWI 1

#if USAR_WOKWI
  const char* WIFI_SSID = "Wokwi-GUEST";
  const char* WIFI_PASS = "";
  #define DHTTYPE DHT22
#else
  const char* WIFI_SSID = "NOME_DA_REDE_WIFI";
  const char* WIFI_PASS = "SENHA_DA_REDE";
  #define DHTTYPE DHT11
#endif

const char*    MQTT_BROKER = "3.135.234.101";
const uint16_t MQTT_PORT   = 1883;

const String BASE         = "fiap/devx/vinheria-agnello/adega01";
const String T_TEMP       = BASE + "/temperatura";
const String T_UMID       = BASE + "/umidade";
const String T_LUM        = BASE + "/luminosidade";
const String T_JSON       = BASE + "/dados";
const String T_ALERTA     = BASE + "/alerta";
const String T_LED_CMD    = BASE + "/led/cmd";
const String T_LED_STATE  = BASE + "/led/estado";

const int PIN_DHT = 4;
const int PIN_LDR = 34;
const int PIN_LED = 2;

const bool LDR_INVERTIDO = false;

const unsigned long INTERVALO_MS = 5000;

DHT dht(PIN_DHT, DHTTYPE);
WiFiClient espClient;
PubSubClient mqtt(espClient);

unsigned long ultimaLeitura = 0;
unsigned long ultimaTentativaMqtt = 0;

void conectaWiFi() {
  Serial.print("Conectando ao Wi-Fi ");
  Serial.print(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.print("\nWi-Fi conectado. IP: ");
  Serial.println(WiFi.localIP());
}

String timestamp() {
  struct tm t;
  if (!getLocalTime(&t, 2000)) return "sem-hora";
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S-03:00", &t);
  return String(buf);
}

void setLed(bool ligado) {
  digitalWrite(PIN_LED, ligado ? HIGH : LOW);
  mqtt.publish(T_LED_STATE.c_str(), ligado ? "on" : "off", true);
  Serial.println(ligado ? "LED ligado via MQTT" : "LED desligado via MQTT");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  msg.toLowerCase();

  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  if (String(topic) == T_LED_CMD) {
    if (msg == "on" || msg == "1" || msg == "ligar")          setLed(true);
    else if (msg == "off" || msg == "0" || msg == "desligar") setLed(false);
  }
}

bool reconectaMqtt() {
  String clientId = "esp32-agnello-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  Serial.print("Conectando ao broker MQTT... ");
  if (mqtt.connect(clientId.c_str())) {
    Serial.println("conectado!");
    mqtt.subscribe(T_LED_CMD.c_str());
    mqtt.publish(T_ALERTA.c_str(), "Sistema online", true);
    return true;
  }
  Serial.println("nao foi possivel conectar ao servidor. Tentando de novo em 5 segundos...");
  return false;
}

int leLuminosidadeRaw() {
  long soma = 0;
  for (int i = 0; i < 10; i++) {
    soma += analogRead(PIN_LDR);
    delay(5);
  }
  return soma / 10;
}

void publicaLeituras() {
  float temp = dht.readTemperature();
  float umid = dht.readHumidity();

  if (isnan(temp) || isnan(umid)) {
    Serial.println("Falha ao ler o DHT. Verifique a ligacao no D4.");
    mqtt.publish(T_ALERTA.c_str(), "Falha na leitura do DHT");
    return;
  }

  int raw = leLuminosidadeRaw();
  int lum = map(raw, 0, 4095, 0, 100);
  if (LDR_INVERTIDO) lum = 100 - lum;

  String ts = timestamp();

  mqtt.publish(T_TEMP.c_str(), String(temp, 1).c_str());
  mqtt.publish(T_UMID.c_str(), String(umid, 1).c_str());
  mqtt.publish(T_LUM.c_str(),  String(lum).c_str());

  char json[256];
  snprintf(json, sizeof(json),
           "{\"timestamp\":\"%s\",\"temperatura\":%.1f,\"umidade\":%.1f,"
           "\"luminosidade\":%d,\"luminosidade_raw\":%d}",
           ts.c_str(), temp, umid, lum, raw);
  mqtt.publish(T_JSON.c_str(), json);

  String alerta = "";
  if (temp > 16.0 || temp < 10.0) alerta += "Temperatura fora de 10-16C. ";
  if (umid < 60.0 || umid > 80.0) alerta += "Umidade fora de 60-80%. ";
  if (lum > 30)                   alerta += "Ambiente muito claro. ";
  if (alerta == "") alerta = "Condicoes ideais";
  mqtt.publish(T_ALERTA.c_str(), alerta.c_str());

  Serial.println(json);
  Serial.println("Alerta: " + alerta);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  analogReadResolution(12);

  dht.begin();
  conectaWiFi();

  configTime(-3 * 3600, 0, "pool.ntp.org", "time.google.com");

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(callback);
  mqtt.setBufferSize(512);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) conectaWiFi();

  if (!mqtt.connected()) {
    if (millis() - ultimaTentativaMqtt > 5000) {
      ultimaTentativaMqtt = millis();
      reconectaMqtt();
    }
  } else {
    mqtt.loop();
    if (millis() - ultimaLeitura >= INTERVALO_MS) {
      ultimaLeitura = millis();
      publicaLeituras();
    }
  }
}
