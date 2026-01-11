#include <WiFi.h>
#include <HTTPClient.h>

// ================= A MODIFIER selon la personne qui partage le réseau =================
const char* ssid = "A35 de Victor";
const char* password = "safehome";

// ================= A MODIFIER selon l'IP de l'ESP32 principal sur le réseau =================
String serverIP = "10.38.36.91"; 
// ==============================================

// ================= PINS =================
const int trigPin = 4;      
const int echoPin = 15;     
const int btnCalib = 12;    

// VARIABLES
float seuilFermeture = 0.0; // La distance de référence (fenêtre fermée)
bool fenetreEstOuverte = false;
bool oldFenetreState = false;

unsigned long lastSent = 0;

// ================= DISTANCE =================
float getDistance() {
  // Nettoyage
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Envoi
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Lecture (Timeout 30ms pour éviter de bloquer)
  long duration = pulseIn(echoPin, HIGH, 30000);
  
  if (duration == 0) return -1.0; // Erreur de lecture
  return (duration * 0.034 / 2);
}

// ================= ENVOI FENETRE =================
void envoyerEtat(bool ouverte) {
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    // On envoie sur la route /update_window
    String url = "http://" + serverIP + "/update_window?state=" + (ouverte ? "OPEN" : "CLOSED");
    
    http.begin(url);
    int httpCode = http.GET();
    http.end();
    
    if(httpCode > 0) Serial.println(ouverte ? ">> Envoi: OUVERTE" : ">> Envoi: FERMÉE");
    else Serial.println(">> Erreur WiFi");
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(btnCalib, INPUT_PULLUP);

  Serial.println("\n--- Module Fenêtre Démarré ---");

  // Connexion WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connecté !");
  
  // Calibrage initial automatique
  Serial.println("Mesure initiale...");
  delay(1000);
  float dist = getDistance();
  if (dist > 0) {
    seuilFermeture = dist;
    Serial.print("Calibrage auto : Fenêtre fermée à ");
    Serial.print(seuilFermeture);
    Serial.println(" cm");
  }
}

// ================= LOOP =================
void loop() {
  // 1. GESTION DU BOUTON CALIBRAGE (D12)
  if (digitalRead(btnCalib) == LOW) {
    Serial.println("Bouton appuyé -> Recalibrage en cours...");
    delay(1000); // On attend que tu retires ta main
    
    // On prend 3 mesures pour être précis
    float d1 = getDistance(); delay(50);
    float d2 = getDistance(); delay(50);
    float d3 = getDistance();
    
    if(d1 > 0 && d2 > 0 && d3 > 0) {
      seuilFermeture = (d1 + d2 + d3) / 3.0;
      Serial.print(">>> NOUVEAU SEUIL ENREGISTRÉ : ");
      Serial.print(seuilFermeture);
      Serial.println(" cm");
      
      // On force l'envoi de l'état "FERMÉ" tout de suite
      fenetreEstOuverte = false;
      envoyerEtat(false);
    } else {
      Serial.println("Erreur mesure pendant calibrage !");
    }
  }

  // 2. LOGIQUE D'OUVERTURE
  float distanceActuelle = getDistance();
  
  if (distanceActuelle > 0) { // Si la lecture est valide
    
    // LOGIQUE DEMANDÉE : Distance > Seuil + 2cm
    if ((distanceActuelle > (seuilFermeture + 2.0))||(distanceActuelle < (seuilFermeture - 2.0))) {
      fenetreEstOuverte = true;
    } else {
      fenetreEstOuverte = false;
    }

    Serial.print("Actuel: "); 
    Serial.print(distanceActuelle);
    Serial.print(" cm | Seuil: "); 
    Serial.println(seuilFermeture);
  }

  // 3. ENVOI AU MODULE PRINCIPAL
  // On envoie si l'état change OU toutes les 3 secondes (Heartbeat)
  unsigned long currentMillis = millis();
  if (fenetreEstOuverte != oldFenetreState || (currentMillis - lastSent > 3000)) {
    
    envoyerEtat(fenetreEstOuverte);
    
    oldFenetreState = fenetreEstOuverte;
    lastSent = currentMillis;
  }
  
  delay(200); // Pause
}
