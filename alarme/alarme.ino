#include "LiquidCrystal.h"
LiquidCrystal lcd(15, 4, 16, 17, 5, 18); 

const int buzzerPin = 19;
const int btnGaz=13;
const int btnAlarme=12;
const int analog_gaz=27;
const int digit_gaz=14;

volatile bool stateBuzzer = false;
bool stateAlarme=HIGH;
bool stateGaz=HIGH;
volatile bool is_gaz=LOW;
int digitgaz,analaoggaz;

void setup() {
  pinMode(btnGaz,INPUT_PULLUP);
  pinMode(btnAlarme,INPUT_PULLUP);
  pinMode(digit_gaz,INPUT);
  pinMode(analog_gaz,INPUT);
  pinMode(buzzerPin, OUTPUT);
  Serial.begin(9600);
  lcd.begin(16,2);
  lcd.clear();
  lcd.write("LCD initialisé");
}
bool read_gaz(){
    digitgaz=digitalRead(digit_gaz);
    analoggaz=analogRead(analog_gaz);
    if (digitgaz==1 or analoggaz>=250){
      return HIGH;
    }
    else{
      return LOW;
    }
    Serial.print("digital gaz : ");
    Serial.println(digitgaz);
    Serial.print("analog gaz : ");
    Serial.println(analoggaz);
}

void affichage(bool stateAlarme,bool stateGaz){
    lcd.setCursor(0,0);
    if (stateAlarme==HIGH){
      lcd.write("Alarm is on");
    }
    else{
      lcd.write("Alarm is off");
    }
    lcd.setCursor(0,1);
    if (stateGaz==HIGH){
      lcd.write("Detect gaz is on");
    }
    else{
      lcd.write("Detect gaz is off");
    }
}
    
void loop() {
  stateAlarme=digitalRead(btnAlarme);
  stateGaz=digitalRead(btnGaz);
    if (stateGaz==LOW){
      is_gaz=read_gaz();
    }
    if (stateAlarme==LOW or (is_gaz==HIGH and stateGaz==LOW)){
        digitalWrite(buzzerPin,HIGH);
    }
    else{
      digitalWrite(buzzerPin,LOW);
    }
    affichage(stateAlarme,stateGaz);
}
