#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "iPhone d'Arthur";
const char* password = "nutellaa";
const char* serverURL = "http://172.20.10.7/door"; 

const int portePin = 15;
int is_open = LOW;

void setup() {
  pinMode(portePin, INPUT);
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Porte connectée au WiFi");
}

void envoyerPorteOuverte() {
  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST("open=1");
  http.end();
}

void loop() {
  is_open = digitalRead(portePin);

  if (is_open == HIGH) {
    Serial.println("Activé la sonnerie");
    envoyerPorteOuverte();   
    delay(2000);
  }
}
