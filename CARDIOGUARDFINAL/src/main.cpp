#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

// Include helpers for Firebase
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Configuración de WiFi y Firebase
#define WIFI_SSID "NETLIFE-LIMA"
#define WIFI_PASSWORD "0932144009"
#define API_KEY "AIzaSyAqtRhedddscubK3waKtpbtN5QfJd4aRH8"
#define DATABASE_URL "https://esp32-firebase-cardio-default-rtdb.firebaseio.com/"

// Configuración de la pantalla LCD
#define COLUMNS 16
#define ROWS 2
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);

// Configuración de Pines I2C
#define SDA_PIN 19
#define SCL_PIN 18

// Objetos globales
MAX30105 particleSensor;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variables globales para lecturas
int32_t spo2 = 0;
int8_t validSPO2 = 0;
int32_t heartRate = 0;
int8_t validHeartRate = 0;
uint32_t irBuffer[100];
uint32_t redBuffer[100];
int bufferLength = 100;
bool signupOK = false;

// Temporizadores
unsigned long previousLCDMillis = 0;
unsigned long previousFirebaseMillis = 0;
const unsigned long lcdInterval = 500;      // Intervalo de actualización del LCD
const unsigned long firebaseInterval = 10000; // Intervalo de subida a Firebase

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);

    // Inicializar LCD
    while (lcd.begin(COLUMNS, ROWS) != 1) {
        Serial.println(F("Error: LCD no conectada o configuración incorrecta."));
        delay(5000);
    }
    lcd.backlight();
    lcd.clear();
    lcd.print("Iniciando...");

    // Conexión a WiFi
    lcd.clear();
    lcd.print("Conectando WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("WiFi conectado.");
    lcd.clear();
    lcd.print("WiFi conectado!");

    // Configuración de Firebase
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    if (Firebase.signUp(&config, &auth, "", "")) {
        Serial.println("Autenticación exitosa.");
        signupOK = true;
    } else {
        Serial.println("Error en autenticación.");
        Serial.println(config.signer.signupError.message.c_str());
        lcd.clear();
        lcd.print("Firebase Error");
        while (true);
    }
    config.token_status_callback = tokenStatusCallback; // Para depurar tokens
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    lcd.clear();
    lcd.print("Por favor espere");

    // Inicialización del sensor MAX30102
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        lcd.clear();
        lcd.print("Sensor Error");
        while (true); // Detener si no se encuentra el sensor
    }
    particleSensor.setup(60, 4, 2, 100, 411, 4096);
}

void loop() {
    // Leer datos del sensor
    for (byte i = 0; i < bufferLength; i++) {
        while (!particleSensor.available()) {
            particleSensor.check();
        }
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i] = particleSensor.getIR();
        particleSensor.nextSample();
    }
    maxim_heart_rate_and_oxygen_saturation(
        irBuffer, bufferLength, redBuffer,
        &spo2, &validSPO2,
        &heartRate, &validHeartRate
    );

    // Actualizar LCD cada intervalo definido
    if (millis() - previousLCDMillis >= lcdInterval) {
        previousLCDMillis = millis();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("HR:");
        lcd.print(validHeartRate ? String(heartRate) + " bpm" : "---");
        lcd.setCursor(0, 1);
        lcd.print("SpO2:");
        lcd.print(validSPO2 ? String(spo2) + "%" : "---");
    }

    // Subir datos a Firebase cada intervalo definido
    if (Firebase.ready() && signupOK && millis() - previousFirebaseMillis >= firebaseInterval) {
        previousFirebaseMillis = millis();
        if (validHeartRate) {
            if (!Firebase.RTDB.setInt(&fbdo, "signos_vitales/frecuencia_cardiaca", heartRate)) {
                Serial.println("Error subiendo frecuencia cardíaca: " + fbdo.errorReason());
            }
        }
        if (validSPO2) {
            if (!Firebase.RTDB.setInt(&fbdo, "signos_vitales/oxigeno", spo2)) {
                Serial.println("Error subiendo oxígeno: " + fbdo.errorReason());
            }
        }
    }
}
