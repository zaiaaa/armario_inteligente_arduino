#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

String urlAbertura = "https://api-armario-inteligente.onrender.com/abertura/";
String urlCadastro = "https://api-armario-inteligente.onrender.com/usuarios/";
String urlMaster = "https://api-armario-inteligente.onrender.com/master/";



char* ssid = "Travazap_2G";
char* password = "GED022829";

String emergencyUID = "F7B2F63";

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
LiquidCrystal_I2C lcd(0x27, 16, 2);

//LiquidCrystal lcd(15, 13, 4, 16, 17, 33);

void setup() {
  Wire.begin(32, 33);
  Serial.begin(115200);
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(limit_switch, INPUT);
  pinMode(rele, OUTPUT);
  pinMode(botao, INPUT);
  pinMode(yellow, OUTPUT);

  //lcd.begin(16, 2);
  lcd.init();
  lcd.backlight();
  
  SPI.begin(18, 19, 23, SS_PIN);
  mfrc522.PCD_Init();
  WiFi.begin(ssid, password);
  lcd.print("Conectando Wi-Fi");
  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(1500);
  }

  digitalWrite(rele, LOW);  // Garante relé desligado no início

  lcd.print("Conectado!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  lcd.clear();

  lcd.print("Aguarde...");
  delay(5000);
  lcd.clear();
}

void loop() {
  digitalWrite(red, LOW);
  digitalWrite(green, LOW);
  digitalWrite(rele, LOW);
  
  trocaEstado();

  if(WiFi.status() == WL_CONNECTED){
    if(estadoBotao == false){
      modoLeitura();
    }else{
      modoCadastro();
    }
  }else{
    offline();
  }

}

void offline(){
      lcd.clear();
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

      if (uid == emergencyUID) {
        // Tag de emergência detectada!
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("EMERGENCIA!");
        digitalWrite(green, HIGH);
        digitalWrite(rele, HIGH);
        delay(5000);  // abre por 5 segundos (ajuste se quiser)
        while(digitalRead(limit_switch) == 1);  // espera o armario fechar (se tiver limit switch)
        digitalWrite(green, LOW);
        digitalWrite(rele, LOW);
        mensagem_mostrada = false;
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        // Aguarda o cartão ser removido do campo
        while (mfrc522.PICC_IsNewCardPresent() || mfrc522.PICC_ReadCardSerial()) {
          delay(100);
        }

        delay(1000);
        return;  // sai do loop para não executar o resto do código
      }
}


void modoCadastro(){
        digitalWrite(yellow, HIGH);
      MFRC522::MIFARE_Key key;
      for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
          
      if (!mensagem_mostrada) {
        lcd.setCursor(0, 0);
        lcd.print("Modo CADASTRO");
        lcd.setCursor(0, 1);
        lcd.print("Aproxime MASTER:");
        mensagem_mostrada = true;
      }

      if (!mfrc522.PICC_IsNewCardPresent()) return;
      if (!mfrc522.PICC_ReadCardSerial()) return;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Lendo cartao...");

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
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Tag master");
        lcd.setCursor(0, 1);
        lcd.print("Invalida.");
        Serial.println(http.getString());
        digitalWrite(red, HIGH);
        delay(2000);
        digitalWrite(yellow, LOW);
        estadoBotao = false;
        mensagem_mostrada = false;
        lcd.clear();
        return;
      }
      delay(1000);
      http.end();
      lcd.clear();
      digitalWrite(green, HIGH);
      lcd.setCursor(0, 0);
      lcd.print("OK! aproxime");
      lcd.setCursor(0, 1);
      lcd.print("sua nova tag!");
      delay(1000);
      while (mfrc522.PICC_IsNewCardPresent()) mfrc522.PICC_ReadCardSerial(); // esvazia buffer

      // 2. Espera NOVA TAG
      while (!mfrc522.PICC_IsNewCardPresent()) yield();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Lendo cartao...");

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
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("tag registrada");
        lcd.setCursor(0, 1);
        lcd.print("com sucesso!");
        Serial.println("resposta http" + http.getString());
        digitalWrite(green, HIGH);
        delay(2000);
      }else if(response == 403 && estadoBotao){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("UID obrigatorio");
        Serial.println("UID obrigatório!!!");
        Serial.println("resposta http: ->" + http.getString());
        digitalWrite(red, HIGH);
        delay(2000);
      }else if(response == 409 && estadoBotao){
        String resp = http.getString();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("cartao ja");
        lcd.setCursor(0, 1);
        lcd.print("cadastrado!");
        Serial.println(resp);
        digitalWrite(red, HIGH);
        delay(5000);
      }
      else{
        String resp = http.getString();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("unknown error");
        Serial.println(resp);
        Serial.println("FALHA!");
        digitalWrite(red, HIGH);
        delay(5000);
      }

      lcd.clear();
      mensagem_mostrada = false;

      http.end();
}


void modoLeitura(){
          digitalWrite(yellow, LOW);
          if (!mensagem_mostrada) {
            lcd.setCursor(0, 0);
            lcd.print("Modo LEITURA");
            lcd.setCursor(0, 1);
            lcd.print("Aproxime cracha:");
            mensagem_mostrada = true;
          }
      Serial.println("Aguardando UID");      
        String uid = "";
        if (!mfrc522.PICC_IsNewCardPresent()) return;
        if (!mfrc522.PICC_ReadCardSerial()) return;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Lendo cartao...");
        Serial.println(F("**Cartão Detectado:**"));
        
          Serial.print(F("Card UID: "));
          for (byte i = 0; i < mfrc522.uid.size; i++) {
            Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
            Serial.print(mfrc522.uid.uidByte[i], HEX);
            uid += String(mfrc522.uid.uidByte[i], HEX);
          }
          uid.toUpperCase();
          Serial.println("UID lido");
          if (uid == emergencyUID) {
            // Tag de emergência detectada!
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("EMERGENCIA!");
            digitalWrite(green, HIGH);
            digitalWrite(rele, HIGH);
            delay(2000);  // abre por 5 segundos (ajuste se quiser)
            while(digitalRead(limit_switch) == 1);  // espera o armario fechar (se tiver limit switch)
            digitalWrite(green, LOW);
            digitalWrite(rele, LOW);
            mensagem_mostrada = false;
            Serial.println("Lido o cartao de emergencia");
            delay(1000);
            lcd.clear();
            return;  // sai do loop para não executar o resto do código
          }


          HTTPClient http;
          http.begin(urlAbertura);
          http.addHeader("Content-Type", "application/json"); // Tipo de dado que vai enviar
          String body = "{\"UID\": \"" + uid + "\"}";

          Serial.println(body);

          int response = http.POST(body);

          if(response == 201 && estadoBotao == false){
            lcd.clear();
            digitalWrite(green, HIGH);
            digitalWrite(rele, HIGH);
            lcd.setCursor(0, 0);
            lcd.print("Aberto!");
            lcd.setCursor(0, 1);
            lcd.print("Cartao OK");
            Serial.println("resposta http" + http.getString());
            delay(1000);
            while(digitalRead(limit_switch) == 1);
            delay(1500);
            Serial.println("leitura ok");
          }else if(response == 403 && estadoBotao == false){
            lcd.clear();
            digitalWrite(green, LOW);
            Serial.println("UID não encontrado!");
            lcd.setCursor(0, 0);
            lcd.print("Cartao nao");
            lcd.setCursor(0, 1);
            lcd.print("encontrado!");
            for(int i = 0; i <= 2; i++){
              digitalWrite(red, HIGH);
              delay(500);
              digitalWrite(red, LOW);
              delay(500);
            }
            Serial.println("leitura ok.");
            delay(1500);
          }
          else{
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Unknown error");
            lcd.setCursor(0, 1);
            lcd.print("Visualize serial.");
            String resp = http.getString();
            digitalWrite(green, LOW);
            digitalWrite(red, HIGH);
            Serial.println("FALHA!");
            Serial.println(response);
            Serial.println(resp);
            Serial.println("leitura ok.");
            delay(2000);
          }
        Serial.println("SAIU do modo leitura.");
        mensagem_mostrada = false;
        Serial.println(F("\n**Leitura finalizada**\n"));

      http.end();
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      delay(1500);
}


void trocaEstado(){

  if(digitalRead(botao) == 1){
    lcd.clear();
    estadoBotao = !estadoBotao;
    mensagem_mostrada = false;
    while(digitalRead(botao) == 1);
    delay(150);
  }

}