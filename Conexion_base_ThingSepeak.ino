// Librerias
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>

// Definicion de ciertos parametros
#define WIFI_SSID "POCO F4 GT" 
#define WIFI_PASS "arev2005"   
#define API_KEY   "DY4ALPNFXO9UNA7U" 
#define HOST      "api.thingspeak.com"

// Configuración de Pines y demas Objetos
const int pinDHT = 4;
const int pinLDR = A0;
const int pinSuelo = A1;
const int pinRelay = 7; 
SoftwareSerial EspSerial(2, 3); // RX, TX
DHT dht(pinDHT, DHT11);
Adafruit_BMP280 bmp; // Prot. I2C (SDA=A4, SCL=A5)

// Funcion Setup
void setup() {
  Serial.begin(9600);
  EspSerial.begin(9600);
  
  Serial.println(F("\n--- INICIANDO SISTEMA 6 CAMPOS ---"));

  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, LOW);

  dht.begin();

  if (!bmp.begin(0x76)) { //porisacaso no este el bmp
    Serial.println(F("⚠️ Error: BMP280 no encontrado."));
  }

  configurarWiFi();
}

// Funcion Loop
void loop() {
  // Lectura
  float t_dht = lectura_dht_temp();
  float h_dht = lectura_dht_hum();
  float pres  = lectura_bmp_presion();
  float t_bmp = lectura_bmp_temp(); // <--- Nueva lectura BMP
  int suelo   = lectura_fc28();
  int luz     = lectura_ldr();

  // Serial pre envio
  Serial.println(F("\n----------- REPORTE ACTUAL -----------"));
  Serial.print(F("(DHT11) Temp interna:")); Serial.print(t_dht); Serial.println(" C");
  Serial.print(F("(BMP280) Temp extrena:")); Serial.print(t_bmp); Serial.println(" C"); 
  Serial.print(F("(DHT11) Humedad: ")); Serial.print(h_dht); Serial.println(" %");
  Serial.print(F("(FC28) Suelo:   ")); Serial.print(suelo); Serial.println(" %");
  Serial.print(F("(LDR) Luz:     ")); Serial.println(luz);
  Serial.print(F("(BMP280) Presion: ")); Serial.print(pres); Serial.println(" hPa");
  Serial.println(F("--------------------------------------"));

  // Envio a ThingSpeak (Ahora con t_bmp al final)
  enviarAThingSpeak(t_dht, h_dht, suelo, luz, pres, t_bmp);

  // Esperar los 20 segundos por cosas de TS y 
  Serial.println(F("-> Esperando 20 seg para siguiente envio..."));
  esperarYEscuchar(20000); 
}

// Funcion de ayuda para bomba
void esperarYEscuchar(unsigned long tiempoEspera) {
  Serial.println(F("   (Ingrese 'a' en cualquier momento para regar)"));
  
  unsigned long tiempoInicio = millis(); // Nota: millis es solo para medir tiempos, como tal el dalay detiene todo, este solo da un registro del t que pasa
  
  // Mientras no hayan pasado los 20 segundos (tiempoEspera)
  while (millis() - tiempoInicio < tiempoEspera) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == 'a') {
        activarBomba();
      }
    }
    // Pequeño retardo para no saturar el procesador
    delay(10); 
  }
}

// Lectura de sensores
float lectura_dht_temp() {
  float t = dht.readTemperature();
  if (isnan(t)) return 0.0; 
  return t;
}
float lectura_dht_hum() {
  float h = dht.readHumidity();
  if (isnan(h)) return 0.0;
  return h;
}
float lectura_bmp_presion() {
  return bmp.readPressure() / 100.0F; // hPa
}
float lectura_bmp_temp() {
  return bmp.readTemperature(); 
}
int lectura_fc28() {
  int lectura = analogRead(pinSuelo);
  int porcentaje = map(lectura, 1023, 300, 0, 100);
  return constrain(porcentaje, 0, 100);
}
int lectura_ldr() {
  return analogRead(pinLDR);
}

// Apartado de actuadores
void activarBomba() {
  Serial.println(F("\n💦 COMANDO RECIBIDO: Accionando bomba 2 seg..."));
  digitalWrite(pinRelay, HIGH); // Enciende Bomba
  delay(1000);                  // Mantiene prendida 2 segundos
  digitalWrite(pinRelay, LOW);  // Apaga Bomba
  Serial.println(F("🛑 Bomba apagada. Retomando espera...\n"));
}
void dftPlayer(int pista){
  //later
}

// Conexion a Wifi
void configurarWiFi() {
  Serial.println(F("📡 Conectando WiFi..."));
  sendCmd("AT+CWMODE=1", 1000);
  sendCmd("AT+CIPMUX=0", 1000);
  String cmd = "AT+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASS "\"";
  sendCmd(cmd, 8000);
  Serial.println(F("✅ WiFi Configurado."));
}

// Envio de comandos
void sendCmd(String cmd, int timeout) {
  EspSerial.println(cmd);
  long int time = millis();
  while ((time + timeout) > millis()) {
    while (EspSerial.available()) {
      EspSerial.read(); 
    }
  }
}

// Envio a ThingSpeak ACTUALIZADO CON FIELD 6
void enviarAThingSpeak(float temp, float hum, int suelo, int luz, float pres, float temp_bmp) {
  EspSerial.listen();
  sendCmd("AT+CIPCLOSE", 500); 
  Serial.println(F("☁️ Subiendo datos..."));

  // 1. Conectar
  String cmdConexion = "AT+CIPSTART=\"TCP\",\"" HOST "\",80";
  EspSerial.println(cmdConexion);
  delay(2000); 

  // 2. Crear peticion (Agregado field6)
  String datos = "&field1=" + String(temp, 1) +
                 "&field2=" + String(hum, 1) +
                 "&field3=" + String(suelo) +
                 "&field4=" + String(luz) +
                 "&field5=" + String(pres, 1) +
                 "&field6=" + String(temp_bmp, 1);

  String peticion = "GET /update?api_key=" API_KEY + datos + " HTTP/1.1\r\n" + 
                    "Host: " HOST "\r\n" + 
                    "Connection: close\r\n\r\n";

  // 3. Enviar Longitud
  EspSerial.print("AT+CIPSEND=");
  EspSerial.println(peticion.length());
  delay(1000); 

  // 4. Enviar Datos
  EspSerial.print(peticion);

  Serial.println(F("☁️ Datos enviados (Sin esperar cierre)."));
}