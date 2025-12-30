#include <WiFi.h>
#include <WebServer.h>
#include "LiquidCrystal.h"

// WIFI 
const char* ssid = "iPhone d'Arthur";
const char* password = "nutellaa";

WebServer server(80);

// LCD 
LiquidCrystal lcd(15, 4, 16, 17, 5, 18);

// PINS 
const int buzzerPin = 19;
const int btnGaz = 13;
const int btnAlarme = 12;
const int analog_gaz = 27;
const int digit_gaz = 14;

// ETATS 
bool porteOuverte = false;
bool fenetreOuverte = false;

bool stateAlarme = HIGH;   // HIGH = désarmée
bool stateGaz = HIGH;      // HIGH = détection gaz OFF
bool is_gaz = LOW;

// mémoires boutons (anti-conflit)
bool lastBtnAlarme = HIGH;
bool lastBtnGaz = HIGH;

int digitgaz, analoggaz;

// PAGE WEB 
String pageHTML() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<title>Alarme ESP32</title>
<style>
body { font-family: Arial; text-align: center; }
button { font-size: 16px; padding: 10px; margin: 5px; }
</style>
</head>
<body>

<h1>Alarme Connectée</h1>

<p>Alarme : <span id="alarme">---</span></p>
<p>Gaz : <span id="gaz">---</span></p>
<p>Porte : <span id="porte">---</span></p>
<p>Fenêtre : <span id="fenetre">---</span></p>

<button onclick="fetch('/arm')">Armer</button>
<button onclick="fetch('/disarm')">Désarmer</button><br>

<button onclick="fetch('/gaz_on')">Gaz ON</button>
<button onclick="fetch('/gaz_off')">Gaz OFF</button>

<script>
function refresh() {
  fetch('/status')
    .then(r => r.json())
    .then(d => {
      alarme.innerText = d.alarme ? "ARMÉE" : "DÉSARMÉE";
      gaz.innerText = d.gaz ? "ACTIF" : "INACTIF";
      porte.innerText = d.porte ? "OUVERTE" : "FERMÉE";
      fenetre.innerText = d.fenetre ? "OUVERTE" : "FERMÉE";
    });
}
setInterval(refresh, 1000);
refresh();
</script>

</body>
</html>
)rawliteral";
}

// ROUTES WEB 
void handleRoot() {
  server.send(200, "text/html", pageHTML());
}

void handleArm() {
  stateAlarme = LOW;
  server.send(200, "OK");
}

void handleDisarm() {
  stateAlarme = HIGH;
  digitalWrite(buzzerPin, LOW);
  server.send(200, "OK");
}

void handleGazOn() {
  stateGaz = LOW;
  server.send(200, "OK");
}

void handleGazOff() {
  stateGaz = HIGH;
  is_gaz = LOW;
  server.send(200, "OK");
}

void handleStatus() {
  String json = "{";
  json += "\"alarme\":" + String(stateAlarme == LOW ? "true" : "false") + ",";
  json += "\"gaz\":" + String(stateGaz == LOW ? "true" : "false") + ",";
  json += "\"porte\":" + String(porteOuverte ? "true" : "false") + ",";
  json += "\"fenetre\":" + String(fenetreOuverte ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

// RÉCEPTION PORTE / FENÊTRE 
void handleDoor() {
  porteOuverte = true;
  server.send(200, "OK");
}

void handleWindow() {
  fenetreOuverte = true;
  server.send(200, "OK");
}

//  GAZ 
bool read_gaz() {
  digitgaz = digitalRead(digit_gaz);
  analoggaz = analogRead(analog_gaz);
  if (digitgaz == 1 || analoggaz >= 250) {
    return HIGH;
  }
  return LOW;
}

//  LCD 
void affichage(bool stateAlarme, bool stateGaz) {
  lcd.setCursor(0, 0);
  lcd.print(stateAlarme == HIGH ? "Alarm is off   " : "Alarm is on    ");

  lcd.setCursor(0, 1);
  lcd.print(stateGaz == HIGH ? "Detect gaz off " : "Detect gaz on  ");
}

// SETUP 
void setup() {
  pinMode(btnGaz, INPUT_PULLUP);
  pinMode(btnAlarme, INPUT_PULLUP);
  pinMode(digit_gaz, INPUT);
  pinMode(analog_gaz, INPUT);
  pinMode(buzzerPin, OUTPUT);

  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Initialisation");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connecté");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/arm", handleArm);
  server.on("/disarm", handleDisarm);
  server.on("/gaz_on", handleGazOn);
  server.on("/gaz_off", handleGazOff);
  server.on("/status", handleStatus);

  server.on("/door", HTTP_POST, handleDoor);
  server.on("/window", HTTP_POST, handleWindow);

  server.begin();
}

void loop() {
  server.handleClient();

  // BOUTON ALARME (toggle)
  bool currentBtnAlarme = digitalRead(btnAlarme);
  if (currentBtnAlarme == LOW && lastBtnAlarme == HIGH) {
    stateAlarme = !stateAlarme;
  }
  lastBtnAlarme = currentBtnAlarme;

  // BOUTON GAZ (toggle)
  bool currentBtnGaz = digitalRead(btnGaz);
  if (currentBtnGaz == LOW && lastBtnGaz == HIGH) {
    stateGaz = !stateGaz;
  }
  lastBtnGaz = currentBtnGaz;

  // LECTURE GAZ 
  if (stateGaz == LOW) {
    is_gaz = read_gaz();
  } else {
    is_gaz = LOW;
  }

  // ALARME 
  if (
    stateAlarme == LOW &&
    (is_gaz == HIGH || porteOuverte || fenetreOuverte)
  ) {
    digitalWrite(buzzerPin, HIGH);
  } else {
    digitalWrite(buzzerPin, LOW);
  }

  affichage(stateAlarme, stateGaz);
}
