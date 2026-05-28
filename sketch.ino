#include <WiFi.h>
#include <WebServer.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================= OLED =================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= WIFI =================

const char* ssid = "Wokwi-GUEST";
const char* password = "";

WebServer server(80);

// ================= HC-SR04 =================

#define TRIG_PIN 5
#define ECHO_PIN 18

// ================= OUTPUT =================

#define BUZZER_PIN 26
#define LED_PIN 27

// ================= LDR =================

#define LDR_PIN 34

// ================= BUTTON =================

#define BUTTON_PIN 14

// ================= VARIABLES =================

long duration;
float distance;

bool ledManual = false;
bool emergencyMode = false;

int warningCount = 0;
int emergencyCount = 0;

int minLight = 4095;
int maxLight = 0;

unsigned long lastReminder = 0;

// ================= WEB PAGE =================

void handleRoot() {

  int lightValue = analogRead(LDR_PIN);

  String page = R"(
  <!DOCTYPE html>
  <html>
  <head>

    <meta name="viewport" content="width=device-width, initial-scale=1">

    <title>Smart Desk Assistant</title>

    <style>

      body {
        background: #111;
        color: white;
        font-family: Arial;
        text-align: center;
        margin-top: 40px;
      }

      .btn {
        display: inline-block;
        padding: 15px;
        margin: 10px;
        font-size: 20px;
        text-decoration: none;
        border-radius: 10px;
        color: white;
      }

      .green {
        background: green;
      }

      .red {
        background: red;
      }

      .blue {
        background: blue;
      }

    </style>

  </head>

  <body>
  )";

  page += "<h1>Smart Desk Assistant</h1>";

  page += "<h2>Distance: ";
  page += String(distance);
  page += " cm</h2>";

  page += "<h2>Light: ";
  page += String(lightValue);
  page += "</h2>";

  if (distance < 30) {
    page += "<h2 style='color:red;'>WARNING</h2>";
  } else {
    page += "<h2 style='color:lightgreen;'>SAFE</h2>";
  }

  page += "<p>Warnings: ";
  page += String(warningCount);
  page += "</p>";

  page += "<p>Emergencies: ";
  page += String(emergencyCount);
  page += "</p>";

  page += "<p>Min light: ";
  page += String(minLight);
  page += "</p>";

  page += "<p>Max light: ";
  page += String(maxLight);
  page += "</p>";

  page += R"(

    <a class="btn green" href="/led/on">LED ON</a>

    <a class="btn red" href="/led/off">LED OFF</a>

    <a class="btn blue" href="/emergency">EMERGENCY</a>

  </body>
  </html>
  )";

  server.send(200, "text/html", page);
}

// ================= LED ON =================

void handleLedOn() {

  ledManual = true;

  digitalWrite(LED_PIN, HIGH);

  server.sendHeader("Location", "/");

  server.send(303);
}

// ================= LED OFF =================

void handleLedOff() {

  ledManual = false;

  digitalWrite(LED_PIN, LOW);

  server.sendHeader("Location", "/");

  server.send(303);
}

// ================= EMERGENCY =================

void handleEmergency() {

  emergencyMode = true;

  emergencyCount++;

  server.sendHeader("Location", "/");

  server.send(303);
}

// ================= SETUP =================

void setup() {

  Serial.begin(115200);

  // OLED

  Wire.begin(21, 22);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.clearDisplay();

  display.setTextSize(2);

  display.setTextColor(WHITE);

  display.setCursor(10, 10);

  display.println("SMART");

  display.setCursor(10, 35);

  display.println("DESK");

  display.display();

  delay(2000);

  // HC-SR04

  pinMode(TRIG_PIN, OUTPUT);

  pinMode(ECHO_PIN, INPUT);

  // OUTPUT

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(LED_PIN, OUTPUT);

  // BUTTON

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // WIFI

  WiFi.begin(ssid, password);

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);

  display.println("Connecting WiFi...");

  display.display();

  while (WiFi.status() != WL_CONNECTED) {

    delay(100);
  }

  Serial.println(WiFi.localIP());

  // SERVER

  server.on("/", handleRoot);

  server.on("/led/on", handleLedOn);

  server.on("/led/off", handleLedOff);

  server.on("/emergency", handleEmergency);

  server.begin();

  display.clearDisplay();

  display.setCursor(0, 0);

  display.println("WiFi Ready");

  display.display();

  delay(1000);
}

// ================= LOOP =================

void loop() {

  server.handleClient();

  // ===== DISTANCE =====

  digitalWrite(TRIG_PIN, LOW);

  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);

  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);

  distance = duration * 0.034 / 2;

  // ===== LIGHT =====

  int lightValue = analogRead(LDR_PIN);

  // ===== LIGHT STATS =====

  if (lightValue < minLight) {

    minLight = lightValue;
  }

  if (lightValue > maxLight) {

    maxLight = lightValue;
  }

  // ===== OLED =====

  display.clearDisplay();

  display.setTextSize(2);

  display.setCursor(0, 0);

  if (distance < 30) {

    display.println("WARNING");

  } else {

    display.println("SAFE");
  }

  display.setTextSize(1);

  display.setCursor(0, 24);

  display.print("Dist: ");

  display.print(distance);

  display.println(" cm");

  display.setCursor(0, 36);

  display.print("Light: ");

  display.println(lightValue);

  display.setCursor(0, 48);

  display.print("W:");

  display.print(warningCount);

  display.print(" E:");

  display.print(emergencyCount);

  display.display();

  // ===== WARNING =====

  if (distance < 30) {

    digitalWrite(LED_PIN, HIGH);

    tone(BUZZER_PIN, 1000);

    warningCount++;

  } else {

    noTone(BUZZER_PIN);

    if (!ledManual && lightValue > 1000) {

      digitalWrite(LED_PIN, LOW);
    }
  }

  // ===== NIGHT MODE =====

  if (lightValue < 1000) {

    digitalWrite(LED_PIN, HIGH);
  }

  // ===== BUTTON =====

  if (digitalRead(BUTTON_PIN) == LOW) {

    emergencyMode = true;

    emergencyCount++;
  }

  // ===== EMERGENCY MODE =====

  if (emergencyMode) {

    display.clearDisplay();

    display.setTextSize(2);

    display.setCursor(0, 20);

    display.println("EMERGENCY");

    display.display();

    for (int i = 0; i < 6; i++) {

      digitalWrite(LED_PIN, HIGH);

      tone(BUZZER_PIN, 2000);

      delay(200);

      digitalWrite(LED_PIN, LOW);

      noTone(BUZZER_PIN);

      delay(200);
    }

    emergencyMode = false;
  }

  // ===== BREAK REMINDER =====

  if (millis() - lastReminder > 30000) {

    display.clearDisplay();

    display.setTextSize(2);

    display.setCursor(0, 20);

    display.println("BREAK!");

    display.display();

    delay(3000);

    lastReminder = millis();
  }

  // ===== SERIAL MONITOR =====

  Serial.print("Distance: ");

  Serial.print(distance);

  Serial.print(" cm | Light: ");

  Serial.println(lightValue);

  delay(500);
}