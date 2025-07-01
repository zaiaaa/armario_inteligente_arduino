#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>

String urlAbertura = "http://192.168.0.52:5000/abertura/";
String urlCadastro = "http://192.168.0.52:5000/usuarios/";
String urlMaster = "http://192.168.0.52:5000/master/";



char* ssid = "Travazap_2G";
char* password = "GED022829";

bool mensagem_mostrada = false;


#define RST_PIN         22
#define SS_PIN          21
#define green           2
#define limit_switch    5
#define red             25
#define botao           26
#define yellow          14


bool estadoBotao = false;

const int rele = 27;
MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(limit_switch, INPUT);
  pinMode(rele, OUTPUT);
  pinMode(botao, INPUT);
  pinMode(yellow, OUTPUT);


  SPI.begin(18, 19, 23, SS_PIN);
  mfrc522.PCD_Init();
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(1500);
  }

  digitalWrite(rele, LOW);  // Garante relé desligado no início

  Serial.println("Conectado!");
  Serial.println(WiFi.localIP());

  Serial.println("Aguarde um momento:");
  delay(5000);
}

void loop() {
  digitalWrite(red, LOW);
  digitalWrite(green, LOW);
  digitalWrite(rele, LOW);
  
  trocaEstado();

  if(WiFi.status() == WL_CONNECTED){
    if(estadoBotao == false){
          digitalWrite(yellow, LOW);
          if (!mensagem_mostrada) {
            Serial.println(F("Modo LEITURA. Aproxime crachá:"));
            mensagem_mostrada = true;
          }
        
        String uid = "";
        if (!mfrc522.PICC_IsNewCardPresent()) return;
        if (!mfrc522.PICC_ReadCardSerial()) return;

        Serial.println(F("**Cartão Detectado:**"));
        
          Serial.print(F("Card UID: "));
          for (byte i = 0; i < mfrc522.uid.size; i++) {
            Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
            Serial.print(mfrc522.uid.uidByte[i], HEX);
            uid += String(mfrc522.uid.uidByte[i], HEX);
          }
          uid.toUpperCase();

          HTTPClient http;
          http.begin(urlAbertura);
          http.addHeader("Content-Type", "application/json"); // Tipo de dado que vai enviar
          String body = "{\"UID\": \"" + uid + "\"}";

          Serial.println(body);

          int response = http.POST(body);

          if(response == 201 && estadoBotao == false){
            digitalWrite(green, HIGH);
            digitalWrite(rele, HIGH);
            Serial.println("Aberto e registrado com sucesso!");
            Serial.println("resposta http" + http.getString());
            delay(5000);
            while(digitalRead(limit_switch) == 1);
            delay(1500);
          }else if(response == 403 && estadoBotao == false){
            digitalWrite(green, LOW);
            Serial.println("UID não encontrado!");
            for(int i = 0; i <= 5; i++){
              digitalWrite(red, HIGH);
              delay(500);
              digitalWrite(red, LOW);
              delay(500);
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
            delay(2000);
          }
        
        mensagem_mostrada = false;
        Serial.println(F("\n**Leitura finalizada**\n"));

      http.end();
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      delay(1000);

    }else{
      digitalWrite(yellow, HIGH);
      MFRC522::MIFARE_Key key;
      for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
          
      if (!mensagem_mostrada) {
        Serial.println(F("Modo CADASTRO. Aproxime crachá MASTER:"));
        mensagem_mostrada = true;
      }

      if (!mfrc522.PICC_IsNewCardPresent()) return;
      if (!mfrc522.PICC_ReadCardSerial()) return;

      String uidMaster = "";
      Serial.print(F("Card UID: "));
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        uidMaster += String(mfrc522.uid.uidByte[i], HEX);
      }
      uidMaster.toUpperCase();
      Serial.println(uidMaster);
      Serial.println();

      HTTPClient http;
      http.begin(urlMaster);
      http.addHeader("Content-Type", "application/json"); // Tipo de dado que vai enviar
      String body = "{\"UID\": \"" + uidMaster + "\"}";;
      Serial.println(body);
      int response = http.POST(body);

      if (response != 201) {
        Serial.println("Tag mestre não autorizada.");
        Serial.println(http.getString());
        digitalWrite(red, HIGH);
        delay(2000);
        digitalWrite(yellow, LOW);
        estadoBotao = false;
        mensagem_mostrada = false;
        return;
      }
      delay(1000);
      http.end();
      digitalWrite(green, HIGH);
      Serial.println("Tag mestre válida! Aproxime a nova TAG para cadastrar.");
      delay(1000);
      while (mfrc522.PICC_IsNewCardPresent()) mfrc522.PICC_ReadCardSerial(); // esvazia buffer

      // 2. Espera NOVA TAG
      while (!mfrc522.PICC_IsNewCardPresent()) yield();
      while (!mfrc522.PICC_ReadCardSerial()) yield();

      String uid_novo = "";
      Serial.print(F("UID NOVO: "));
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        uid_novo += String(mfrc522.uid.uidByte[i], HEX);
      }
      uid_novo.toUpperCase();
      Serial.println(uid_novo);
      digitalWrite(green, LOW);
      // Envia o cadastro
      http.begin(urlCadastro);
      http.addHeader("Content-Type", "application/json");
      body = "{\"UID\": \"" + uid_novo + "\"}";
      response = http.POST(body);

      if(response == 201 && estadoBotao){
        Serial.println("Usuário registrado com sucesso!");
        Serial.println("resposta http" + http.getString());
        digitalWrite(green, HIGH);
      }else if(response == 403 && estadoBotao){
        Serial.println("UID obrigatório!!!");
        Serial.println("resposta http: ->" + http.getString());
        digitalWrite(red, HIGH);
        delay(2000);
      }
      else{
        String resp = http.getString();
        Serial.println(resp);
        Serial.println("FALHA!");
        digitalWrite(red, HIGH);
        delay(5000);
      }


      Serial.println(F("Remova o cartão."));
      mensagem_mostrada = false;

      http.end();
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
    }
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(1000);
  }

}

void trocaEstado(){

  if(digitalRead(botao) == 1){
    estadoBotao = !estadoBotao;
    mensagem_mostrada = false;
    while(digitalRead(botao) == 1);
    delay(150);
  }

}
