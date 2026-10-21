# Vinheria Agnello - Monitoramento Ambiental com ESP32 e MQTT

[![Typing SVG](https://readme-typing-svg.demolab.com?font=Fjalla+One&weight=600&size=45&pause=1000&color=9A00F7&background=C5C5C5&center=true&vCenter=true&width=1100&height=100&lines=Monitoramento+Ambiental;ESP32+%2B+MQTT+%2B+IoT;C%2B%2B)](https://git.io/typing-svg)

![Status](https://img.shields.io/badge/status-Finalizado-green)
![Tipo](https://img.shields.io/badge/tipo-Projeto%20Acad%C3%AAmico-blue)
![Código](https://img.shields.io/badge/código-C%2B%2B-blue)
![Plataforma](https://img.shields.io/badge/plataforma-ESP32-blueviolet)
![Protocolo](https://img.shields.io/badge/protocolo-MQTT-orange)

Projeto acadêmico desenvolvido na FIAP (Engenharia de Software, disciplina Edge Computing & Computer Systems) com foco em monitoramento ambiental aplicado ao armazenamento de vinhos. Um ESP32 realiza a leitura de temperatura, umidade (DHT11) e luminosidade (LDR) da adega, associa um timestamp a cada leitura e envia os dados por Wi-Fi a um servidor de IoT via protocolo MQTT. Os dados podem ser lidos, e o LED do dispositivo pode ser controlado, pelo aplicativo MyMQTT.

---

## Visão Geral do Circuito

> **Simulação disponível no Wokwi:**
> [Acessar projeto](https://wokwi.com/projects/478367741497086977)

<img width="888" height="560" alt="image" src="https://github.com/user-attachments/assets/62cdc576-f05f-43da-b046-fe4858683b17" />


### Arquitetura

```
 [DHT11] --(D4)--\
                  \
 [LDR]  --(D34)----> [ESP32] --Wi-Fi--> [Servidor MQTT 3.135.234.101:1883] <--> [App MyMQTT]
                  /                                                          (Subscribe / Publish)
 [LED]  <--(D2)--/
```

1. O ESP32 lê os sensores a cada 5 segundos.
2. Os valores são publicados no servidor MQTT, com timestamp.
3. O MyMQTT assina os tópicos e exibe os dados.
4. Um comando `on` ou `off` publicado no MyMQTT é repassado ao ESP32, que acende ou apaga o LED.

---

## Funcionalidades Técnicas

- **Leitura de temperatura e umidade:** sensor DHT11 no pino D4.
- **Leitura de luminosidade:** LDR no pino analógico D34, com média de 10 amostras convertida para porcentagem.
- **Timestamp em cada leitura:** hora sincronizada por NTP (fuso UTC-3) e enviada em formato ISO 8601.
- **Comunicação Wi-Fi:** ESP32 em modo estação, com reconexão automática.
- **Publicação MQTT:** tópicos individuais (temperatura, umidade, luminosidade) e um JSON completo com timestamp.
- **Escrita via MQTT:** o ESP32 assina o tópico de comando e controla o LED conforme as mensagens `on` e `off`.
- **Alertas automáticos:** tópico de alerta com base nas faixas ideais de conservação do vinho.
- **Controle de tempo sem bloqueios:** uso de `millis()` para o intervalo entre leituras e para as tentativas de reconexão.
- **Compatível com simulação:** a constante `USAR_WOKWI` alterna entre o Wokwi (DHT22 e Wi-Fi virtual) e a placa real (DHT11).

---

## Componentes Utilizados

| Componente | Descrição | Pino do ESP32 |
| :--- | :--- | :--- |
| ESP32 DevKit (ESP32-VROOM-32D) | Microcontrolador com Wi-Fi | - |
| Sensor DHT11 | Temperatura e umidade (na simulação: DHT22) | D4 (GPIO 4) |
| LDR | Sensor de luminosidade analógico | D34 (GPIO 34) |
| LED | Atuador controlado via MQTT (LED1 na placa do laboratório) | D2 (GPIO 2) |
| Resistor 220 Ω | Proteção do LED (necessário apenas na simulação) | - |
| Jumpers | Ligação entre o ESP32 e a placa de montagem | - |

### Ligações

| Cor do fio | Pino do ESP32 | Destino na placa |
| :--- | :--- | :--- |
| Vermelho | 3V3 | VCC |
| Preto | GND | GND |
| Azul | D4 (GPIO 4) | DATA (DHT11) |
| Roxo | D34 (GPIO 34) | LDR |
| Amarelo | D2 (GPIO 2) | LED1 |

---

## Lógica do Sistema

O ESP32 compara cada leitura com as faixas ideais de conservação e publica o resultado no tópico de alerta:

| Parâmetro | Faixa ideal | Fora da faixa |
| :--- | :--- | :--- |
| Temperatura | 10 °C a 16 °C (referência: 13 °C) | `Temperatura fora de 10-16C.` |
| Umidade | 60% a 80% (referência: 70%) | `Umidade fora de 60-80%.` |
| Luminosidade | até 30% (penumbra) | `Ambiente muito claro.` |
| Todos dentro da faixa | - | `Condicoes ideais` |

### Servidor e tópicos MQTT

**Servidor:** `3.135.234.101` | **Porta:** `1883` | **Usuário e senha:** não utilizados

| Tópico | Direção | Conteúdo |
| :--- | :--- | :--- |
| `fiap/devx/vinheria-agnello/adega01/temperatura` | ESP32 publica | Temperatura em °C |
| `fiap/devx/vinheria-agnello/adega01/umidade` | ESP32 publica | Umidade em % |
| `fiap/devx/vinheria-agnello/adega01/luminosidade` | ESP32 publica | Luminosidade em % |
| `fiap/devx/vinheria-agnello/adega01/dados` | ESP32 publica | JSON com timestamp |
| `fiap/devx/vinheria-agnello/adega01/alerta` | ESP32 publica | Texto de alerta |
| `fiap/devx/vinheria-agnello/adega01/led/cmd` | MyMQTT publica | `on` ou `off` |
| `fiap/devx/vinheria-agnello/adega01/led/estado` | ESP32 publica | `on` ou `off` |

Exemplo do JSON publicado em `.../dados`:

```json
{"timestamp":"2026-10-22T14:30:05-03:00","temperatura":22.5,"umidade":61.0,"luminosidade":35,"luminosidade_raw":1433}
```

---

## Código Principal

```cpp
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
```

---

## Como Simular no Wokwi

Não é necessária nenhuma placa: basta um navegador e o aplicativo MyMQTT.

1. Acesse o link da simulação: [Wokwi - Vinheria Agnello](https://wokwi.com/projects/478367741497086977)
2. Clique no botão verde **Play** (▶) para iniciar.
3. Acompanhe o **Monitor Serial** (parte inferior da tela). A saída esperada é:

```
Conectando ao Wi-Fi Wokwi-GUEST...
Wi-Fi conectado. IP: ...
Conectando ao broker MQTT... conectado!
{"timestamp":"2026-10-22T14:30:05-03:00","temperatura":24.0,"umidade":40.0,"luminosidade":50,"luminosidade_raw":2048}
Alerta: ...
```

4. Clique no **DHT22** para variar temperatura e umidade e no **LDR** para variar a luminosidade.
5. Use o MyMQTT conforme a seção [Leitura e Escrita pelo MyMQTT](#leitura-e-escrita-pelo-mymqtt).

> O Wokwi simula o DHT22 em vez do DHT11. Com `USAR_WOKWI` igual a `1`, o código seleciona o sensor e a rede Wi-Fi virtual (`Wokwi-GUEST`) automaticamente.

### Recriar a simulação a partir dos arquivos do repositório

1. Acesse [wokwi.com/projects/new/esp32](https://wokwi.com/projects/new/esp32).
2. Na aba `sketch.ino`, substitua o conteúdo por `sketch/sketch.ino`.
3. Na aba `diagram.json`, substitua o conteúdo por `wokwi/diagram.json`.
4. Adicione as bibliotecas `PubSubClient`, `DHT sensor library` e `Adafruit Unified Sensor` (Library Manager ou `wokwi/libraries.txt`).
5. Clique em **Play** (▶).

---

## Execução na Placa Real (Arduino IDE)

### Requisitos

- ESP32 DevKit e placa de montagem com DHT11, LDR e LED, ligados conforme a tabela de [Ligações](#ligações)
- Cabo micro-USB de dados
- Computador com [Arduino IDE 2.x](https://www.arduino.cc/en/software)
- Rede Wi-Fi 2,4 GHz com acesso à internet

### Passo a passo

1. **Instale o suporte ao ESP32.** Em `File > Preferences`, adicione em *Additional boards manager URLs*: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`. Depois, em `Tools > Board > Boards Manager`, instale **esp32 by Espressif Systems**.
2. **Instale as bibliotecas** em `Sketch > Include Library > Manage Libraries`:
   - `PubSubClient` (Nick O'Leary)
   - `DHT sensor library` (Adafruit)
   - `Adafruit Unified Sensor` (Adafruit)
3. **Obtenha o código.** Clone ou baixe este repositório (`Code > Download ZIP`) e abra `sketch/sketch.ino` na Arduino IDE.
4. **Configure o código.** No início do arquivo, altere:
   ```cpp
   #define USAR_WOKWI 0
   const char* WIFI_SSID = "NOME_DA_REDE_WIFI";
   const char* WIFI_PASS = "SENHA_DA_REDE";
   ```
5. **Selecione a placa e a porta:** `Tools > Board > esp32 > ESP32 Dev Module` e, em `Tools > Port`, a porta COM do ESP32.
6. **Envie o código** com **Upload**. Se aparecer `Connecting...`, mantenha o botão **BOOT** do ESP32 pressionado até o início da gravação.
7. **Acompanhe a execução** em `Tools > Serial Monitor`, com **115200 baud**. Se necessário, pressione o botão **EN** do ESP32 para reiniciar.

---

## Leitura e Escrita pelo MyMQTT

**Conexão**

1. No MyMQTT, acesse a configuração da conexão.
2. Informe **Broker** `3.135.234.101`, **Porta** `1883`, sem usuário, senha ou SSL/TLS.
3. Conecte.

**Leitura (Subscribe)**

1. Abra a aba **Subscribe**.
2. Assine `fiap/devx/vinheria-agnello/adega01/#`.
3. Em até 5 segundos chegam temperatura, umidade, luminosidade, JSON com timestamp e alerta.

**Escrita (Publish)**

1. Abra a aba **Publish**.
2. Informe o tópico `fiap/devx/vinheria-agnello/adega01/led/cmd`.
3. Publique `on` para acender o LED e `off` para apagá-lo. O estado é confirmado no tópico `.../led/estado`.

> O servidor é compartilhado com outras equipes. Use sempre o tópico completo do projeto em vez de assinar apenas `#`.

<img width="788" height="1600" alt="image" src="https://github.com/user-attachments/assets/5823116f-4264-4566-8d71-00174fb80a97" />


### Resultado esperado

| Ação | Resultado |
| :--- | :--- |
| Ligar o ESP32 | Serial mostra Wi-Fi e MQTT conectados |
| Aguardar 5 segundos | Uma linha JSON é impressa e publicada |
| Subscribe no MyMQTT | Os tópicos de dados chegam, com timestamp no JSON |
| Cobrir o LDR | O valor de `luminosidade` muda |
| Soprar o DHT11 | Temperatura e umidade sobem |
| Publicar `on` / `off` | O LED acende / apaga e `led/estado` é atualizado |

---

## Solução de Problemas

| Sintoma | Causa provável | Solução |
| :--- | :--- | :--- |
| `Falha ao ler o DHT` | Fio de DATA solto ou pino incorreto | Conferir a ligação no D4 e a alimentação em 3V3 |
| Serial exibe apenas pontos após `Conectando ao Wi-Fi` | SSID ou senha incorretos, ou rede 5 GHz | Conferir os dados e usar rede 2,4 GHz |
| `nao foi possivel conectar ao servidor` | Servidor indisponível no momento ou porta 1883 bloqueada pela rede | O código tenta novamente a cada 5 s. Se persistir, testar outra rede |
| Timestamp `sem-hora` | NTP bloqueado na rede | Usar outra rede |
| Luminosidade com sentido invertido | LDR da placa opera de forma inversa | Alterar `LDR_INVERTIDO` para `true` |
| Upload preso em `Connecting...` | ESP32 fora do modo de gravação | Manter o botão BOOT pressionado durante o upload |
| Porta COM não aparece | Driver USB ausente ou cabo apenas de carga | Instalar o driver CP210x e usar cabo de dados |
| Erro de compilação em `DHT.h` ou `PubSubClient.h` | Biblioteca não instalada | Instalar conforme o passo 2 da execução na placa real |

---

## Estrutura do Repositório

```
.
├── README.md
├── sketch/
│   └── sketch.ino          Código do ESP32
├── wokwi/
│   ├── diagram.json        Circuito para importar no Wokwi
│   └── libraries.txt       Bibliotecas da simulação
```

---

## Equipe do Projeto

| Nome | RM | Responsabilidade |
| :--- | :--- | :--- |
| Diego Candido Stoianof | 570748 | Desenvolvimento |
| Felipe Moreira Mendes | 570807 | Desenvolvimento |
| Lucas Zezzi Custódio | 571161 | Desenvolvimento |
| Romulo Mendes Sousa | 570620 | Desenvolvimento |
