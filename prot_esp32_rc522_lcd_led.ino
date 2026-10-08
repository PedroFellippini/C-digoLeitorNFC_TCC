#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

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
// CARTÃO AUTORIZADO
// ==========================

byte uidAutorizado[][4] = {
  {0x7A, 0xBF, 0x3B, 0x1D},
  {0xD3, 0xC6, 0xEC, 0xF6}
};
// ==========================================
// FUNÇÃO: LED AGUARDANDO
// ==========================================

void ledAguardando() {

  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_BRANCO, HIGH);
  digitalWrite(LED_VERDE, LOW);
}

// ==========================================
// FUNÇÃO: LED CARTÃO VÁLIDO
// ==========================================

void ledValido() {

  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_BRANCO, LOW);
  digitalWrite(LED_VERDE, HIGH);
}

// ==========================================
// FUNÇÃO: LED CARTÃO INVÁLIDO
// ==========================================

void ledInvalido() {

  digitalWrite(LED_VERMELHO, HIGH);
  digitalWrite(LED_BRANCO, LOW);
  digitalWrite(LED_VERDE, LOW);
}

// ==========================================
// SETUP
// ==========================================

void setup() {

  Serial.begin(115200);

  // ==========================
  // CONFIGURAÇÃO DOS LEDs
  // ==========================

  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_BRANCO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);

  // Começa aguardando cartão
  ledAguardando();

  // ==========================
  // INICIA LCD
  // ==========================

  Wire.begin(21, 22);

  lcd.begin();
  lcd.backlight();

  // ==========================
  // INICIA RC522
  // ==========================

  SPI.begin();
  rfid.PCD_Init();

  // ==========================
  // TELA INICIAL
  // ==========================

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("   QUICKPASS");

  lcd.setCursor(0, 1);
  lcd.print("Aproxime cartao");

  Serial.println("QUICKPASS iniciado.");
}

// ==========================================
// LOOP
// ==========================================

void loop() {

  // ==========================
  // AGUARDANDO CARTÃO
  // ==========================

  ledAguardando();

  // Verifica se existe cartão
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Tenta ler o cartão
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // ==========================
  // MOSTRA UID NO SERIAL
  // ==========================

  Serial.print("UID: ");

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {
      Serial.print("0");
    }

    Serial.print(rfid.uid.uidByte[i], HEX);

    if (i < rfid.uid.size - 1) {
      Serial.print(":");
    }
  }

  Serial.println();

  // ==========================
  // CARTÃO VÁLIDO
  // ==========================

  if (cartaoAutorizado()) {

    Serial.println("CARTAO VALIDO");

    // Acende LED verde
    ledValido();

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("CARTAO VALIDO");

    lcd.setCursor(0, 1);
    lcd.print("Acesso liberado");

    delay(3000);
  }

  // ==========================
  // CARTÃO INVÁLIDO
  // ==========================

  else {

    Serial.println("CARTAO INVALIDO");

    // Acende LED vermelho
    ledInvalido();

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("CARTAO INVALIDO");

    lcd.setCursor(0, 1);
    lcd.print("Acesso negado");

    delay(3000);
  }

  // ==========================
  // FINALIZA COMUNICAÇÃO
  // ==========================

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // ==========================
  // VOLTA PARA TELA INICIAL
  // ==========================

  ledAguardando();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("   QUICKPASS");

  lcd.setCursor(0, 1);
  lcd.print("Aproxime cartao");
}

// ==========================================
// FUNÇÃO PARA VERIFICAR O CARTÃO
// ==========================================


bool cartaoAutorizado() {
  // Verifica se o UID possui 4 bytes
  if (rfid.uid.size != 4) {
    return false;
  }

  // Compara com os dois cartões autorizados
  for (byte i = 0; i < 2; i++) {
    bool igual = true;

    for (byte j = 0; j < 4; j++) {
      if (rfid.uid.uidByte[j] != uidAutorizado[i][j]) {
        igual = false;
        break;
      }
    }

    if (igual) {
      return true;
    }
  }

  return false;
}