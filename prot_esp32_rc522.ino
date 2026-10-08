#include <SPI.h>
#include <MFRC522.h>

// Pinos do RC522
#define SS_PIN 5
#define RST_PIN 22

MFRC522 rfid(SS_PIN, RST_PIN);

// UID do cartão autorizado
// ALTERE para o UID do seu cartão
byte uidAutorizado[] = {0x7A, 0xBF, 0x3B, 0x1D};

void setup() {
  Serial.begin(115200);

  SPI.begin();
  rfid.PCD_Init();

  Serial.println("================================");
  Serial.println("       QUICKPASS - VALIDADOR");
  Serial.println("================================");
  Serial.println("Aproxime o cartao...");
}

void loop() {

  // Verifica se existe cartão próximo
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Tenta ler o cartão
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println();
  Serial.println("Cartao detectado!");

  // Mostra o UID
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

  // Verifica se o cartão está autorizado
  if (cartaoAutorizado()) {
    Serial.println("STATUS: CARTAO VALIDO");
    Serial.println("Acesso liberado!");
  } 
  else {
    Serial.println("STATUS: CARTAO INVALIDO");
    Serial.println("Acesso negado!");
  }

  Serial.println("--------------------------------");

  // Finaliza comunicação com o cartão
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(1500);

  Serial.println("Aproxime o cartao...");
}
// ======================================
// FUNÇÃO PARA VALIDAR O CARTÃO
// ======================================
bool cartaoAutorizado() {

  // Verifica se o tamanho do UID é igual
  if (rfid.uid.size != sizeof(uidAutorizado)) {
    return false;
  }

  // Compara cada byte do UID
  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] != uidAutorizado[i]) {
      return false;
    }
  }

  return true;
}
