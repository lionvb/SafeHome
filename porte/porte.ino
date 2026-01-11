#include <WiFi.h>
#include <HTTPClient.h>

// ================= A MODIFIER selon la personne qui partage le réseau =================
const char* ssid = "A35 de Victor";
const char* password = "safehome";

// ================= A MODIFIER selon l'IP de l'ESP32 principal sur le réseau=================
String serverIP = "10.38.36.91"; 
// ==============================================

const int portePin = 15; 

// Variables pour le lissage du signal (Anti-rebond)
int etatStable = -1;       // Le dernier état confirmé
int lecturePrecedente = -1; 
unsigned long derniereModifTemps = 0;
const long delaiAntiRebond = 100; // Il faut que le signal reste stable 100ms pour être validé

unsigned long lastHeartbeat = 0;

// ================= ENVOI PORTE =================
void envoyerEtat(bool estOuverte) {
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    String url = "http://" + serverIP + "/update_door?state=" + (estOuverte ? "OPEN" : "CLOSED");
    
    // Serial.println("Envoi vers: " + url); // Décommenter pour debug
    
    http.begin(url);
    int httpCode = http.GET();
    http.end();
    
    if(httpCode > 0) Serial.println(estOuverte ? "Envoi: OUVERT" : "Envoi: FERMÉ");
    else Serial.println("Erreur envoi WiFi");
    
  } else {
    Serial.println("Erreur: WiFi perdu !");
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // IMPORTANT : INPUT_PULLDOWN car tu es branché sur le 3.3V
  // Cela tire le signal vers le bas (0V) quand l'aimant est loin
  pinMode(portePin, INPUT_PULLDOWN);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nESP32 Porte connectée !");
}

// ================= LOOP =================
void loop() {
  // 1. LECTURE BRUTE
  // Avec ton branchement 3.3V :
  // HIGH (1) = Aimant touche (Fermé)
  // LOW (0)  = Aimant loin (Ouvert) grâce au PULLDOWN
  int lectureActuelle = digitalRead(portePin);

  // 2. FILTRAGE (Anti-rebond / Anti-sauts)
  // Si le signal change par rapport à l'instant d'avant, on reset le chrono
  if (lectureActuelle != lecturePrecedente) {
    derniereModifTemps = millis();
  }
  lecturePrecedente = lectureActuelle;

  // Si le signal est resté pareil pendant plus de 100ms, on le valide
  if ((millis() - derniereModifTemps) > delaiAntiRebond) {
    
    // Si cet état validé est différent de ce qu'on connaissait avant
    if (lectureActuelle != etatStable) {
      etatStable = lectureActuelle;
      
      // On détermine l'état logique final
      // Si lecture est LOW (0), c'est que le circuit est ouvert -> Porte OUVERTE
      bool estOuverte = (etatStable == LOW);

      Serial.print("Changement état validé : ");
      Serial.println(estOuverte ? "OUVERTE" : "FERMÉE");
      
      envoyerEtat(estOuverte);
    }
  }

  // 3. HEARTBEAT (Rappel toutes les 3 secondes pour dire "je suis connecté")
  if (millis() - lastHeartbeat > 3000) {
    bool estOuverte = (etatStable == LOW);
    envoyerEtat(estOuverte);
    lastHeartbeat = millis();
  }
}
