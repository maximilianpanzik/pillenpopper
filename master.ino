#include <Servo.h>
Servo servo_cut;    // erstellt ein Servo-Objekt, um einen Servomotor zu steuern
Servo servo_press;  // erstellt ein Servo-Objekt, um einen Servomotor zu steuern
Servo servo_vorschub;
Servo servo_sortierer;

#define E 8
#define D 9
#define C 10
#define B 11
#define A 12
#define OK_BUTTON 13

/* ALT -> prüfen
//#define ENA 9
#define IN1 6
#define IN2 7
#define BUTTON 5
#define LIGHT_GATE 2
#define SERVO_CUT_PIN 8
#define SERVO_PRESS_PIN 4
// Variablen definieren

//#define TRIGGER_VALUE 500
#define PILLS_IN_BLISTER 5
#define TIME_TO_NUPSI 20000
#define SPEED 150
*/

#define LED_PERIODE 1
#define BUTTON_SCHUTZ_PERIODE 1000
#define BUTTON_TAG_ABTAST_PERIODE 100
#define BUTTON_OK_ABTAST_PERIODE 100

int buttonPins[7] = {2,3,3,3,3,3,3}; //noch zu defnieren!!

unsigned long currentMillis;               // vergangene Zeit in ms seit Programmstart
unsigned long startMillisLed;              // aktuelle LED periode
unsigned long startMillisButtonsSchutz[7]; // aktuellte Button Schutz Periode
unsigned long startMillisButtonsTagAbtast; // aktuelle Tage Button abtast Periode
unsigned long startMillisButtonsOkAbtast;  // aktuelle Ok Button abtast Periode
int c[2][7][2] =                           // schaltplan, um LED # mit charlieplexing blau oder grün zu schalten
    {
        {{C, A}, {C, B}, {B, C}, {B, D}, {B, E}, {E, A}, {E, B}},  // blau
        {{B, A}, {A, B}, {A, C}, {A, D}, {A, E}, {D, A}, {D, B}} // grün
};      //  1       2       3       4       5        6       7
int ledIncrement = 0;
//bool tageButtonValues[] = {1,1,1,1,1,1,1}; // 0: nicht aktiv, 1: aktiv
bool tageButtonValues[] = {0,1,0,1,1,1,1}; // 0: nicht aktiv, 1: aktiv
//int fuellStand[] = {0, 0, 0, 0, 0, 0, 0};        // 0: nicht voll, 1: voll
int fuellStand[] = {1, 1, 1, 0, 0, 0, 0};        // 0: nicht voll, 1: voll


void setup()
{
  Serial.begin(9600);
  pinMode(A, INPUT);
  pinMode(B, INPUT);
  pinMode(C, INPUT);
  pinMode(D, INPUT);
  pinMode(E, INPUT);
  
  //timer perioden starten
  startMillisLed = millis();
  startMillisButtonsTagAbtast = millis();
  startMillisButtonsOkAbtast = millis();
  for (int i = 0; i < 7; i++)
  {
    startMillisButtonsSchutz[i] = millis();
    pinMode(buttonPins[i],INPUT);
  }
  lightshow(); //lichtwelle

/* ALT -> prüfen
  servo_cut.attach(SERVO_CUT_PIN); //Das Setup enthält die Information, dass das Servo an der Steuerleitung mit Pin 10 verbunden wird.
  servo_press.attach(SERVO_PRESS_PIN); //Das Setup enthält die Information, dass das Servo an der Steuerleitung mit Pin 11 verbunden wird.
  //pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(BUTTON, INPUT);


  // Rotationrichtung bestimmen

  servo_cut.write(0);   // Beim Starten des Programmes fährt der Motor auf die 0-Position.
  servo_press.write(0); 
  //digitalWrite(IN1, LOW);
  //digitalWrite(IN2, HIGH);
*/

}
void light(int pins[2])
{
  /**
   * @brief settet alle LED pins auf GND und stellt dann eine Anoden-Catoden Kombination ein
   * gemäß charlieplexing
   * @param pins der zu schaltenden Anoden [0] und Catoden [1] Pin (in)
   *
   */

  // alle LED pins auf Input -> GND 
  pinMode(A, INPUT);
  pinMode(B, INPUT);
  pinMode(C, INPUT);
  pinMode(D, INPUT);
  pinMode(E, INPUT);

  // stellt eine Anoden-Catoden Kombination ein
  pinMode(pins[0], OUTPUT);
  digitalWrite(pins[0], HIGH);
  pinMode(pins[1], OUTPUT);
  digitalWrite(pins[1], LOW);
}
void LED_schalten()
{
  /** LED_schalten
   * @brief schaltet LEDs abhängig von Button Selection (bool tageButtonValues[]) und
   * Füllstatus (int fuellStand []) schaltet alle LED_PERIODE ms eine LED
   *
   **/
  if (currentMillis - startMillisLed >= LED_PERIODE)
  {
    startMillisLed = currentMillis;
    if (tageButtonValues[ledIncrement])
    { // wenn Tag # button aktiv

      // blau (c[0][]) wenn tag # fuellstand leer,
      // grün (c[1][]) wenn tag # fuellstand voll
      light(c[fuellStand[ledIncrement]][ledIncrement]);
    }
    // incrementieren wenn <6, sonst 0
    //ledIncrement = (ledIncrement < 6) ? ledIncrement++ : 0;
    if(ledIncrement <6){
      ledIncrement++;
    }
    else{
      ledIncrement = 0;
    }
  }
}
void lightshow()
/**
 * @brief leuchtet alle LEDs auf
 * 
 */
{
  for(int i = 0; i<2; i++){
    for(int j = 0; j<7; j++){
      light(c[i][j]);
      delay(100);
    }
        for(int j = 6; j>-1; j--){
      light(c[i][j]);
      delay(100);
    }
  }
  //alles aus schalten
  pinMode(A, INPUT);
  pinMode(B, INPUT);
  pinMode(C, INPUT);
  pinMode(D, INPUT);
  pinMode(E, INPUT);
  
}
void update_tage_button_selection()
{
  /**
   * @brief ändert für jeden Button periodisch nach BUTTON_TAG_ABTAST_PERIODE den selection
   * Status wenn der Button gedrückt wurde und die BUTTON_SCHUTZ_PERIODE für diesen Button
   * abgelaufen ist
   *
   */
  if (currentMillis - startMillisButtonsTagAbtast >= BUTTON_TAG_ABTAST_PERIODE)
  {
    startMillisButtonsTagAbtast = millis(); // abtast Periode resetten
    for (int i = 1; i < 7; i++)
    {
      if (currentMillis - startMillisButtonsSchutz[i] >= BUTTON_SCHUTZ_PERIODE)
      {
        startMillisButtonsSchutz[i] = millis(); // Button Schutz Periode resetten
        if (digitalRead (buttonPins[i]) == HIGH)          // Button Selection umkehren wenn Button gedrückt
        {
          tageButtonValues[i] = !tageButtonValues[i];
        }
      }
    }
  }
}
bool abfrage_ok_button(){
/**
 * @brief fragt periodisch den OK Button Wert ab und gibt ihn zurück
 * @param buttonOkValue   Button Value (out)
 * 
 */
  bool buttonOkValue = false;
  if (currentMillis - startMillisButtonsOkAbtast >= BUTTON_OK_ABTAST_PERIODE)
  {
    startMillisButtonsOkAbtast = millis(); // abtast Periode resetten
    buttonOkValue = (digitalRead(OK_BUTTON) == HIGH);
  }
  return buttonOkValue;

}
void waiting_for_start()
{
  /**
   * @brief loopt bis ok button gedrückt wird. Liest Tag Button Values aus, speichert diese und gibt 
   * die eingestellten Tage via LED Front an User aus
   * 
   */
  while(1){
  currentMillis = millis();      // aktuelle Zeit speichern  
  update_tage_button_selection(); 
  //Serial.println(digitalRead(2));
  //delay(500);
  LED_schalten();
  if (abfrage_ok_button())  // Abbruchkriterium: OK-Button gedrückt
  {
    break;
  }
}
}
void loop()
{
  currentMillis = millis();      // aktuelle Zeit speichern
  //delay(2000);
  waiting_for_start();
  //LED_schalten();

  /* ALT -> prüfen
waiting_for_start();
  if(turn_to_nupsi() == false)
    return;
  cut_blister();
  for(int i=0; i< (PILLS_IN_BLISTER - 2); i++)
  {
    if(turn_to_nupsi() == false)
      return;
    cut_blister();
    press_pill();
  }
  if(turn_to_nupsi() == false)
    return;
  press_pill(); 
  // Auswerfen der Blisterhalterung
  */
}

/* ab hier ALT -> prüfen
bool turn_to_nupsi()
{
  analogWrite(IN1, SPEED);
  analogWrite(IN2, 0);
  delay(1000);
  unsigned long mytime = millis();
  delay(100);
  while (true)
  {
    int potValue = digitalRead(LIGHT_GATE);   //Wert des Potentiometers auslesen
    if(potValue == HIGH)
    {
      analogWrite(IN1, 0);
      analogWrite(IN2, 0);
      return true;
    }
    unsigned long time_since_start = (millis()-mytime);
    if(time_since_start >= TIME_TO_NUPSI)
    {
      analogWrite(IN1, 0);
      analogWrite(IN2, 0);
      delay(1000);
      analogWrite(IN1, 0);
      analogWrite(IN2, SPEED); 
      delay(TIME_TO_NUPSI);
      analogWrite(IN1, 0);
      analogWrite(IN2, 0); 
      return false;
    }
  }
}
void cut_blister()
{
  servo_cut.write(180);
  delay(1000);
  servo_cut.write(0);
  delay(1000);
}
void press_pill()
{
  servo_press.write(180);
  delay(1000);
  servo_press.write(0);
  delay(1000);
  
}
*/