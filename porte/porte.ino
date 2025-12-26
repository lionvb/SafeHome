const int portePin=15;
int is_open=LOW;

void setup() {
  pinMode(portePin,INPUT);
  Serial.begin(9600);
}

void loop() {
  is_open=digitalRead(portePin);
  if (is_open==HIGH){
    Serial.print("Activé la sonnerie"); //Voir pour le transmettre à la carte principale
  }
}
