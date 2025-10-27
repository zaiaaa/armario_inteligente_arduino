#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ArduinoJson.h>

String urlAbertura = "https://api-armario-inteligente.onrender.com/abertura/";
String urlCadastro = "https://api-armario-inteligente.onrender.com/usuarios/";
String urlMaster = "https://api-armario-inteligente.onrender.com/master/";
String urlStatus = "https://api-armario-inteligente.onrender.com/status_abertura/status";
String urlResetStatus = "https://api-armario-inteligente.onrender.com/status_abertura/status/reset";


char* ssid = "TDB-GUEST";
char* password = "";

String emergencyUID = "F7B2F63";

bool mensagem_mostrada = false;


#define RST_PIN         22
#define SS_PIN          21
#define green           2
#define red             25
#define botao           26
#define yellow          14


enum Modo { RETIRADA, CADASTRO, DEVOLUCAO };
Modo modoAtual = RETIRADA;
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
  resetStatus();
  delay(1000);
  lcd.clear();
}

void loop() {
  digitalWrite(red, LOW);
  digitalWrite(green, LOW);
  digitalWrite(rele, LOW);
  
  trocaEstado();

  if(WiFi.status() == WL_CONNECTED){
    switch (modoAtual) {
      case RETIRADA:
        validaUsuario();
        break;
      case CADASTRO:
        modoCadastro();
        break;
      case DEVOLUCAO:
        validaUsuario();
        break;
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

      if(response == 201){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("tag registrada");
        lcd.setCursor(0, 1);
        lcd.print("com sucesso!");
        Serial.println("resposta http" + http.getString());
        digitalWrite(green, HIGH);
        delay(2000);
      }else if(response == 403){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("UID obrigatorio");
        Serial.println("UID obrigatório!!!");
        Serial.println("resposta http: ->" + http.getString());
        digitalWrite(red, HIGH);
        delay(2000);
      }else if(response == 409){
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

void modoRetirada() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Escaneie qrcode");
  lcd.setCursor(0, 1);
  lcd.print("de Retirada");

  unsigned long tempoInicio = millis();
  while (true) {

    if (millis() - tempoInicio > 120000) {  // 2 minutos = 120000 ms
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tempo expirado!");
      delay(2000);
      lcd.clear();
      mensagem_mostrada = false;  // permite exibir nova mensagem depois
      return;  // sai do modoRetirada e volta para o loop principal
    }
    HTTPClient http;
    
    http.begin(urlStatus);
    int response = http.GET();

    if (response == 200) {
      Serial.println("entrou 200");
      String payload = http.getString();
      Serial.println("Resposta da API:");
      Serial.println(payload);
      Serial.println("");

    if (payload.indexOf("\"status\":true") != -1) {
      resetStatus();
      delay(100);
      Serial.println("Abriu a trava!");
      abrirTrava();
      break;
    }
  }else{
    String payload = http.getString();
    Serial.println(payload);
  }

    http.end();
    delay(2000);  // espera 2 segundos antes de consultar novamente
  }

  mensagem_mostrada = false;

}


void modoDevolucao(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Escaneie qrcode");
  lcd.setCursor(0, 1);
  lcd.print("de Devolucao");

  unsigned long tempoInicio = millis();
  while (true) {

    if (millis() - tempoInicio > 120000) {  // 2 minutos = 120000 ms
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tempo expirado!");
      delay(2000);
      lcd.clear();
      mensagem_mostrada = false;  // permite exibir nova mensagem depois
      return;  // sai do modoRetirada e volta para o loop principal
    }
    HTTPClient http;
    
    http.begin(urlStatus);
    int response = http.GET();

    if (response == 200) {
      String payload = http.getString();
      Serial.println("Resposta da API:");
      Serial.println(payload);

    if (payload.indexOf("\"status\":true") != -1) {
      resetStatus();
      delay(100);
      Serial.println("Abriu a trava!");
      abrirTrava();
      break;
    }
  }

    http.end();
    delay(2000);  // espera 2 segundos antes de consultar novamente
  }

  mensagem_mostrada = false;

}

void validaUsuario(){
      digitalWrite(yellow, LOW);
      if (!mensagem_mostrada) {
        switch(modoAtual){
          case RETIRADA:
          lcd.setCursor(0, 0);
          lcd.print("Modo RETIRADA");
          lcd.setCursor(0, 1);
          lcd.print("Aproxime cracha:");
          break;

          case DEVOLUCAO:
          lcd.setCursor(0, 0);
          lcd.print("Modo DEVOLUCAO");
          lcd.setCursor(0, 1);
          lcd.print("Aproxime cracha:");
          break;
        }
        
        mensagem_mostrada = true;
      }
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
            abrirEmergencia();
            return;  // sai do loop para não executar o resto do código
          }


          HTTPClient http;
          http.begin(urlAbertura);
          http.addHeader("Content-Type", "application/json"); // Tipo de dado que vai enviar
          String body = "";

          switch (modoAtual) {
            case DEVOLUCAO:
              body = "{\"UID\": \"" + uid + "\", \"acao\": \"devolucao\"}";
              break;
            case RETIRADA:
              body = "{\"UID\": \"" + uid + "\", \"acao\": \"retirada\"}";
              break;
          }


          Serial.println(body);

          int response = http.POST(body);

          if(response == 201){
            //Aqui solicitaremos a leitura do qrcode e preenchimento do formulario. Após isso abriremos a trava.
            switch (modoAtual) {
              case DEVOLUCAO:
                modoDevolucao();
                break;
              case RETIRADA:
                modoRetirada();
                break;
            }
            //abrirTrava();
          }else if(response == 403){
            recusaAbertura();
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

    static unsigned long ultimaTroca = 0;
  if (digitalRead(botao) == HIGH && millis() - ultimaTroca > 500) {
    modoAtual = static_cast<Modo>((modoAtual + 1) % 3);
    mensagem_mostrada = false;
    lcd.clear();
    ultimaTroca = millis();
    while (digitalRead(botao) == HIGH);  // debounce
  }


}

void abrirTrava(){
  lcd.clear();
  digitalWrite(green, HIGH);
  digitalWrite(rele, HIGH);
  lcd.setCursor(0, 0);
  lcd.print("Aberto!");
  lcd.setCursor(0, 1);
  lcd.print("Cartao OK");
  //Serial.println("resposta http" + error);
  delay(1000);
  delay(1500);
  Serial.println("leitura ok");
}

void abrirEmergencia(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("EMERGENCIA!");
  digitalWrite(green, HIGH);
  digitalWrite(rele, HIGH);
  delay(2000);  // abre por 5 segundos (ajuste se quiser)
  digitalWrite(green, LOW);
  digitalWrite(rele, LOW);
  mensagem_mostrada = false;
  Serial.println("Lido o cartao de emergencia");
  delay(1000);
  lcd.clear();
}

void recusaAbertura(){
  lcd.clear();
  digitalWrite(green, LOW);
  Serial.println("UID não encontrado!");
  lcd.setCursor(0, 0);
  lcd.print("Cartao nao");
  lcd.setCursor(0, 1);
  lcd.print("encontrado!");
  digitalWrite(red, HIGH);
  delay(2000);
  Serial.println("leitura ok.");
  delay(1500);
}

void resetStatus(){
  HTTPClient httpLimpa;
  httpLimpa.begin(urlResetStatus);
  int respostaLimpeza = httpLimpa.sendRequest("DELETE");

  Serial.println("entrou reset");

  if (respostaLimpeza == 200) {
    Serial.println("Tabela status_abertura limpa com sucesso.");
      Serial.println("entrou 200");
  } else {
      Serial.println("entrou erro");
    Serial.print("Erro ao limpar tabela: ");
    Serial.println(respostaLimpeza);
  }
  delay(1000);
  httpLimpa.end();
}