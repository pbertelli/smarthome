#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#define RELAY_PIN_1 0
#define RELAY_PIN_2 2
#include "fauxmoESP.h"
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>


#define SERIAL_BAUDRATE 115200

#define WIFI_SSID "PEDRO E BRUNA"
#define WIFI_PASS "tanageladeira"

// Define Firebase API Key, Project ID, and user credentials
#define API_KEY "AIzaSyC0yYG2RuBWnm5DNryPNg-9Z4eer1STBUk"
#define FIREBASE_PROJECT_ID "smarthome-328d7"
#define USER_EMAIL "admin@admin.com"
#define USER_PASSWORD "123456"

bool chamadaAlexa;
bool state_chamadaAlexa;
bool flag_timer = false;
unsigned long t_inicial = millis();

// Define Firebase Data object, Firebase authentication, and configuration
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

#define TOM "Tv"
#define LAMP "Tomada Central"

fauxmoESP fauxmo;

// Conexão WiFi
void wifiSetup(){

  //Define o como STA
  WiFi.mode(WIFI_STA);

  //Conecta 
  Serial.printf("[WIFI] Conectado ao %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Espera 
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  //Conectado
  Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(),WiFi.localIP().toString().c_str());
  }
  
void setup() {
  // Inicia a Serial 
  Serial.begin(SERIAL_BAUDRATE);
  Serial.println();

  // Conexão WiFi
  wifiSetup();

  //Dispositivos a serem ligados
  pinMode(RELAY_PIN_1, OUTPUT);
  digitalWrite(RELAY_PIN_1, HIGH);

  pinMode(RELAY_PIN_2, OUTPUT);
  digitalWrite(RELAY_PIN_2,HIGH);
 
  // Print Firebase client version
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    // Assign the API key
  config.api_key = API_KEY;

  // Assign the user sign-in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the callback function for the long-running token generation task
  config.token_status_callback = tokenStatusCallback;  // see addons/TokenHelper.h

  // Begin Firebase with configuration and authentication
  Firebase.begin(&config, &auth);

  // Reconnect to Wi-Fi if necessary
  Firebase.reconnectWiFi(true);


  // Por padrão, fauxmoESP cria seu próprio servidor web na porta definida
  // A porta TCP deve ser 80 para dispositivos gen3 (o padrão é 1901)
  // Isso deve ser feito antes da chamada enable()
  fauxmo.createServer(true); // Cria o servidor
  fauxmo.setPort(80); // Necessário para os dispositivos gen3

  // Você deve chamar enable(true) assim que tiver uma conexão WiFi
  // Você pode ativar ou desativar a biblioteca a qualquer momento
  // Desativá-lo impedirá que os dispositivos sejam descobertos e trocados
  fauxmo.enable(true);

  // Você pode usar diferentes maneiras de chamar a Alexa para modificar o estado dos dispositivos:
  // "Alexa, ligar Ventilador"

  // Adiciona os dispositivos 
  fauxmo.addDevice(TOM);
  fauxmo.addDevice(LAMP);

  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
    // Retorno de chamada quando um comando da Alexa é recebido.
    // Você pode usar device_id ou device_name para escolher o elemento no qual realizar uma ação (relé, LED, ...)
    // O state é um booleano (ON / OFF) e value um número de 0 a 255 (se você disser "definir a luz da cozinha para 50%", receberá 128 aqui).

    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
    if ( (strcmp(device_name, TOM) == 0) ) {
      Serial.println("RELAY 1 switched by Alexa");
      //digitalWrite(RELAY_PIN_1, !digitalRead(RELAY_PIN_1));
      if (state) {
        chamadaAlexa = true;
        Serial.print("Tom - true");
        //digitalWrite(RELAY_PIN_1, LOW);
      } else {
        chamadaAlexa = false;
        Serial.print("Tom - false");
        //digitalWrite(RELAY_PIN_1, HIGH);
      }
    }
    if ( (strcmp(device_name, LAMP) == 0) ) {
      // Esse comando define a variavel da função para realizar alguma ação
      Serial.println("RELAY 2 switched by Alexa");
      if (state) {
        Serial.print("Lamp - true");
        chamadaAlexa = true;
      } else {
        Serial.print("Lamp - false");
        chamadaAlexa = false;
      }
    }
  });


}

void loop() {

  String documentPath = "dispositivos/YZCWdk5Ti9gxGwTDAlkp";
  FirebaseJson content;
  bool flag = true;

  // testando metodo set
  if(chamadaAlexa != state_chamadaAlexa) {
    Serial.print("Update Data... ");

    // Setando valores de "key" e "timer"
    if(chamadaAlexa) {
      content.set("fields/chave/booleanValue", true);
      content.set("fields/timer/booleanValue", false);  
      digitalWrite(RELAY_PIN_2, LOW);
      digitalWrite(RELAY_PIN_1, LOW);
    } else {
      content.set("fields/chave/booleanValue", false);
      content.set("fields/timer/booleanValue", false);
      digitalWrite(RELAY_PIN_1, HIGH);
      digitalWrite(RELAY_PIN_2, HIGH);
    }

    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "chave") && Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "timer")) {
      Serial.print("Set ok ");
      //Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    } else {
            Serial.println(fbdo.errorReason());
    }
    state_chamadaAlexa = chamadaAlexa;
  } else{
    Serial.println("No Update Data... ");
  }

  //testanto o metodo get
  if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())) {
    Serial.println("Recebido");
    
    FirebaseJson payload;
    payload.setJsonData(fbdo.payload().c_str());

    FirebaseJsonData jsonData;
    payload.get(jsonData, "fields/timerValue/stringValue", true);
    String timerValueStr = jsonData.stringValue;
    int timerValue = timerValueStr.toInt();

    payload.get(jsonData, "fields/chave/booleanValue", true);
    //Serial.println(jsonData.boolValue);
    bool chave = jsonData.boolValue;

    payload.get(jsonData, "fields/timer/booleanValue", true);
    //Serial.println(jsonData.stringValue);
    bool timer = jsonData.boolValue;


    // Faça o que quiser com os dados lidos
    Serial.println("Novos dados recebidos:");
    Serial.println(timerValueStr);
    Serial.println(chave);
    Serial.println(timer);
    Serial.println(timerValue);

    Serial.println(millis()/100);
    

    if (chave){
      digitalWrite(RELAY_PIN_2, LOW);
      digitalWrite(RELAY_PIN_1, LOW);
      Serial.println("Chave on");
      if(timer){
        if(flag_timer == false){
          flag_timer = true;
          t_inicial = millis();
          Serial.print("t_inicial:");
          Serial.println(t_inicial);
        } else {
          Serial.print("diferenca (em s):");
          Serial.println((millis() - t_inicial)/1000);
        }

        if (millis() - t_inicial >= timerValue * 1000) {
          flag_timer = false;
          Serial.print("|| t_inicial:");
          Serial.println(t_inicial);
          Serial.print("|| timerValue:");
          Serial.println(timerValue);
          digitalWrite(RELAY_PIN_2, LOW);
          digitalWrite(RELAY_PIN_1, LOW);

          content.set("fields/chave/booleanValue", false);
          content.set("fields/timer/booleanValue", false);
          
          if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "chave") && Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "timer")) {
            Serial.print("Set ok ");
            //Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
          } else {
            Serial.println(fbdo.errorReason());
          }
        }

      }
    } else {
      digitalWrite(RELAY_PIN_2, HIGH);
      digitalWrite(RELAY_PIN_1, HIGH);
      Serial.println("Chave off");
    }
      
  } else{
    Serial.printf("método get falhou");
  }


  // fauxmoESP usa um servidor TCP assíncrono, mas um servidor UDP sincronizado
  // Portanto, temos que pesquisar manualmente os pacotes UDP
  fauxmo.handle();

  static unsigned long last = millis();
  if (millis() - last > 5000) {
    last = millis();
    Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
  }

}