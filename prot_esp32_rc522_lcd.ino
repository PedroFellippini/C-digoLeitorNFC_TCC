#include <LiquidCrystal_I2C.h>

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

// Endereço mais comum: 0x27
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==========================
// CARTÃO AUTORIZADO
// ==========================

// COLOQUE AQUI O UID DO SEU CARTÃO
byte uidAutorizado[] = {
  0x7A,
  0xBF,
  0x3B,
  0x1D
};

void setup() {

  Serial.begin(115200);

  // Inicia LCD
  Wire.begin(21, 22);

  lcd.begin();
  lcd.backlight();

  // Inicia RC522
  SPI.begin();
  rfid.PCD_Init();

  // Tela inicial
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   QUICKPASS");

  lcd.setCursor(0, 1);
  lcd.print("Aproxime cartao");

  delay(2000);

  Serial.println("QUICKPASS iniciado.");
}

void loop() {

  // Verifica se existe cartão
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Tenta ler o cartão
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Mostra UID no Serial
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

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("CARTAO INVALIDO");

    lcd.setCursor(0, 1);
    lcd.print("Acesso negado");

    delay(3000);
  }

  // Finaliza comunicação
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // Volta para tela inicial
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

  // Verifica tamanho do UID
  if (rfid.uid.size != sizeof(uidAutorizado)) {
    return false;
  }

  // Compara UID
  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] != uidAutorizado[i]) {
      return false;
    }
  }

  return true;
}