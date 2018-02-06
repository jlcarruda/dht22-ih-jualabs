
#include "fsm_config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN D3
#define DHTTYPE DHT22   // DHT 22  (AM2302)

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "jualabs";
const char* password = "jualabsufrpe";
const char* mqtt_server = "things.ubidots.com";

WiFiClient espClient;
PubSubClient client(espClient);

const int buttonPin = D7;    // definicao do pino utilizado pelo botao
const int ledPin = D5;       // definicao do pino utilizado pelo led

float hum;
float temp;
int buttonState = LOW;             // armazena a leitura atual do botao
int lastButtonState = LOW;         // armazena a leitura anterior do botao
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

// So entra se wifi_connected
event server_check_state(void) {
  Serial.println("Server Check State");
  client.setServer(mqtt_server, 1883);
  if (client.connect("ESP8266Client", "A1E-MNEtItlUDrfu1LHwVdPXnqFrnPHvBA", "")) {
    if(client.connected()){
      return server_connected;
    } else {
      return server_not_connected;  // retorna pro wifi_check_state
    }
  }
}

event start_state(void) {
  Serial.println("Start State");
  Serial.println(dht.readHumidity());
  
  bool btnPressed = false;
  int secCounter = 0;

  
  while(!btnPressed) {
    int buttonState = digitalRead(buttonPin);  
    if(buttonState == 1){
      Serial.println("button state pressed");
      btnPressed = true;
      return coleta_btn;
    } else if (secCounter == 20){
      Serial.println("time event");
      secCounter = 0;
      return coleta_time;
      break;
    }
    
    delay(500);
    secCounter = secCounter + 1;
  }

  
//  Serial.println("Start State");
//  digitalWrite(ledPin, HIGH);
//  delay(1000);
//  digitalWrite(ledPin, LOW);
//  delay(5000);
//  return repeat;
}


event coleta_state(void) {
  //Serial.println("Coleta");
  hum=dht.readHumidity();
  temp=dht.readTemperature();

  char valueHum[12];
  char valueTemp[12];
  
  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.print(" %, Temp: ");
  Serial.print(temp);
  Serial.println(" Celsius");

  dtostrf(hum, 7, 3, valueHum);
  dtostrf(temp, 7, 3, valueTemp);
  
//  sprintf(valueHum, "%f", hum);
//  sprintf(valueTemp, "%f", temp);
  Serial.println(strcat("value ", valueHum));
//  client.publish("/v1.6/devices/jualabs-projeto/humidity", valueHum);
//  client.publish("/v1.6/devices/jualabs-projeto/temperature", valueTemp);
  
  return goto_led;
}

event coleta_btn_state(void){
  Serial.println("ColetaBTN");
  hum=dht.readHumidity();
  temp=dht.readTemperature();

  char valueHum[50];
  char valueTemp[50];
  
  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.print(" %, Temp: ");
  Serial.print(temp);
  Serial.println(" Celsius");

  sprintf(valueHum, "%f", hum);
  sprintf(valueTemp, "%f", temp);

  //client.publish("/v1.6/devices/jualabs-projeto/humidity", valueHum);
  //client.publish("/v1.6/devices/jualabs-projeto/temperature", valueTemp);
//  client.publish("/v1.6/devices/jualabs-projeto/counter", "{\"value\":1}")
  
  return goto_led;
}

event led_on_state(void) {
  digitalWrite(ledPin, LOW);
  if(temp > 22) {
    digitalWrite(ledPin, HIGH);
  }

  return goto_start;
}

event empty_state(void) {
  Serial.println("Empty State");
  return repeat;
}

// variaveis que armazenam estado atual, evento atual e funcao de tratamento do estado atual
state cur_state = ENTRY_STATE;
event cur_evt;
event (* cur_state_function)(void);

//// implementacao de funcoes auxiliares
//int read_button() {
//  int reading = digitalRead(buttonPin);
//
//  if (reading != lastButtonState) {
//    lastDebounceTime = millis();
//  }
//  
//  lastButtonState = reading;
//  
//  if ((millis() - lastDebounceTime) > debounceDelay) {
//    if (reading != buttonState) {
//      buttonState = reading;
//      if (buttonState == HIGH) {
//        return true;
//      }
//    }
//  }
//  return false;
//}

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
