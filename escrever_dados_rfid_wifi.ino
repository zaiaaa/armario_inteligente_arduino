#include <SPI.h>
#include <MFRC522.h>
#include "WiFi.h"
#include <HTTPClient.h>

#define RST_PIN         22
#define SS_PIN          21

String url = "http://192.168.0.52:5000/usuarios/";

char* ssid = "Travazap_2G";
char* password = "GED022829";

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  SPI.begin(18, 19, 23, SS_PIN);
  mfrc522.PCD_Init();
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(1500);
  }

  Serial.println("Conectado!");
  Serial.println(WiFi.localIP());

  Serial.println("Aguarde um momento:");
  delay(5000);

  Serial.println(F("Gravador RFID iniciado. Aproximar o cartão e enviar os dados:"));
}

void loop() {
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  String uid = "";
  Serial.print(F("Card UID: "));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  Serial.println(uid);
  Serial.println();

  byte buffer[34];
  byte len;
  byte block;
  MFRC522::StatusCode status;

  Serial.setTimeout(20000L);

  // ==================== NOME COMPLETO ====================
  Serial.println(F("Digite o NOME COMPLETO (ex: João Silva), termine com #:"));
  len = Serial.readBytesUntil('#', (char *)buffer, 32);
  for (byte i = len; i < 32; i++) buffer[i] = ' ';

  String nome = "";
  for (int i = 0; i < 32; i++) nome += (char)buffer[i];
  nome.trim();  // Remove espaços extras no final

  Serial.println(nome);

  // Bloco 1
  block = 1;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Erro de autenticação no bloco 1: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Erro ao escrever bloco 1: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // Bloco 2
  block = 2;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Erro de autenticação no bloco 2: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Write(block, &buffer[16], 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Erro ao escrever bloco 2: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // ==================== ID ====================
  Serial.println(F("Digite o ID de 5 dígitos (ex: 12345), termine com #:"));
  len = Serial.readBytesUntil('#', (char *)buffer, 16);
  for (byte i = len; i < 16; i++) buffer[i] = ' ';

  String id_colaborador = "";
  for (int i = 0; i < 32; i++) id_colaborador += (char)buffer[i];
  id_colaborador.trim();  // Remove espaços extras no final

  Serial.println(id_colaborador);

  block = 4;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Erro de autenticação no bloco 4: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Erro ao escrever bloco 4: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // ==================== SETOR ====================
  Serial.println(F("Digite o SETOR (ex: manut T), termine com #:"));
  len = Serial.readBytesUntil('#', (char *)buffer, 16);
  for (byte i = len; i < 16; i++) buffer[i] = ' ';

  String setor = "";
  for (int i = 0; i < 32; i++) setor += (char)buffer[i];
  setor.trim();  // Remove espaços extras no final

  Serial.println(setor);

  block = 5;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Erro de autenticação no bloco 5: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Erro ao escrever bloco 5: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json"); // Tipo de dado que vai enviar
  String body = "{\"nome\": \"" + nome +
                "\", \"id_colaborador\": \"" + id_colaborador +
                "\", \"setor\": \"" + setor +
                "\", \"UID\": \"" + uid + "\"}";;

  Serial.println(body);

  int response = http.POST(body);

  if(response == 201){
    Serial.println("Usuário registrado com sucesso!");
    Serial.println("resposta http" + http.getString());
  }else if(response == 403){
    Serial.println("UID obrigatório!");
    Serial.println("resposta http" + http.getString());
    delay(2000);
  }
  else{
    String resp = http.getString();
    Serial.println("resposta http" + http.getString());
    Serial.println("FALHA!");
    delay(5000);
  }


  Serial.println(F("Todos os dados foram gravados com sucesso!"));
  Serial.println(F("Remova o cartão."));

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
