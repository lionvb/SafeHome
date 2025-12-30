#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "iPhone d'Arthur";
const char* password = "nutellaa";
const char* serverURL = "http://172.20.10.7/window";  

const int trigPin = 2;
const int echoPin = 15;
const int btnSeuil = 12;

long int seuilDistance = 0;
long distance = 0;

void setup() {
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(btnSeuil, INPUT_PULLUP);
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Fenêtre connectée au WiFi");
}

long mesureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  return (duration / 2) * 0.0344;
}

void envoyerFenetreOuverte() {
  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST("open=1");
  http.end();
}

void loop() {
  distance = mesureDistance();

  if (digitalRead(btnSeuil) == LOW) {
    seuilDistance = distance;
  }

  if (distance >= seuilDistance + 2 || distance <= seuilDistance - 2) {
    Serial.println("Activé la sonnerie");
    envoyerFenetreOuverte();  
    delay(2000);               
  }
}
