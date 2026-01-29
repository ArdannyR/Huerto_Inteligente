// Credenciales de Blynk
#define BLYNK_TEMPLATE_NAME "ESP32 IoT Test"
#define BLYNK_AUTH_TOKEN "7Y6dJ0CPHGPWHrjEnZXI4gcyI_cdNlCp"
#define BLYNK_TEMPLATE_ID "TMPL20JqYUwWY"

// Credenciales wifi
char ssid[] = "POCO F4 GT";
char pass[] = "arev2005";

// Liberias
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>

// Pins de servos
const int pinServo1 = 13; 
const int pinServo2 = 14; 

Servo servoA;
Servo servoB;

// angulos fijos
const int CERRADO = 90;
const int ABIERTO_A = 180; 
const int ABIERTO_B = 0;   


BLYNK_WRITE(V0) {
  int valorBoton = param.asInt(); 

  if (valorBoton == 1) {
    Serial.println("Abriendo techos");
    servoA.write(ABIERTO_A);
    servoB.write(ABIERTO_B);
  } else {
    Serial.println("Cerrando techos");
    servoA.write(CERRADO);
    servoB.write(CERRADO);
  }
}

void setup() {
  Serial.begin(115200);

  // Configuración de Servos
  servoA.setPeriodHertz(50);
  servoA.attach(pinServo1, 500, 2400);
  servoB.setPeriodHertz(50);
  servoB.attach(pinServo2, 500, 2400);

  // Posición inicial: Cerrado
  servoA.write(CERRADO);
  servoB.write(CERRADO);

  // Conexión a Blynk (Esto también conecta el WiFi)
  Serial.println("Conectando a Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Serial.println("✅ Conectado y listo.");
}

void loop() {
  Blynk.run();
}