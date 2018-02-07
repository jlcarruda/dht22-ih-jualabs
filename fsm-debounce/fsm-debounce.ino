
#include "fsm_config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN D3
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define MQTT_KEEPALIVE 5000

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "jualabs";
const char* password = "jualabsufrpe";
const char* mqtt_server = "things.ubidots.com";

WiFiClient espClient;
PubSubClient client(espClient);

const int buttonPin = D7;    // definicao do pino utilizado pelo botao
const int ledPin = D5;       // definicao do pino utilizado pelo led

bool VERBOSE_ERRORS = false;  // Variavel que define se todos os erros serão printados no serial Monitor

String humRequestObj;        // string com obj de request para humidade
String tempRequestObj;       // string com obj de request para temperatura
bool repeatingState = false; // Flag que demonstra se o estado atual veio de um evento de repetição
float hum;
float temp;

//int buttonState = LOW;             // armazena a leitura atual do botao
int lastButtonState;         // armazena a leitura anterior do botao

unsigned long lastDebounceTime = 0;  // armazena a ultima vez que a leitura da entrada variou
unsigned long debounceDelay = 50;    // tempo utilizado para implementar o debounce

event wifi_check_state(void) {
  Serial.println("Wifi Check State");
  delay(10);
  // We start by connecting to a WiFi network

  WiFi.begin(ssid, password);
  delay(3000);
  if(WiFi.status() != WL_CONNECTED){
    return wifi_not_connected;
  } else {
    Serial.println();
    Serial.print("Connected to ");
    Serial.println(ssid);
    return wifi_connected;
  }  
}

event server_check_state(void) {
  Serial.println("Server Check State");
  client.setServer(mqtt_server, 1883);
  if (client.connect("ESP8266Client", "A1E-MNEtItlUDrfu1LHwVdPXnqFrnPHvBA", "")) {
    if(client.connected()){
      return server_connected;
    } 
  } 
  
  return server_not_connected; 
}

event start_state(void) {
//  Serial.println("Start State");
  
  bool btnPressed = false;
  int secCounter = 0;

  
  while(!btnPressed) {
    int buttonState = digitalRead(buttonPin);

    if(buttonState == 0 && lastButtonState == 1){
       lastButtonState = 0;
    } 
    if(buttonState == 1 && lastButtonState == 0){
      lastButtonState = 1;
      Serial.println();  
      Serial.println("-> Button Pressed");
      btnPressed = true;
      return coleta_btn;
    } else if (secCounter == 40){
      Serial.println();  
      secCounter = 0;
      return coleta_time;
      break;
    } else if (secCounter % 4 == 0) {
      Serial.print(".");
    }

    
    
    delay(250);
    secCounter = secCounter + 1;
  }
}


event coleta_state(void) {
  //Serial.println("Coleta");
  hum=dht.readHumidity();
  temp=dht.readTemperature();
  String hum_str = String(hum);
  String temp_str = String(temp);
  
  printHumTemp();
  
  humRequestObj = "{\"value\":" + hum_str + "}";
  tempRequestObj = "{\"value\":" + temp_str + "}";

  // Função que retorna um evento e verifica se alguma das duas requisições ao Ubidots falhou. Se sim, retorna a verificação de conexão wifi se a placa não estiver conectada
  return _sendDataToUbidots();
  
}

event coleta_btn_state(void){
  hum=dht.readHumidity();
  temp=dht.readTemperature();
  String hum_str = String(hum);
  String temp_str = String(temp);
  
  printHumTemp();

  humRequestObj = "{\"value\":" + hum_str + "}";
  tempRequestObj = "{\"value\":" + temp_str + "}";
  
  return _sendDataToUbidotsBTN();

}

event led_on_state(void) {
  digitalWrite(ledPin, LOW);
  if(temp > 22) {
    digitalWrite(ledPin, HIGH);
  }

  return goto_start;
}

event empty_state(void) {
  Serial.println("Empty State - Algo deu errado!");
  return repeat;
}

// ======== Utility Functions ========

// Função que imprime valores no Serial Monitor
void printHumTemp(void) {
  if(!repeatingState){
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.print(" %, Temp: ");
    Serial.print(temp);
    Serial.println(" Celsius");  
  }
}

// Função que retorna evento de acordo com conexão/resposta do servidor Ubidots para requisições sem o envio do botão
event _sendDataToUbidots(void) {
  // Verifica se alguma das duas requisições ao Ubidots falhou. Se sim, retorna a verificação de conexão wifi se a placa não estiver conectada
  bool humPub = client.publish("/v1.6/devices/jualabs-projeto/humidity", humRequestObj.c_str());
  bool tempPub = client.publish("/v1.6/devices/jualabs-projeto/temperature", tempRequestObj.c_str());
  if(!humPub || !tempPub) {
    
    if(VERBOSE_ERRORS){
      Serial.print("-> Failed to publish data to UBIDOTS server: humidity -> ");
      Serial.print(humPub);
      Serial.print(" ; temperature -> ");
      Serial.println(tempPub);
    }
    
    if(!client.connected()){
      _MQTTErrorHandler();
      repeatingState = true;
      return repeat;
    } else {
      return _MQTTErrorHandler();
    }
  }
  if(repeatingState){
    repeatingState = false;
    
    if(VERBOSE_ERRORS) {
      Serial.println("--> State repeat completed successfully");  
    }
  }
  return goto_led;
}

// Mesma função que _sendDataToUbidots, mas com o envio do valor de contador para o botão
event _sendDataToUbidotsBTN(void) {
  event serverReturn = _sendDataToUbidots();
  bool btnPub = client.publish("/v1.6/devices/jualabs-projeto/counter", "{\"value\":1}");
  
  if(serverReturn != goto_led){
    return serverReturn;
    
  } else if(client.publish("/v1.6/devices/jualabs-projeto/counter", "{\"value\":1}")) {
    if(repeatingState){
      repeatingState = false;
      
      if(VERBOSE_ERRORS) {
        Serial.println("--> State repeat completed successfully");  
      }
    }
    return goto_led;
    
  } else if(!client.connected()) {
    _MQTTErrorHandler();
    repeatingState = true;
    return repeat;  
  
  } else {
    return _MQTTErrorHandler();
  }
  
}

// Função que faz o handling de erros que podem ocorrer ao tentar se conectar com UBIDOTS
event _MQTTErrorHandler(void) {
    String error;
    int state = client.state();
    if(state == MQTT_CONNECTION_TIMEOUT){
      error = "--> Cause: Connection Timeout to server ...";
      
    } else if (state == MQTT_CONNECTION_LOST){
      error = "--> Cause: Connection Lost. Reconnecting to Client and repeating state ...";
      if(VERBOSE_ERRORS){
        Serial.println(error);  
      }
      return _clientConnect();
   
    } else if (state == MQTT_CONNECT_FAILED){
      error = "--> Cause: Failed to Connect ..." ;
    
    } else if (state == MQTT_DISCONNECTED){
      error = "--> Cause: Disconnected from server ...";
    
    } else if (state == MQTT_CONNECT_BAD_PROTOCOL){
      error = "--> Cause: Server doesn't support MQTT version ...";
    
    } else if (state == MQTT_CONNECT_BAD_CLIENT_ID){
      error = "--> Cause: Server Rejected client ID ...";
    
    } else if (state == MQTT_CONNECT_UNAVAILABLE){
      error = "--> Cause: Server was unable to accept connection ...";
    
    } else if (state == MQTT_CONNECT_BAD_CREDENTIALS){
      error = "--> Cause: Wrong Credentials ...";
    
    } else if (state == MQTT_CONNECT_UNAUTHORIZED){
      error = "--> Cause: Client wasn't authorized to connect ...";
    
    } else {
      error = "---> Cause: Unknown Error ... ";  
    }
  
    if(VERBOSE_ERRORS){
      Serial.println(error);  
    }
    return wifi_not_connected; // Retorna para o estado de checagem de Wifi
}

event _clientConnect(void){
  if (client.connect("ESP8266Client", "A1E-MNEtItlUDrfu1LHwVdPXnqFrnPHvBA", "")) {
    if(client.connected()){
      return server_connected;
    } 
  }
     return server_not_connected;
}

// variaveis que armazenam estado atual, evento atual e funcao de tratamento do estado atual
state cur_state = ENTRY_STATE;
event cur_evt;
event (* cur_state_function)(void);

void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  dht.begin();
}

void loop() {
  cur_state_function = state_functions[cur_state];
  cur_evt = (event) cur_state_function();
  if (EXIT_STATE == cur_state)
    return;
  cur_state = lookup_transitions(cur_state, cur_evt);
}
