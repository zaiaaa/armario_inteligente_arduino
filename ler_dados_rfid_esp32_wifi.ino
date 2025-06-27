#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>

String url = "http://192.168.0.52:5000/abertura/";

char* ssid = "Travazap_2G";
char* password = "GED022829";

bool mensagem_mostrada = false;


#define RST_PIN         22
#define SS_PIN          21
#define green           2
#define botao           5
#define red             25

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(botao, INPUT);

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

  Serial.println(F("Leitor RFID pronto. Aproxime um cartão..."));
}

void loop() {
  digitalWrite(red, LOW);
  digitalWrite(green, LOW);
  if (!mensagem_mostrada) {
    Serial.println(F("Leitor RFID pronto. Aproxime um cartão..."));
    mensagem_mostrada = true;
  }
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  byte block;
  byte len = 18;
  MFRC522::StatusCode status;

  byte buffer1[18];
  byte buffer2[18];
  byte bufferID[18];
  byte bufferSetor[18];

  if(WiFi.status() == WL_CONNECTED){

    if (!mfrc522.PICC_IsNewCardPresent()) return;
    if (!mfrc522.PICC_ReadCardSerial()) return;

    Serial.println(F("**Cartão Detectado:**"));
    mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));

    // ====================== ID ======================
    block = 4;
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    if (status == MFRC522::STATUS_OK && mfrc522.MIFARE_Read(block, bufferID, &len) == MFRC522::STATUS_OK) {
      String id = "";
      for (uint8_t i = 0; i < 16; i++) id += (char)bufferID[i];
      id.trim();
      Serial.print(F("ID: "));
      Serial.println(id);

      if(id.length() == 0 || id == ""){
        digitalWrite(red, HIGH);
        Serial.println("não existe ID na memória do cartão.");
        delay(2000);
        return;
      }
    
      HTTPClient http;
      http.begin(url);
      http.addHeader("Content-Type", "application/json"); // Tipo de dado que vai enviar
      String body = "{\"id_colaborador\": \"" + id + "\"}";

      Serial.println(body);

      int response = http.POST(body);

      if(response == 201 && digitalRead(botao) == 0){
        digitalWrite(green, HIGH);
        Serial.println("Aberto e registrado com sucesso!");
        Serial.println("resposta http" + http.getString());
        while(digitalRead(botao) == 0);
      }else if(response == 403 && digitalRead(botao) == 0){
        digitalWrite(green, LOW);
        Serial.println("ID não encontrado!");
        for(int i = 0; i <= 5; i++){
          digitalWrite(red, HIGH);
          delay(1000);
        }
        delay(2000);
      }
      else{
        String resp = http.getString();
        digitalWrite(green, LOW);
        digitalWrite(red, HIGH);
        Serial.println("FALHA!");
        Serial.println(response);
        Serial.println(resp);
        delay(5000);
      }
    
    }else{
      digitalWrite(red, HIGH);
      Serial.println("não existe ID na memória do cartão.");
      delay(2000);
    }
    mensagem_mostrada = false;
    Serial.println(F("\n**Leitura finalizada**\n"));

  }
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(1000);
}
