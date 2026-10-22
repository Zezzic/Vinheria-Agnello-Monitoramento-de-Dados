# Vinheria Agnello - Monitoramento Ambiental com ESP32 e MQTT

[![Typing SVG](https://readme-typing-svg.demolab.com?font=Fjalla+One&weight=600&size=45&pause=1000&color=9A00F7&background=C5C5C5&center=true&vCenter=true&width=1100&height=100&lines=Monitoramento+Ambiental;ESP32+%2B+MQTT+%2B+IoT;C%2B%2B)](https://git.io/typing-svg)

![Status](https://img.shields.io/badge/status-Finalizado-green)
![Tipo](https://img.shields.io/badge/tipo-Projeto%20Acad%C3%AAmico-blue)
![Código](https://img.shields.io/badge/código-C%2B%2B-blue)
![Plataforma](https://img.shields.io/badge/plataforma-ESP32-blueviolet)
![Protocolo](https://img.shields.io/badge/protocolo-MQTT-orange)

Projeto acadêmico desenvolvido na FIAP (Engenharia de Software, disciplina Edge Computing & Computer Systems) com foco em monitoramento ambiental aplicado ao armazenamento de vinhos. Um ESP32 realiza a leitura de temperatura, umidade (DHT11) e luminosidade (LDR) da adega, associa um timestamp às leituras e envia os dados por Wi-Fi a um servidor de IoT via protocolo MQTT, no padrão de tópicos do FIWARE. Os dados podem ser lidos, e o LED do dispositivo pode ser controlado, pelo aplicativo MyMQTT.

---

## Visão Geral do Circuito

> **Simulação disponível no Wokwi:**
> [Acessar projeto](https://wokwi.com/projects/478367741497086977)

<!-- COLE AQUI O PRINT DO CIRCUITO NO WOKWI -->

### Arquitetura

```
 [DHT11] --(D4)--\
                  \
 [LDR]  --(D34)----> [ESP32] --Wi-Fi--> [Servidor MQTT 3.135.234.101:1883] <--> [App MyMQTT]
                  /                      (tópicos /TEF/device011/...)         (Subscribe / Publish)
 [LED]  <--(D2)--/
```

1. O ESP32 lê os sensores a cada 2 segundos.
2. Os valores são publicados no servidor MQTT nos tópicos do dispositivo `device011`.
3. O MyMQTT assina os tópicos e exibe os dados.
4. O comando `device011@on|` ou `device011@off|` publicado no MyMQTT é repassado ao ESP32, que acende ou apaga o LED.

---

## Funcionalidades Técnicas

- **Leitura de temperatura e umidade:** sensor DHT11 no pino D4.
- **Leitura de luminosidade:** LDR no pino analógico D34, com média de 10 amostras convertida para porcentagem (0 a 100).
- **Timestamp:** hora sincronizada por NTP (fuso UTC-3), enviada em formato ISO 8601 junto com temperatura, umidade e luminosidade no mesmo JSON.
- **Comunicação Wi-Fi:** ESP32 em modo estação, com reconexão automática.
- **Publicação MQTT:** padrão de tópicos do FIWARE (`/TEF/<device>/attrs`), com JSON completo (temperatura, umidade, luminosidade e timestamp) e tópicos individuais para cada valor.
- **Escrita via MQTT:** o ESP32 assina o tópico de comando e controla o LED conforme os comandos recebidos, informando o estado em um tópico próprio.
- **Alertas legíveis:** mensagem de alerta a cada ciclo, com base nas faixas ideais de conservação do vinho, exibida no Serial Monitor e publicada em tópico próprio para leitura no MyMQTT.
- **Reconexão automática:** Wi-Fi e broker MQTT são reconectados sem intervenção, com avisos no Serial Monitor.
- **Compatível com simulação:** a constante `USAR_WOKWI` alterna entre o Wokwi (DHT22 e Wi-Fi virtual) e a placa real (DHT11 e rede do laboratório).

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

A cada ciclo, o ESP32 compara as leituras com as faixas ideais de conservação, exibe o resultado no Serial Monitor e o publica no tópico de alerta:

| Parâmetro | Faixa ideal | Fora da faixa |
| :--- | :--- | :--- |
| Temperatura | 10 °C a 16 °C (referência: 13 °C) | `Temperatura ALTA.` ou `Temperatura BAIXA.` |
| Umidade | 60% a 80% (referência: 70%) | `Umidade ALTA.` ou `Umidade BAIXA.` |
| Luminosidade | até 30% (penumbra) | `Ambiente muito claro.` |
| Todos dentro da faixa | - | `Condições ideais` |

### Servidor e tópicos MQTT

**Servidor:** `3.135.234.101` | **Porta:** `1883` | **Dispositivo do grupo:** `device011` | **Usuário e senha:** não utilizados

| Tópico | Direção | Conteúdo |
| :--- | :--- | :--- |
| `/TEF/device011/attrs` | ESP32 publica | JSON com temperatura, umidade, luminosidade e timestamp |
| `/TEF/device011/attrs/p` | ESP32 publica | Luminosidade em % (valor isolado) |
| `/TEF/device011/attrs/t` | ESP32 publica | Temperatura em °C |
| `/TEF/device011/attrs/h` | ESP32 publica | Umidade em % |
| `/TEF/device011/attrs/a` | ESP32 publica | Mensagem de alerta (por exemplo, `Condições ideais`) |
| `/TEF/device011/attrs/s` | ESP32 publica | Estado do LED (`on` ou `off`) |
| `/TEF/device011/cmd` | MyMQTT publica | `device011@on\|` ou `device011@off\|` |

Exemplo do JSON publicado em `/TEF/device011/attrs`:

```json
{"temperatura":22.5,"umidade":61.0,"luminosidade":35,"timestamp":"2026-10-22T14:30:05-03:00"}
```

### Configurações do código

| Constante | Valor | Função |
| :--- | :--- | :--- |
| `USAR_WOKWI` | `1` simulação, `0` placa real | Seleciona o sensor (DHT22 ou DHT11) e a rede Wi-Fi |
| `ENVIAR_TIMESTAMP` | `1` ou `0` | Inclui ou não o timestamp no JSON |
| `WIFI_SSID` e `WIFI_PASS` | Rede Wi-Fi | Rede usada na placa real |
| `MQTT_BROKER` e `MQTT_PORT` | `3.135.234.101` e `1883` | Servidor MQTT |
| `LDR_INVERTIDO` | `false` | Use `true` se a luminosidade variar no sentido contrário |

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

  Codigo adaptado dos exemplos fornecidos na disciplina (Prof. Lucas Demetrius
  Augusto, com base no codigo de Fabio Henrique Cabrini), no padrao de topicos
  do FIWARE (/TEF/<device>/attrs, /attrs/p e /cmd).

  O que este codigo faz:
    1) Le temperatura e umidade (DHT11) e luminosidade (LDR)
    2) Conecta no Wi-Fi e sincroniza a hora (NTP) para gerar timestamp
    3) Publica os dados via MQTT no servidor de IoT (device011)
    4) Escuta o topico de comando para ligar/desligar o LED (escrita via MyMQTT)

  Ligacoes (iguais ao slide 10 do professor):
    DHT11 DATA -> GPIO 4  (D4)
    LDR        -> GPIO 34 (D34, entrada analogica)
    LED1       -> GPIO 2  (D2)
    VCC -> 3V3 | GND -> GND
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
           "{\"temperatura\":%.1f,\"umidade\":%.1f,\"luminosidade\":%d,\"timestamp\":\"%s\"}",
           t, h, lum, timestamp().c_str());
#else
  snprintf(json, sizeof(json),
           "{\"temperatura\":%.1f,\"umidade\":%.1f,\"luminosidade\":%d}", t, h, lum);
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
```

---

## Como Simular no Wokwi

Não é necessária nenhuma placa: basta um navegador e o aplicativo MyMQTT.

1. Acesse o link da simulação: [Wokwi - Vinheria Agnello](https://wokwi.com/projects/478367741497086977)
2. Clique no botão verde **Play** (▶) para iniciar.
3. Acompanhe o **Monitor Serial** (parte inferior da tela). A saída esperada é:

```
------Conexao WI-FI------
Conectando-se na rede: Wokwi-GUEST
Aguarde
.....
Conectado com sucesso na rede Wokwi-GUEST
IP obtido: ...
* Tentando se conectar ao Broker MQTT: 3.135.234.101
Conectado com sucesso ao broker MQTT!
Valor da luminosidade: 50
Publicado: {"temperatura":24.0,"umidade":40.0,"luminosidade":50,"timestamp":"2026-10-22T14:30:05-03:00"}
Temperatura: 24.0 C | Umidade: 40.0 %
Alerta: Temperatura ALTA. Umidade BAIXA. Ambiente muito claro.
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
   const char* WIFI_SSID = "FIAP-IOT";
   const char* WIFI_PASS = "SENHA_DA_REDE";
   ```
   Informe a senha da rede Wi-Fi utilizada. A senha não é publicada neste repositório.
5. **Selecione a placa e a porta:** `Tools > Board > esp32 > ESP32 Dev Module` e, em `Tools > Port`, a porta COM do ESP32.
6. **Envie o código** com **Upload**. Se aparecer `Connecting...`, mantenha o botão **BOOT** do ESP32 pressionado até o início da gravação.
7. **Acompanhe a execução** em `Tools > Serial Monitor`, com **115200 baud**. Se necessário, pressione o botão **EN** do ESP32 para reiniciar.

Ao iniciar, o LED pisca por cerca de 2 segundos (teste de saída) e a conexão com o Wi-Fi e com o servidor é exibida no Serial Monitor.

---

## Leitura e Escrita pelo MyMQTT

**Conexão**

1. No MyMQTT, acesse a configuração da conexão.
2. Informe **Broker** `3.135.234.101`, **Porta** `1883`, sem usuário, senha ou SSL/TLS.
3. Conecte.

**Leitura (Subscribe)**

1. Abra a aba **Subscribe**.
2. Assine `/TEF/device011/#`.
3. Em até 2 segundos chegam o JSON (temperatura, umidade, luminosidade e timestamp), a temperatura (`/attrs/t`), a umidade (`/attrs/h`), a luminosidade (`/attrs/p`) e a mensagem de alerta (`/attrs/a`). O estado do LED (`/attrs/s`) é publicado quando o LED muda.

**Escrita (Publish)**

1. Abra a aba **Publish**.
2. Informe o tópico `/TEF/device011/cmd`.
3. Publique `device011@on|` para acender o LED e `device011@off|` para apagá-lo. O estado é confirmado no tópico `/TEF/device011/attrs/s`.

> O servidor é compartilhado com outras equipes, cada uma com o seu dispositivo. Use sempre o tópico completo do `device011` em vez de assinar apenas `#`.

<!-- COLE AQUI O PRINT DO MYMQTT -->

### Resultado esperado

| Ação | Resultado |
| :--- | :--- |
| Ligar o ESP32 | O LED pisca e o Serial mostra Wi-Fi e MQTT conectados |
| Aguardar 2 segundos | O Serial exibe a luminosidade, o JSON publicado, temperatura, umidade e o alerta |
| Subscribe no MyMQTT | Chegam `/attrs` (JSON com timestamp), `/attrs/t`, `/attrs/h`, `/attrs/p` e `/attrs/a`, além de `/attrs/s` quando o LED muda |
| Cobrir o LDR | O valor de luminosidade muda |
| Soprar o DHT11 | Temperatura e umidade sobem |
| Publicar `device011@on\|` / `device011@off\|` | O LED acende / apaga e `/attrs/s` é atualizado |

---

## Solução de Problemas

| Sintoma | Causa provável | Solução |
| :--- | :--- | :--- |
| `Falha ao ler o DHT!` | Fio de DATA solto ou pino incorreto | Conferir a ligação no D4 e a alimentação em 3V3 |
| Serial exibe apenas pontos após `Aguarde` | SSID ou senha incorretos, ou rede 5 GHz | Conferir os dados e usar rede 2,4 GHz. No Wokwi, parar e iniciar a simulação novamente ou atualizar a página |
| `Falha ao conectar no broker.` | Servidor indisponível no momento ou porta 1883 bloqueada pela rede | O código tenta novamente a cada 2 s. Se persistir, testar outra rede |
| Timestamp `sem-hora` | NTP bloqueado na rede | A leitura segue normal. Use outra rede para obter a hora |
| LED não reage ao comando | Mensagem diferente do padrão | Publicar exatamente `device011@on\|` ou `device011@off\|` no tópico `/TEF/device011/cmd` |
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

## Créditos

O código foi adaptado dos exemplos fornecidos na disciplina pelo Prof. Lucas Demetrius Augusto, baseados no código de Fábio Henrique Cabrini, incorporando a leitura do DHT11, do LDR, o timestamp e o controle do LED via MQTT.

---

## Equipe do Projeto

| Nome | RM | Responsabilidade |
| :--- | :--- | :--- |
| Diego Candido Stoianof | 570748 | Desenvolvimento |
| Felipe Moreira Mendes | 570807 | Desenvolvimento |
| Lucas Zezzi Custódio | 571161 | Desenvolvimento |
| Romulo Mendes Sousa | 570620 | Desenvolvimento |
