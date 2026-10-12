
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>

// ==========================
// WIFI
// ==========================

const char* ssid = "Wifi 2.4";
const char* password = "mfellippini1";

// ==========================
// API QUICKPASS
// ==========================

const char* apiURL =
  "https://tcc-82gl.onrender.com/cartao/verificar";

// ==========================
// RC522
// ==========================

#define SS_PIN 5
#define RST_PIN 4

MFRC522 rfid(SS_PIN, RST_PIN);

// ==========================
// LCD
// ==========================

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==========================
// LEDs
// ==========================

#define LED_VERMELHO 32
#define LED_BRANCO   33
#define LED_VERDE    25

// ==========================
// ESTADO DOS LEDs
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
// VERIFICAR UID NA API
//
// Retorno:
//  1 = cadastrado
//  0 = nao cadastrado
// -1 = erro de conexao/API
// ==========================

int verificarCartaoNaAPI(String uid) {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi desconectado.");
    return -1;
  }

  WiFiClientSecure cliente;
  cliente.setInsecure(); // Apenas para prototipo/testes

  HTTPClient https;

  if (!https.begin(cliente, apiURL)) {
    Serial.println("Falha ao iniciar HTTPS.");
    return -1;
  }

  https.setTimeout(15000);
  https.addHeader("Content-Type", "application/json");

  String corpo = "{\"uid_cartao\":\"" + uid + "\"}";

  Serial.println("Consultando API QUICKPASS...");
  Serial.println("UID enviado: " + uid);

  int codigoHTTP = https.POST(corpo);
  String resposta = "";

  if (codigoHTTP > 0) {
    resposta = https.getString();

    Serial.print("HTTP: ");
    Serial.println(codigoHTTP);

    Serial.print("Resposta: ");
    Serial.println(resposta);
  } else {
    Serial.print("Erro HTTP: ");
    Serial.println(https.errorToString(codigoHTTP));
  }

  https.end();

  if (codigoHTTP != 200) {
    return -1;
  }

  // Interpreta a resposta JSON da API
  int posCadastro = resposta.indexOf("\"cadastrado\"");
  int posTrue = resposta.indexOf("true", posCadastro);
  int posFalse = resposta.indexOf("false", posCadastro);

  if (posCadastro >= 0 &&
      posTrue >= 0 &&
      (posFalse < 0 || posTrue < posFalse)) {
    return 1;
  }

  if (posCadastro >= 0 &&
      posFalse >= 0 &&
      (posTrue < 0 || posFalse < posTrue)) {
    return 0;
  }

  Serial.println("Resposta da API inesperada.");
  return -1;
}

// ==========================
// SETUP
// ==========================

void setup() {

  Serial.begin(115200);

  // LEDs
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_BRANCO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);

  ledAguardando();

  // LCD
  Wire.begin(21, 22);
  lcd.begin();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando WiFi");

  // Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando ao Wi-Fi");

  unsigned long inicio = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - inicio < 15000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi conectado!");
    Serial.print("IP do ESP32: ");
    Serial.println(WiFi.localIP());

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi conectado");
  } else {
    Serial.println("\nFalha ao conectar ao Wi-Fi.");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Falha no WiFi");
  }

  delay(1000);

  // RC522
  SPI.begin();
  rfid.PCD_Init();

  telaInicial();

  Serial.println("QUICKPASS iniciado.");
}

// ==========================
// LOOP PRINCIPAL
// ==========================

void loop() {

  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Monta o UID em hexadecimal, sem separadores
  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }

    String parte = String(rfid.uid.uidByte[i], HEX);
    parte.toUpperCase();
    uid += parte;
  }

  Serial.println();
  Serial.println("Cartao detectado!");
  Serial.println("UID: " + uid);

  // Finaliza a leitura antes da consulta HTTP
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // Indica consulta em andamento
  ledAguardando();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Verificando...");
  lcd.setCursor(0, 1);
  lcd.print("Aguarde");

  int resultado = verificarCartaoNaAPI(uid);

  // ==========================
  // CADASTRADO
  // ==========================

  if (resultado == 1) {

    Serial.println("CARTAO CADASTRADO");

    ledValido();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CARTAO CADASTR.");
    lcd.setCursor(0, 1);
    lcd.print("Acesso liberado");
  }

  // ==========================
  // NAO CADASTRADO
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
  // ERRO DE CONEXAO/API
  // ==========================

  else {

    Serial.println("ERRO NA CONSULTA API");

    // Nao libera acesso quando nao consegue validar
    ledInvalido();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erro na consulta");
    lcd.setCursor(0, 1);
    lcd.print("Tente novamente");
  }

  delay(3000);

  ledAguardando();
  telaInicial();
}