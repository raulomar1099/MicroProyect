#include <Arduino.h>
#include <WiFi.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <PubSubClient.h>
#include <driver/adc.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

//*******************//
//****** TEMP CONFIGURATION *******//
//*******************//
#define PIN_ANALOG_IN   33
#define OLED_MOSI 23
#define OLED_CLK 18
#define OLED_DC 3
#define OLED_CS 5
#define OLED_RESET 1
int adcValue ;
double voltage ;
double Rt ;
double tempK ;
double tempC ;
double tempf ;          

//*****Calibration Variable*****//
double Calibration_vC = 5.79;
double Calibration_vF = 8.4;

//*******************//
//****** WIFI CONFIGURATION *******//
//*******************//
const char* ssid = "iPhone";
const char* password =  "12345678";

//*******************//
//****** MQTT CONFIGURATION *******//
//*******************//
const char *mqtt_server = "54.175.97.105";
const int mqtt_port = 1883;
const char *mqtt_user = "INOVA_USER";
const char *mqtt_pass = "INOVA";
const char *root_topic_subscribe = "room2";
const char *root_topic_publish = "room2";
const char *root_topic_subscribe2 = "room2C";
const char *root_topic_publish2 = "room2C";
//*******************//
//******* GLOBAL **********//
//*******************//
WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
char msg[16];
char msg2[16];
long count=0;

//*******************//
//******* FUNCTIONS *********//
//*******************//
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void setup_wifi();

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  
  //***Display*******//

 display.begin(SSD1306_SWITCHCAPVCC); // Inicia el display OLED
 display.clearDisplay(); // Borrar imagen en el OLED
 display.setTextSize(1.5); // Tamaño del texto
 display.setTextColor(WHITE); // Definir color del texto (WHITE-BLACK)
 display.setCursor(10,8); // Definir posición Columna, Fila 
 display.println("Proyecto Micro"); // Carga la información al buffer
 display.display(); // Actualiza display con datos en Buffer
 delay(3000); 

 display.clearDisplay(); 
 display.setTextSize(1.5); 
 display.setTextColor(WHITE); 
 display.setCursor(10,8); 
 display.println("Is the A/C on?"); 
 display.display(); 
 delay(3000); 
}

void loop() {
  
  if (!client.connected()) {
		reconnect();
	}

  if (client.connected()){

    //TEMPERATURE READING

    adcValue = analogRead(PIN_ANALOG_IN);                    //read ADC pin
    voltage = (float)adcValue / 4095.0 * 3.3;                // calculate voltage
    Rt = 10 * voltage / (3.3 - voltage);                     //calculate resistance value of thermistor
    tempK = 1 / (1 / (273.15 + 25) + log(Rt / 10) / 3950.0); //calculate tempera0ture (Kelvin)
    tempC = (tempK - 273.15) - Calibration_vC;               //calculate temperature (Celsius)
    tempf = (1.8 * (tempK - 273.15) + 32) - Calibration_vF;  // calculate temp (fahrenheit)
    

    display.clearDisplay(); 
    display.setTextSize(1);
    display.setCursor(0, 0);       
    display.println("Temp Room:"); 
    display.setCursor(65, 0);
    display.println(tempC);
    display.setCursor(100, 0);
    display.println("C");
    display.setCursor(0, 10);
    display.println("Temp Room:");
    display.setCursor(65, 10);
    display.println(tempf);
    display.setCursor(100, 10);
    display.println("F");
    display.display(); 
    delay(500); 
    
    snprintf(msg, 16, "%.2f", tempf);
    client.publish(root_topic_publish, msg);
    snprintf(msg2, 16, "%.2f", tempC);
    client.publish(root_topic_publish2, msg2);

    
    delay(1000);
  }
  client.loop();
  
}

//*******************//
//****** WIFI CONNECTION ********//
//*******************//
void setup_wifi(){
	delay(10);
	// Nos conectamos a nuestra red Wifi
	Serial.println();
	Serial.print("Conectando a ssid: ");
	Serial.println(ssid);

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.println("Conectado a red WiFi!");
	Serial.println("Dirección IP: ");
	Serial.println(WiFi.localIP());
}

//*******************//
//****** MQTT CONNECTION ********//
//*******************//
void reconnect() {

	while (!client.connected()) {
		Serial.print("Intentando conexión Mqtt...");
		// Creamos un cliente ID
		String clientId = "IOTICOS_H_W_";
		clientId += String(random(0xffff), HEX);
		// Intentamos conectar
		if (client.connect(clientId.c_str(),mqtt_user,mqtt_pass)) {
			Serial.println("Conectado!");
			// Nos suscribimos
			if(client.subscribe(root_topic_subscribe)){
        Serial.println("Suscripcion ok");
      }else{
        Serial.println("fallo Suscripciión");
      }
		} else {
			Serial.print("falló :( con error -> ");
			Serial.print(client.state());
			Serial.println(" Intentamos de nuevo en 5 segundos");
			delay(5000);
		}
    if (client.connect(clientId.c_str(),mqtt_user,mqtt_pass)) {
			Serial.println("Conectado!");
			// Nos suscribimos
			if(client.subscribe(root_topic_subscribe2)){
        Serial.println("Suscripcion ok");
      }else{
        Serial.println("fallo Suscripciión");
      }
		} else {
			Serial.print("falló :( con error -> ");
			Serial.print(client.state());
			Serial.println(" Intentamos de nuevo en 5 segundos");
			delay(5000);
		}
	}
}

//*******************//
//******* CALLBACK **********//
//*******************//
void callback(char* topic, byte* payload, unsigned int length){
	String incoming = "";
	Serial.print("Mensaje recibido desde -> ");
	Serial.print(topic);
	Serial.println("");
	for (int i = 0; i < length; i++) {
		incoming += (char)payload[i];
	}
	incoming.trim();
	Serial.println("Mensaje -> " + incoming);

}