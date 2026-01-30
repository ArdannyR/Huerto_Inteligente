// Librerias
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <DFPlayerMini_Fast.h>

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
SoftwareSerial EspSerial(2, 3);     // RX, TX para ESP8266
SoftwareSerial DFSerial(5, 6);      // RX, TX para DFPlayer Mini
DHT dht(pinDHT, DHT11);
Adafruit_BMP280 bmp;   // Prot. I2C (SDA=A4, SCL=A5)
DFPlayerMini_Fast myMP3;

// Variables de ayuda
bool primeraLectura = true;

// Funcion Setup
void setup() {
  Serial.begin(9600);
  EspSerial.begin(9600);
  DFSerial.begin(9600);
  
  Serial.println(F("\n--- INICIANDO SISTEMA Agreenbyte ---"));
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, LOW);
  Serial.println(F("- Esperando estabilización de voltaje..."));
  delay(3000); // 3 segundos para regular voltaje

  // Inicializar DFPlayer Mini
  Serial.println(F("- Inicializando DFPlayer Mini..."));
  if (myMP3.begin(DFSerial)) {
    Serial.println(F("- DFPlayer Mini OK"));
    myMP3.volume(20); 
    delay(500);
    reproducirPista(8); // Pista 8 - "Iniciando"
  } else {
    Serial.println(F(" Error: DFPlayer Mini no responde"));
  }

  dht.begin();

  if (!bmp.begin(0x76)) {
    Serial.println(F(" Error: BMP280 no encontrado."));
  }

  configurarWiFi();
}

// Funcion Loop
void loop() {
  // Lectura
  float t_dht = lectura_dht_temp();
  float h_dht = lectura_dht_hum();
  float pres  = lectura_bmp_presion();
  float t_bmp = lectura_bmp_temp();
  int suelo   = lectura_fc28();
  int luz     = lectura_ldr();

  // Serial pre envio
  Serial.println(F("\n----------- REPORTE ACTUAL -----------"));
  Serial.print(F("(DHT11) Temp interna: ")); Serial.print(t_dht); Serial.println(" C");
  Serial.print(F("(BMP280) Temp externa: ")); Serial.print(t_bmp); Serial.println(" C"); 
  Serial.print(F("(DHT11) Humedad: ")); Serial.print(h_dht); Serial.println(" %");
  Serial.print(F("(FC28) Suelo:   ")); Serial.print(suelo); Serial.println(" %");
  Serial.print(F("(LDR) Luz:     ")); Serial.println(luz);
  Serial.print(F("(BMP280) Presion: ")); Serial.print(pres); Serial.println(" hPa");
  Serial.println(F("--------------------------------------"));

  // Verificar condiciones y reproducir alertas
  verificarAlertas(t_dht, h_dht, pres, suelo, t_bmp);

  // Envio a ThingSpeak
  enviarAThingSpeak(t_dht, h_dht, suelo, luz, pres, t_bmp);
  Serial.println(F("-> Esperando 20 seg para siguiente envio..."));
  delay(20000); 
}

// Funcion de alertas
void verificarAlertas(float temp, float hum, float presion, int suelo, float temp_bmp) {
  // Pista 4 - Temperatura alta BMP280 
  if (temp_bmp > 30.0) {
    Serial.println(F("⚠️ ALERTA: Temperatura alta (BMP280)"));
    reproducirPista(4);
    delay(2000);
  }

  // Pista 5 - Humedad alta DHT11 
  if (hum > 80.0) {
    Serial.println(F("⚠️ ALERTA: Humedad alta (DHT11)"));
    reproducirPista(5);
    delay(2000);
  }

  // Pista 6 - Humedad baja DHT11 
  if (hum < 30.0) {
    Serial.println(F("⚠️ ALERTA: Humedad baja (DHT11)"));
    reproducirPista(6);
    delay(2000);
  }
  
    // Pista 2 - Suelo seco FC28 
  if (suelo < 15) {
    Serial.println(F("⚠️ ALERTA: Suelo seco detectado (FC28)"));
    reproducirPista(2);
    delay(2000); 
    
    Serial.println(F("💦 Activando bomba..."));
    digitalWrite(pinRelay, HIGH);
    delay(2000); 
    digitalWrite(pinRelay, LOW);
    Serial.println(F("🛑 Bomba apagada."));
    delay(1000); 
  }
  primeraLectura = false;
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
  return bmp.readPressure() / 100.0F; // explica este 
}
float lectura_bmp_temp() {
  return bmp.readTemperature(); 
}
int lectura_fc28() {
  int lectura = analogRead(pinSuelo);
  int valorSeco = 1023; 
  int valorMojado = 400;  
  int porcentaje = map(lectura, valorSeco, valorMojado, 0, 100);
  porcentaje = constrain(porcentaje, 0, 100);
  return porcentaje;
}
int lectura_ldr() {
  return analogRead(pinLDR);
}

// Función de musica
void reproducirPista(int pista) {
  Serial.print(F("Reproduciendo pista: "));
  Serial.println(pista);
  myMP3.play(pista);
}

// Conexion a Wifi
void configurarWiFi() {
  Serial.println(F("-> Conectando WiFi..."));
  sendCmd("AT+CWMODE=1", 1000);
  sendCmd("AT+CIPMUX=0", 1000);
  String cmd = "AT+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASS "\"";
  sendCmd(cmd, 8000);
  Serial.println(F("-> WiFi Configurado."));
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

// Envio a ThingSpeak
void enviarAThingSpeak(float temp, float hum, int suelo, int luz, float pres, float temp_bmp) {
  EspSerial.listen();
  sendCmd("AT+CIPCLOSE", 500); 
  Serial.println(F("=== Subiendo datos ==="));

  String cmdConexion = "AT+CIPSTART=\"TCP\",\"" HOST "\",80";
  EspSerial.println(cmdConexion);
  delay(2000); 

  String datos = "&field1=" + String(temp, 1) +
                 "&field2=" + String(hum, 1) +
                 "&field3=" + String(suelo) +
                 "&field4=" + String(luz) +
                 "&field5=" + String(pres, 1) +
                 "&field6=" + String(temp_bmp, 1);

  String peticion = "GET /update?api_key=" API_KEY + datos + " HTTP/1.1\r\n" + 
                    "Host: " HOST "\r\n" + 
                    "Connection: close\r\n\r\n";

  EspSerial.print("AT+CIPSEND=");
  EspSerial.println(peticion.length());
  delay(1000); 

  EspSerial.print(peticion);

  Serial.println(F("☁️ Datos enviados (Sin esperar cierre)."));
}