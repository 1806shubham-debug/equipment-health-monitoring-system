#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_ID"
#define BLYNK_TEMPLATE_NAME "Equipment Moniter"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_TOKEN"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <DHT.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// ---------------- WIFI for wokwi ----------------
//char ssid[] = "Wokwi-GUEST";
//char pass[] = "";

char ssid[] = "YOUR_WIFI";
char pass[] = "YOUR_PASSWORD";

// ---------------- DHT22 ----------------
#define DHTPIN 15
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// ---------------- LEDs ----------------
#define GREEN_LED 25
#define RED_LED 26

// ---------------- BUZZER ----------------
#define BUZZER_PIN 5

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- MPU6050 ----------------
Adafruit_MPU6050 mpu;

// ---------------- ALERT STATE ----------------
bool previousAlertState = false;
unsigned long previousBlinkMillis = 0;
bool redLedState = false;

// ---------------- SETUP ----------------
void setup() {

  Serial.begin(115200);

  // LCD
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  // WiFi
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");

  lcd.clear();
  lcd.print("WiFi Connected");

  // Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // I2C
  Wire.begin(21, 22);

  // DHT
  dht.begin();

  // MPU6050
  if (!mpu.begin()) {

    Serial.println("MPU6050 NOT FOUND");

    lcd.clear();
    lcd.print("MPU ERROR");

    while (1);
  }

  // Pins
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Startup State
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Equipment");

  lcd.setCursor(0, 1);
  lcd.print("Monitoring");

  delay(2000);

  lcd.clear();

  Serial.println("SYSTEM READY");
}

// ---------------- LOOP ----------------
void loop() {

  Blynk.run();

  // ---------------- READ DHT ----------------
  float temp = dht.readTemperature();

  if (isnan(temp)) {

    Serial.println("DHT ERROR");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("DHT ERROR");

    delay(1000);
    return;
  }

  // ---------------- READ MPU6050 ----------------
  sensors_event_t a, g, t;

  mpu.getEvent(&a, &g, &t);

  // ---------------- CALCULATE VIBRATION ----------------
  float vibration =
    abs(a.acceleration.x) +
    abs(a.acceleration.y) +
    abs(a.acceleration.z);

  // ---------------- SERIAL OUTPUT ----------------
  Serial.println("========== MACHINE DATA ==========");

  Serial.print("Temperature: ");
  Serial.println(temp);

  Serial.print("Vibration: ");
  Serial.println(vibration);

  Serial.println("==================================");

  // ---------------- SEND TO BLYNK ----------------
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, vibration);

  // ---------------- LCD DISPLAY ----------------
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temp, 1);
  lcd.print("C");

  lcd.setCursor(9, 0);
  lcd.print("V:");
  lcd.print(vibration, 0);

  // ---------------- ALERT CONDITION ----------------
  bool currentAlertState =
    (temp > 50 || vibration > 20);

  if (currentAlertState) {

    // Green OFF
    digitalWrite(GREEN_LED, LOW);

    // Buzzer ON
    digitalWrite(BUZZER_PIN, HIGH);

    // LCD
    lcd.setCursor(0, 1);
    lcd.print("STATUS: ALERT");

    // Dashboard
    Blynk.virtualWrite(V2, "ALERT");

    // ---------------- SMART FLICKERING ----------------
    int flickerDelay;

    // Smooth flickering based on vibration
    flickerDelay = map(
      constrain((int)vibration, 20, 60),
      20,
      60,
      500,
      30
    );

   unsigned long currentMillis = millis();

if (currentMillis - previousBlinkMillis >= flickerDelay) {

  previousBlinkMillis = currentMillis;

  redLedState = !redLedState;

  digitalWrite(RED_LED, redLedState);
}

    // ---------------- NOTIFICATION ----------------
    if (!previousAlertState) {

      String alertMessage =
        "WARNING! Temp=" +
        String(temp, 1) +
        " C | Vibration=" +
        String(vibration, 1);

      Blynk.logEvent(
        "machine_alert",
        alertMessage
      );

      Serial.println("BLYNK ALERT SENT");
    }

  } else {

    // Healthy System

    digitalWrite(RED_LED, LOW);
    redLedState = false;

    digitalWrite(GREEN_LED, HIGH);

    digitalWrite(BUZZER_PIN, LOW);

    lcd.setCursor(0, 1);
    lcd.print("STATUS: NORMAL");

    Blynk.virtualWrite(V2, "NORMAL");
  }

  // ---------------- UPDATE STATE ----------------
  previousAlertState = currentAlertState;

 delay(50);
}