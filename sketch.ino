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
    3) Publica os dados via MQTT no servidor de IoT (device011)
    4) Escuta o topico de comando para ligar/desligar o LED (escrita via MyMQTT)
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <time.h>

#define USAR_WOKWI 1
#define ENVIAR_TIMESTAMP 1

#if USAR_WOKWI
  const char* WIFI_SSID = "Wokwi-GUEST";
  const char* WIFI_PASS = "";
  const int   WIFI_CANAL = 6;
  #define DHTTYPE DHT22
#else
  const char* WIFI_SSID = "FIAP-IOT";
  const char* WIFI_PASS = "SENHA_DA_REDE";
  const int   WIFI_CANAL = 0;
  #define DHTTYPE DHT11
#endif

const char* MQTT_BROKER = "3.135.234.101";
const int   MQTT_PORT   = 1883;

const char* TOPICO_SUBSCRIBE    = "/TEF/device011/cmd";
const char* TOPICO_TELEMETRIA   = "/TEF/device011/attrs";
const char* TOPICO_LUMINOSIDADE = "/TEF/device011/attrs/p";
const char* TOPICO_ESTADO_LED   = "/TEF/device011/attrs/s";
const char* TOPICO_TEMPERATURA  = "/TEF/device011/attrs/t";
const char* TOPICO_UMIDADE      = "/TEF/device011/attrs/h";
const char* TOPICO_ALERTA       = "/TEF/device011/attrs/a";

const char* CMD_LIGAR    = "device011@on|";
const char* CMD_DESLIGAR = "device011@off|";

const int PIN_DHT = 4;
const int PIN_LDR = 34;
const int PIN_LED = 2;

const bool LDR_INVERTIDO = false;

const unsigned long INTERVALO_MS = 2000;

DHT dht(PIN_DHT, DHTTYPE);
WiFiClient espClient;
PubSubClient MQTT(espClient);

unsigned long ultimaLeitura = 0;
bool ledLigado = false;

void initSerial() {
  Serial.begin(115200);
}

void initOutput() {
  pinMode(PIN_LED, OUTPUT);
  bool nivel = false;
  for (int i = 0; i <= 10; i++) {
    nivel = !nivel;
    digitalWrite(PIN_LED, nivel);
    delay(200);
  }
  digitalWrite(PIN_LED, LOW);
  ledLigado = false;
}

void reconectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS, WIFI_CANAL);
  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
    if (millis() - inicio > 15000) {
      Serial.println();
      Serial.println("Sem resposta do Wi-Fi. Tentando novamente...");
      WiFi.disconnect();
      delay(500);
      WiFi.begin(WIFI_SSID, WIFI_PASS, WIFI_CANAL);
      inicio = millis();
    }
  }
  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.println(WIFI_SSID);
  Serial.print("IP obtido: ");
  Serial.println(WiFi.localIP());
}

void initWiFi() {
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(WIFI_SSID);
  Serial.println("Aguarde");
  reconectWiFi();
}

String timestamp() {
  time_t agora = time(nullptr);
  if (agora < 1700000000) return "sem-hora";
  struct tm t;
  localtime_r(&agora, &t);
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S-03:00", &t);
  return String(buf);
}

void publicaEstadoLed() {
  MQTT.publish(TOPICO_ESTADO_LED, ledLigado ? "on" : "off");
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  int fim = length;
  while (fim > 0 && (msg[fim - 1] == ' ' || msg[fim - 1] == '\r' || msg[fim - 1] == '\n')) {
    msg[--fim] = '\0';
  }

  Serial.printf("- Comando recebido no tópico %s: %s\n", topic, msg);

  if (strcmp(msg, CMD_LIGAR) == 0) {
    digitalWrite(PIN_LED, HIGH);
    ledLigado = true;
    Serial.println("- Ação: LED LIGADO");
    publicaEstadoLed();
  } else if (strcmp(msg, CMD_DESLIGAR) == 0) {
    digitalWrite(PIN_LED, LOW);
    ledLigado = false;
    Serial.println("- Ação: LED DESLIGADO");
    publicaEstadoLed();
  } else {
    Serial.println("- Ação: comando não reconhecido. Ignorado.");
  }
}

void initMQTT() {
  MQTT.setServer(MQTT_BROKER, MQTT_PORT);
  MQTT.setCallback(mqtt_callback);
  MQTT.setBufferSize(512);
}

void reconnectMQTT() {
  while (!MQTT.connected()) {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(MQTT_BROKER);

    char clientId[32];
    snprintf(clientId, sizeof(clientId), "fiware_011_%04X", (unsigned int)random(0xffff));

    if (MQTT.connect(clientId)) {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.subscribe(TOPICO_SUBSCRIBE);
      publicaEstadoLed();
    } else {
      Serial.println("Falha ao conectar no broker.");
      Serial.println("Haverá nova tentativa de conexão em 2s");
      delay(2000);
    }
  }
}

void verificaConexoes() {
  reconectWiFi();
  if (!MQTT.connected()) reconnectMQTT();
}

int leLuminosidade() {
  long soma = 0;
  for (int i = 0; i < 10; i++) {
    soma += analogRead(PIN_LDR);
    delay(5);
  }
  int valor = map(soma / 10, 0, 4095, 0, 100);
  if (LDR_INVERTIDO) valor = 100 - valor;
  return valor;
}

void publicaLeituras() {
  int lum = leLuminosidade();
  char msgLum[8];
  snprintf(msgLum, sizeof(msgLum), "%d", lum);
  MQTT.publish(TOPICO_LUMINOSIDADE, msgLum);
  Serial.print("Valor da luminosidade: ");
  Serial.println(msgLum);

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("Falha ao ler o DHT! Verifique a ligação no pino D4.");
    return;
  }

  char json[160];
#if ENVIAR_TIMESTAMP
  snprintf(json, sizeof(json),
           "{\"temperatura\":%.1f,\"umidade\":%.1f,\"timestamp\":\"%s\"}",
           t, h, timestamp().c_str());
#else
  snprintf(json, sizeof(json),
           "{\"temperatura\":%.1f,\"umidade\":%.1f}", t, h);
#endif

  if (MQTT.publish(TOPICO_TELEMETRIA, json)) {
    Serial.printf("Publicado: %s\n", json);
  } else {
    Serial.println("Falha ao publicar.");
  }

  char msgT[12];
  char msgH[12];
  snprintf(msgT, sizeof(msgT), "%.1f", t);
  snprintf(msgH, sizeof(msgH), "%.1f", h);
  MQTT.publish(TOPICO_TEMPERATURA, msgT);
  MQTT.publish(TOPICO_UMIDADE, msgH);
  Serial.print("Temperatura: ");
  Serial.print(msgT);
  Serial.print(" C | Umidade: ");
  Serial.print(msgH);
  Serial.println(" %");

  String alerta = "";
  if (t > 16.0)      alerta += "Temperatura ALTA. ";
  else if (t < 10.0) alerta += "Temperatura BAIXA. ";
  if (h > 80.0)      alerta += "Umidade ALTA. ";
  else if (h < 60.0) alerta += "Umidade BAIXA. ";
  if (lum > 30)      alerta += "Ambiente muito claro. ";
  if (alerta == "") alerta = "Condições ideais";
  MQTT.publish(TOPICO_ALERTA, alerta.c_str());
  Serial.println("Alerta: " + alerta);
}

void setup() {
  initSerial();
  dht.begin();
  analogReadResolution(12);
  initOutput();
  initWiFi();
  configTime(-3 * 3600, 0, "pool.ntp.org", "time.google.com");
  initMQTT();
}

void loop() {
  verificaConexoes();
  MQTT.loop();

  unsigned long agora = millis();
  if (agora - ultimaLeitura >= INTERVALO_MS) {
    ultimaLeitura = agora;
    publicaLeituras();
  }
}
