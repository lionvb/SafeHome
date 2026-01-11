#include <WiFi.h>
#include <WebServer.h>
#include "LiquidCrystal.h"

// ================= WIFI =================
// A MODIFIER selon la personne qui partage le réseau 
const char* ssid = "A35 de Victor";
const char* password = "safehome";

WebServer server(80);

// ================= LCD ==================
// Pins : RS, E, D4, D5, D6, D7
LiquidCrystal lcd(15, 4, 21, 22, 5, 18);

// ================= PINS =================
const int buzzerPin = 19;
const int btnGaz = 13;
const int btnAlarme = 14;
const int analog_gaz = 25;
const int digit_gaz = 26;

// ================= ETATS =================
bool porteOuverte = false;
bool fenetreOuverte = false;
bool doorConnected = false;
bool windowConnected = false; 

// --- MODIFICATION ICI : INITIALISATION A "OFF" ---
bool stateAlarme = HIGH;   // HIGH = DÉSARMÉE au démarrage
bool stateGaz = HIGH;      // HIGH = GAZ DÉSACTIVÉ au démarrage
// -------------------------------------------------

bool is_gaz = false; 
bool lastBtnAlarme = HIGH;
bool lastBtnGaz = HIGH;
int digitgaz, analoggaz;

// ================= TIMING =================
unsigned long lastDoorTime = 0;
unsigned long lastWindowTime = 0;
const unsigned long TIMEOUT = 4000; // 4s sans nouvelle = déconnecté
unsigned long previousMillisLoop = 0; 
const long intervalleLoop = 500; 

// ================= PAGE WEB DESIGN PRO =================
String pageHTML() {
  // Préparation des valeurs initiales pour le HTML
  return R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>SafeHome Control</title>
<style>
  :root {
    --primary: #4a90e2;
    --success: #2ecc71;
    --danger: #e74c3c;
    --warning: #f1c40f;
    --dark: #2c3e50;
    --light: #ecf0f1;
    --white: #ffffff;
    --shadow: 0 10px 20px rgba(0,0,0,0.08);
  }

  body { 
    font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; 
    background-color: #f4f7f6; 
    margin: 0; 
    padding: 20px; 
    color: var(--dark);
    display: flex;
    justify-content: center;
  }

  .dashboard { 
    background: var(--white); 
    width: 100%; 
    max-width: 450px; 
    border-radius: 20px; 
    box-shadow: var(--shadow); 
    overflow: hidden; 
  }

  .header {
    background: var(--primary);
    color: var(--white);
    padding: 20px;
    text-align: center;
  }
  .header h1 { margin: 0; font-size: 24px; font-weight: 600; }
  .header p { margin: 5px 0 0; opacity: 0.8; font-size: 14px; }

  .section { padding: 20px; border-bottom: 1px solid var(--light); }
  .section:last-child { border-bottom: none; }
  
  .section-title { 
    font-size: 12px; 
    text-transform: uppercase; 
    letter-spacing: 1px; 
    color: #95a5a6; 
    margin-bottom: 15px; 
    font-weight: 700;
  }

  /* Status Display Styling */
  .status-display {
    text-align: center;
    margin-bottom: 15px;
  }
  .status-badge {
    display: inline-block;
    padding: 8px 16px;
    border-radius: 50px;
    font-weight: bold;
    font-size: 14px;
    background: var(--light);
    color: var(--dark);
    transition: 0.3s;
  }
  .status-badge.safe { background: rgba(46, 204, 113, 0.15); color: var(--success); }
  .status-badge.danger { background: rgba(231, 76, 60, 0.15); color: var(--danger); }
  .status-badge.warning { background: rgba(241, 196, 15, 0.15); color: #d35400; }

  /* Grid for Sensors */
  .sensor-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 15px;
  }
  .sensor-card {
    background: #f8f9fa;
    padding: 15px;
    border-radius: 12px;
    text-align: center;
  }
  .sensor-icon { font-size: 24px; margin-bottom: 5px; display: block; }
  .sensor-name { font-size: 14px; color: #7f8c8d; display: block; }
  .sensor-value { font-weight: bold; font-size: 16px; margin-top: 5px; display: block;}

  /* Connection Info */
  .connection-row {
    display: flex;
    justify-content: space-between;
    font-size: 13px;
    margin-bottom: 8px;
    color: #34495e;
  }
  .conn-dot {
    height: 8px; width: 8px;
    background-color: #bdc3c7;
    border-radius: 50%;
    display: inline-block;
    margin-right: 5px;
  }
  .conn-dot.online { background-color: var(--success); box-shadow: 0 0 5px var(--success);}
  .conn-dot.offline { background-color: var(--danger); }

  /* Buttons */
  .btn-group { display: flex; gap: 10px; margin-top: 10px; }
  button { 
    flex: 1; 
    padding: 12px; 
    border: none; 
    border-radius: 10px; 
    cursor: pointer; 
    font-weight: 600; 
    font-size: 14px; 
    transition: 0.2s transform;
  }
  button:active { transform: scale(0.98); }
  
  .btn-arm { background: var(--success); color: white; }
  .btn-disarm { background: white; border: 2px solid var(--danger); color: var(--danger); }
  
  .btn-on { background: var(--primary); color: white; }
  .btn-off { background: white; border: 2px solid #95a5a6; color: #7f8c8d; }
  
  .refresh-btn { 
    width: 100%; 
    background: transparent; 
    color: var(--primary); 
    margin-top: 10px; 
    font-size: 12px;
  }

</style>
</head>
<body>

<div class="dashboard">
  <div class="header">
    <h1>🏡 Ma Maison</h1>
    <p>Système de Sécurité ESP32</p>
  </div>

  <div class="section">
    <div class="section-title">🚨 Alarme Intrusion</div>
    <div class="status-display">
      <span id="badge-alarme" class="status-badge">Chargement...</span>
    </div>
    <div class="btn-group">
      <button class="btn-arm" onclick="fetch('/arm'); updateUI()">🔒 ARMER</button>
      <button class="btn-disarm" onclick="fetch('/disarm'); updateUI()">🔓 DÉSARMER</button>
    </div>
  </div>

  <div class="section">
    <div class="section-title">🚪 Ouvertures</div>
    <div class="sensor-grid">
      <div class="sensor-card">
        <span class="sensor-icon">🚪</span>
        <span class="sensor-name">Porte Entrée</span>
        <span id="val-porte" class="sensor-value">--</span>
      </div>
      <div class="sensor-card">
        <span class="sensor-icon">🪟</span>
        <span class="sensor-name">Fenêtre</span>
        <span id="val-fenetre" class="sensor-value">--</span>
      </div>
    </div>
  </div>

  <div class="section">
    <div class="section-title">📶 État du Réseau</div>
    <div class="connection-row">
      <span>Module Porte</span>
      <span id="conn-porte"><span class="conn-dot"></span> --</span>
    </div>
    <div class="connection-row">
      <span>Module Fenêtre</span>
      <span id="conn-fenetre"><span class="conn-dot"></span> --</span>
    </div>
  </div>

  <div class="section">
    <div class="section-title">🔥 Sécurité Gaz</div>
    <div class="sensor-grid">
      <div class="sensor-card" style="grid-column: span 2;">
        <span class="sensor-name">État Surveillance</span>
        <span id="badge-gaz-system" class="sensor-value" style="color:var(--primary)">--</span>
      </div>
    </div>
    <div style="margin-top:15px; text-align:center;">
       <span id="badge-gaz-detect" class="status-badge">Analyse...</span>
    </div>
    <div class="btn-group">
      <button class="btn-on" onclick="fetch('/gaz_on'); updateUI()">⚡ ACTIVER</button>
      <button class="btn-off" onclick="fetch('/gaz_off'); updateUI()">💤 DÉSACTIVER</button>
    </div>
  </div>

  <div class="section" style="text-align: center; padding: 10px;">
    <button class="refresh-btn" onclick="updateUI()">🔄 Actualiser les données</button>
  </div>
</div>

<script>
function updateUI() {
  setTimeout(() => {
    fetch('/status')
      .then(response => response.json())
      .then(data => {
        // 1. ALARME
        const badgeAlarme = document.getElementById('badge-alarme');
        if (data.alarme) {
           badgeAlarme.innerText = "ARMÉE";
           badgeAlarme.className = "status-badge safe";
        } else {
           badgeAlarme.innerText = "DÉSARMÉE";
           badgeAlarme.className = "status-badge warning";
        }

        // 2. PORTE & FENETRE
        const valPorte = document.getElementById('val-porte');
        valPorte.innerText = data.porte ? "OUVERTE" : "FERMÉE";
        valPorte.style.color = data.porte ? "#e74c3c" : "#2ecc71";

        const valFenetre = document.getElementById('val-fenetre');
        valFenetre.innerText = data.fenetre ? "OUVERTE" : "FERMÉE";
        valFenetre.style.color = data.fenetre ? "#e74c3c" : "#2ecc71";

        // 3. CONNEXIONS
        const connPorte = document.getElementById('conn-porte');
        if(data.doorConnected) connPorte.innerHTML = '<span class="conn-dot online"></span> Connecté';
        else connPorte.innerHTML = '<span class="conn-dot offline"></span> Déconnecté !';

        const connFenetre = document.getElementById('conn-fenetre');
        if(data.windowConnected) connFenetre.innerHTML = '<span class="conn-dot online"></span> Connecté';
        else connFenetre.innerHTML = '<span class="conn-dot offline"></span> Déconnecté !';

        // 4. GAZ
        const badgeGazSys = document.getElementById('badge-gaz-system');
        badgeGazSys.innerText = data.gazActive ? "ACTIVÉE" : "EN PAUSE";
        badgeGazSys.style.color = data.gazActive ? "#2ecc71" : "#95a5a6";

        const badgeGazDet = document.getElementById('badge-gaz-detect');
        if(data.gazDetect) {
           badgeGazDet.innerHTML = "⚠️ DANGER : GAZ DÉTECTÉ";
           badgeGazDet.className = "status-badge danger";
        } else {
           badgeGazDet.innerHTML = "Air Sain (R.A.S)";
           badgeGazDet.className = "status-badge safe";
        }
      })
      .catch(err => console.error("Erreur update:", err));
  }, 200);
}

// Rafraichissement automatique
updateUI();
setInterval(updateUI, 2000);
</script>
</body>
</html>
)rawliteral";
}

// ================= ROUTES =================
void handleRoot() { server.send(200, "text/html", pageHTML()); }
void handleArm() { stateAlarme = LOW; server.send(200, "text/plain", "OK"); }
void handleDisarm() { stateAlarme = HIGH; digitalWrite(buzzerPin, LOW); server.send(200, "text/plain", "OK"); }
void handleGazOn() { stateGaz = LOW; server.send(200, "text/plain", "OK"); }
void handleGazOff() { stateGaz = HIGH; is_gaz = false; server.send(200, "text/plain", "OK"); }

void handleStatus() {
  String json = "{";
  json += "\"alarme\":" + String(stateAlarme == LOW ? "true" : "false") + ",";
  json += "\"porte\":" + String(porteOuverte ? "true" : "false") + ",";
  json += "\"fenetre\":" + String(fenetreOuverte ? "true" : "false") + ",";
  json += "\"doorConnected\":" + String(doorConnected ? "true" : "false") + ",";
  json += "\"windowConnected\":" + String(windowConnected ? "true" : "false") + ",";
  json += "\"gazActive\":" + String(stateGaz == LOW ? "true" : "false") + ",";
  json += "\"gazDetect\":" + String(is_gaz ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

// === RECEPTION PORTE ===
void handleUpdateDoor() {
  if (server.hasArg("state")) {
    porteOuverte = (server.arg("state") == "OPEN");
    lastDoorTime = millis();
    doorConnected = true;
    server.send(200, "text/plain", "OK");
  } else server.send(400, "text/plain", "Erreur");
}

// === RECEPTION FENETRE ===
void handleUpdateWindow() {
  if (server.hasArg("state")) {
    fenetreOuverte = (server.arg("state") == "OPEN");
    lastWindowTime = millis();
    windowConnected = true;
    server.send(200, "text/plain", "OK");
  } else server.send(400, "text/plain", "Erreur");
}

// ================= GAZ =================
bool read_gaz() {
  digitgaz = digitalRead(digit_gaz);
  analoggaz = analogRead(analog_gaz);
  return (digitgaz == 1 || analoggaz >= 1500); 
}

// ================= LCD =================
void affichage() {
  lcd.clear(); // Clear nécessaire ici car on change beaucoup de texte
  
  lcd.setCursor(0, 0);
  if(stateAlarme == LOW) lcd.print("ALARM:ON ");
  else                   lcd.print("ALARM:OFF");

  lcd.print(" GAZ:");
  lcd.print((stateGaz == LOW) ? "ON " : "OFF");

  lcd.setCursor(0, 1);
  if(is_gaz)             lcd.print("!! GAZ DANGER !!"); 
  else if(porteOuverte)  lcd.print("!! PORTE OPEN !!"); 
  else if(fenetreOuverte) lcd.print("! FENETRE OPEN !");
  else {
      // Affichage état connexions si tout est calme
      lcd.print("P:");
      lcd.print(doorConnected ? "OK" : "XX");
      lcd.print(" F:");
      lcd.print(windowConnected ? "OK" : "XX");
  }
}

// ================= SETUP =================
void setup() {
  pinMode(btnGaz, INPUT_PULLUP);
  pinMode(btnAlarme, INPUT_PULLUP);
  pinMode(digit_gaz, INPUT);
  pinMode(analog_gaz, INPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Connexion WiFi");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  Serial.println("\nWiFi OK ! IP: ");
  Serial.println(WiFi.localIP());

  lcd.setCursor(0,0); lcd.print("IP Adr:");
  lcd.setCursor(0,1); lcd.print(WiFi.localIP());
  delay(3000); lcd.clear();

  server.on("/", handleRoot);
  server.on("/arm", handleArm);
  server.on("/disarm", handleDisarm);
  server.on("/gaz_on", handleGazOn);
  server.on("/gaz_off", handleGazOff);
  server.on("/update_door", handleUpdateDoor);
  server.on("/update_window", handleUpdateWindow);
  server.on("/status", handleStatus);
  
  server.begin();
}

// ================= LOOP =================
void loop() {
  server.handleClient();

  // TIMEOUT DES CAPTEURS
  if (millis() - lastDoorTime > TIMEOUT) doorConnected = false;
  else doorConnected = true;
  
  if (millis() - lastWindowTime > TIMEOUT) windowConnected = false;
  else windowConnected = true;

  // BOUTONS (Inversion d'état à chaque appui)
  if (digitalRead(btnAlarme) == LOW) { 
      delay(50); 
      if(digitalRead(btnAlarme) == LOW && lastBtnAlarme == HIGH) stateAlarme = !stateAlarme; 
      lastBtnAlarme = LOW; 
  } else lastBtnAlarme = HIGH;

  if (digitalRead(btnGaz) == LOW) { 
      delay(50); 
      if(digitalRead(btnGaz) == LOW && lastBtnGaz == HIGH) stateGaz = !stateGaz; 
      lastBtnGaz = LOW; 
  } else lastBtnGaz = HIGH;

  // LOGIQUE
  // On ne lit le gaz que si la surveillance Gaz est ACTIVE (LOW)
  is_gaz = (stateGaz == LOW) ? read_gaz() : false;
  
  // Intrusion seulement si Alarme ARMÉE (LOW)
  bool intrusion = (stateAlarme == LOW && (porteOuverte || fenetreOuverte));
  
  // SONNERIE
  if (intrusion || is_gaz) digitalWrite(buzzerPin, HIGH);
  else digitalWrite(buzzerPin, LOW);

  // AFFICHAGE
  if (millis() - previousMillisLoop >= intervalleLoop) {
    previousMillisLoop = millis();
    affichage();
  }
}
