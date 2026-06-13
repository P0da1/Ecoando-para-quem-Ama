#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <Wire.h>
#include <MPU6050.h>
#include <math.h>

// Pinos
#define BUZZER 2
#define LED    4
#define BOTAO  5

MPU6050 mpu;

// Configurações
const float LIMIAR_SUP = 2.5; // g - impacto forte
const float LIMIAR_INF = 0.5; // g - possível queda livre
bool alarmeAtivo = false;

//Comunicação
const char* WIFI_SSID     = "S20"; //Nome da Rede WiFi
const char* WIFI_PASSWORD = "87654321"; //Senha da Rede WiFi

const char* TWILIO_ACCOUNT_SID = "AC0a8a3de2fa2f32ab8e02d61d411dada9";
const char* TWILIO_AUTH_TOKEN  = "69b6a84315b9d59f02ac2a5fdf11be98";

//Dados Pessoais
const char* contact = "5511997773239";
const char* name = "José";

// NÚMERO DO SANDBOX
String TWILIO_WHATSAPP_FROM = "whatsapp:%2B14155238886"; // %2B é '+'
String YOUR_WHATSAPP_TO     = "whatsapp:%2B" + String(contact);
// =========================

void sendMessage(const String& body) {
  WiFiClientSecure client;
  client.setInsecure(); // Para testes. Em produção, valide o certificado.

  HTTPClient http;
  String endpoint = String("https://api.twilio.com/2010-04-01/Accounts/") +
                    TWILIO_ACCOUNT_SID + "/Messages.json";

  if (!http.begin(client, endpoint)) {
    Serial.println("Falha ao abrir conexão HTTPS");
    return;
  }

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.setAuthorization(TWILIO_ACCOUNT_SID, TWILIO_AUTH_TOKEN);

  String postData = "To="   + String(YOUR_WHATSAPP_TO) +
                    "&From=" + String(TWILIO_WHATSAPP_FROM) +
                    "&Body=" + urlEncode(body);  // agora sem conflito

  int httpCode = http.POST(postData);
  Serial.printf("HTTP %d\n", httpCode);
  String payload = http.getString();
  Serial.println(payload);

  http.end();
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nWiFi OK. IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA, SCL

  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BOTAO, INPUT_PULLUP); // Botão entre pino e GND

  digitalWrite(LED, LOW);
  digitalWrite(BUZZER, LOW);

  Serial.println("Inicializando MPU-6050...");
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU-6050 conectado!");
  } else {
    Serial.println("Falha na conexão com MPU-6050!");
    while (1);
  }

  connectWiFi();
}

void loop() {
  
  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float accelX = ax / 16384.0;
  float accelY = ay / 16384.0;
  float accelZ = az / 16384.0;

  // Cálculo da aceleração total
  float accelTotal = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);

  // Saída para o Serial Plotter
  // Formato: AccelX, AccelY, AccelZ, AccelTotal
  Serial.print(accelX);      Serial.print("\t");
  Serial.print(accelY);      Serial.print("\t");
  Serial.print(accelZ);      Serial.print("\t");
  Serial.println(accelTotal);

  // Se alarme não está ativo, verificar queda
  if (!alarmeAtivo) {
    if (accelTotal > LIMIAR_SUP || accelTotal < LIMIAR_INF) {
      alarmeAtivo = true;
      Serial.println("🚨 Queda detectada! 🚨");
      sendMessage("🚨 Queda detectada! 🚨 \n O(A) Senhor(a) " + String(name) + " sofreu um acidente, entrar em contato e procurar por socorro!");
    }
    if (digitalRead(BOTAO) == LOW) {
      alarmeAtivo = true;
      Serial.println("🚨 Situação de Emergência! 🚨");
      sendMessage("🚨 Situação de Emergência! 🚨 \n O(A) Senhor(a) " + String(name) + " solicitou uma emergência, entrar em contato e procurar por socorro!");
      delay(500);
    }
  }
  // Alarme ativo → LED e buzzer intermitentes
  if (alarmeAtivo) {
    digitalWrite(LED, HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(200);
    digitalWrite(LED, LOW);
    digitalWrite(BUZZER, LOW);
    delay(200);

    // Cancelar alarme pelo botão
    if (digitalRead(BOTAO) == LOW) {
      alarmeAtivo = false;
      digitalWrite(LED, LOW);
      digitalWrite(BUZZER, LOW);
      Serial.println("✅ Alarme cancelado");
      delay(500); // debounce
    }
  } else {
    delay(50); // atualização rápida quando sem alarme
  }
}