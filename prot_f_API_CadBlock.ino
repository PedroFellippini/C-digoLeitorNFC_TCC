
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// ==========================
// WIFI E API
// ==========================

const char* ssid = "NOME_DO_WIFI";
const char* password = "SENHA_DO_WIFI";

const char* apiURL =
  "https://tcc-82gl.onrender.com/cartao/verificar";

// ==========================
// RC522
// ==========================

#define SS_PIN 5
#define RST_PIN 4

MFRC522 rfid(SS_PIN, RST_PIN);

// ==========================
// LCD I2C
// ==========================

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==========================
// LEDS
// ==========================

#define LED_VERMELHO 32
#define LED_BRANCO 33
#define LED_VERDE 25

// ==========================
// ESTADOS DOS LEDS
// ==========================

void ledAguardando() {
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_BRANCO, HIGH);
  digitalWrite(LED_VERDE, LOW);
}

void ledValido() {
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_BRANCO, LOW);
  digitalWrite(LED_VERDE, HIGH);
}

void ledInvalido() {
  digitalWrite(LED_VERMELHO, HIGH);
  digitalWrite(LED_BRANCO, LOW);
  digitalWrite(LED_VERDE, LOW);
}

// ==========================
// TELA INICIAL
// ==========================

void telaInicial() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   QUICKPASS");
  lcd.setCursor(0, 1);
  lcd.print("Aproxime cartao");
}

// ==========================
// CONSULTA A API
// Retorno:
//  1 = liberado
//  0 = nao cadastrado
//  2 = bloqueado
// -1 = erro
// ==========================

int verificarCartaoNaAPI(String uid) {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi desconectado.");
    return -1;
  }

  WiFiClientSecure cliente;

  // Apenas para prototipo.
  // Em producao, valide o certificado HTTPS.
  cliente.setInsecure();

  HTTPClient https;

  if (!https.begin(cliente, apiURL)) {
    Serial.println("Falha ao iniciar HTTPS.");
    return -1;
  }

  https.setTimeout(15000);
  https.addHeader("Content-Type", "application/json");

  // Monta o JSON enviado para a API
  JsonDocument envio;
  envio["uid_cartao"] = uid;

  String corpo;
  serializeJson(envio, corpo);

  Serial.println("Consultando API QUICKPASS...");
  Serial.println("UID enviado: " + uid);

  int codigoHTTP = https.POST(corpo);
  String resposta = "";

  if (codigoHTTP > 0) {
    resposta = https.getString();

    Serial.printf("HTTP: %d\n", codigoHTTP);
    Serial.println("Resposta da API:");
    Serial.println(resposta);
  } else {
    Serial.println(https.errorToString(codigoHTTP));
  }

  https.end();

  // Somente HTTP 200 e JSON valido podem autorizar
  if (codigoHTTP != 200) {
    Serial.println("Erro na resposta HTTP.");
    return -1;
  }

  JsonDocument doc;

  DeserializationError erro =
    deserializeJson(doc, resposta);

  if (erro) {
    Serial.println("JSON invalido.");
    return -1;
  }

  // Confere se o campo cadastrado existe e e booleano
  if (!doc["cadastrado"].is<bool>()) {
    Serial.println("Campo cadastrado ausente ou invalido.");
    return -1;
  }

  bool cadastrado = doc["cadastrado"].as<bool>();

  if (!cadastrado) {
    return 0;
  }

  // Para cartao cadastrado, os dois campos sao obrigatorios
  if (!doc["cartao_bloqueado"].is<bool>() ||
      !doc["autorizado"].is<bool>()) {
    Serial.println("Dados de autorizacao invalidos.");
    return -1;
  }

  bool bloqueado =
    doc["cartao_bloqueado"].as<bool>();

  bool autorizado =
    doc["autorizado"].as<bool>();

  if (bloqueado) {
    return 2;
  }

  if (autorizado && !bloqueado) {
    return 1;
  }

  // Qualquer resposta inconsistente nega o acesso
  return -1;
}

// ==========================
// CONFIGURACAO INICIAL
// ==========================

void setup() {

  Serial.begin(115200);

  // Configura os LEDs
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_BRANCO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);

  ledAguardando();

  // Inicializa o LCD
  Wire.begin(21, 22);
  lcd.begin();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   QUICKPASS");
  lcd.setCursor(0, 1);
  lcd.print("Conectando WiFi");

  // Conecta ao Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando ao Wi-Fi");

  unsigned long inicio = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - inicio < 15000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi conectado");
  } else {
    Serial.println("Falha ao conectar Wi-Fi.");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Falha no WiFi");
    lcd.setCursor(0, 1);
    lcd.print("Verifique rede");

    ledInvalido();
    delay(2000);
  }

  // Inicializa o RC522
  SPI.begin();
  rfid.PCD_Init();

  Serial.println("RC522 inicializado.");

  telaInicial();
  ledAguardando();
}

// ==========================
// LOOP PRINCIPAL
// ==========================

void loop() {

  // Aguarda um cartao
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // ==========================
  // LEITURA DO UID
  // ==========================

  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }

    String parte =
      String(rfid.uid.uidByte[i], HEX);

    parte.toUpperCase();
    uid += parte;
  }

  Serial.println();
  Serial.println("Cartao detectado!");
  Serial.println("UID: " + uid);

  // Encerra a comunicacao com o cartao
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // ==========================
  // CONSULTA
  // ==========================

  ledAguardando();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Verificando...");
  lcd.setCursor(0, 1);
  lcd.print("Aguarde");

  int resultado = verificarCartaoNaAPI(uid);

  // ==========================
  // CARTAO LIBERADO
  // ==========================

  if (resultado == 1) {

    Serial.println("CARTAO LIBERADO");

    ledValido();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CARTAO CADASTR.");
    lcd.setCursor(0, 1);
    lcd.print("Acesso liberado");
  }

  // ==========================
  // CARTAO NAO CADASTRADO
  // ==========================

  else if (resultado == 0) {

    Serial.println("CARTAO NAO CADASTRADO");

    ledInvalido();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CARTAO INVALIDO");
    lcd.setCursor(0, 1);
    lcd.print("Acesso negado");
  }

  // ==========================
  // CARTAO BLOQUEADO
  // ==========================

  else if (resultado == 2) {

    Serial.println("CARTAO BLOQUEADO");

    ledInvalido();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CARTAO BLOQUEADO");
    lcd.setCursor(0, 1);
    lcd.print("Acesso negado");
  }

  // ==========================
  // ERRO NA API OU CONEXAO
  // ==========================

  else {

    Serial.println("ERRO NA CONSULTA API");

    ledInvalido();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erro na consulta");
    lcd.setCursor(0, 1);
    lcd.print("Tente novamente");
  }

  // Mantem o resultado visivel por 3 segundos
  delay(3000);

  // Volta ao estado de espera
  ledAguardando();
  telaInicial();
}