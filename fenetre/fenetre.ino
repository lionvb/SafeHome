const int trigPin = 2;
const int echoPin = 15;
const int btnSeuil=20;
long int seuilDistance=0;
long distance = 0; // Distance mesurée

void setup() {
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(btnSeuil,INPUT_PULLUP);
  Serial.begin(9600);
}

long mesureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  return (duration / 2) * 0.0344;  // Distance en cm
}

void consoleDistance(long dist,long seuilDistance,bool etatAlarme) {
  Serial.print("Distance: ");
  Serial.print(dist);
  Serial.print(" cm,  ");
  Serial.print("Seuil: ");
  Serial.print(seuilDistance);
  Serial.print(" cm");
}

void loop() {
  distance = mesureDistance(); // mesure de distance
  if (digitalRead(btnSeuil)==LOW){
    seuilDistance=distance;
  }
  if (distance>=seuilDistance+2 or distance<=seuilDistance-2){
    Serial.print("Activé la sonnerie"); //Voir pour le transmettre à la carte principale
  }
}
