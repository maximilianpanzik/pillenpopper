#include <Servo.h>

// A-E: LED Pins für Charlieplexing
#define E 8
#define D 9
#define C 10
#define B 11
#define A 12

#define OK_BUTTON 5

//Servo Pins
#define SERVO_PIN_SORTIERER
#define SERVO_PIN_VORSCHUB 6
#define SERVO_PIN_SCHNEID 7
#define SERVO_PIN_DRUCK 8

#define LICHTSCHRANKE_VORSCHUB 2

#define LED_PERIODE 1
#define BUTTON_SCHUTZ_PERIODE 1000
#define BUTTON_TAG_ABTAST_PERIODE 100
#define BUTTON_OK_ABTAST_PERIODE 100
#define SERVO_VORSCHUB_SPEED 60       //max = 70
#define PILLS_IN_BLISTER 5

int buttonPins[7] = {2,3,3,3,3,3,3}; //noch zu defnieren!!

unsigned long currentMillis;               // vergangene Zeit in ms seit Programmstart
unsigned long startMillisLed;              // aktuelle LED periode
unsigned long startMillisButtonsSchutz[7]; // aktuellte Button Schutz Periode
unsigned long startMillisButtonsTagAbtast; // aktuelle Tage Button abtast Periode
unsigned long startMillisButtonsOkAbtast;  // aktuelle Ok Button abtast Periode
unsigned long startMillisServoVorschub;    // aktuelle Servo Vorschub Periode
unsigned long startMillisServoDruck;       // aktuelle Servo Druck Periode
unsigned long startMillisServoSchneid;      // aktuelle Servo Schneid Periode

int charlieplexingLeds[2][7][2] =                           // schaltplan, um LED # mit charlieplexing blau oder grün zu schalten
    {
        {{C, A}, {C, B}, {B, C}, {B, D}, {B, E}, {E, A}, {E, B}},  // blau
        {{B, A}, {A, B}, {A, C}, {A, D}, {A, E}, {D, A}, {D, B}} // grün
};      //  1       2       3       4       5        6       7

int ledIncrement = 0;
//bool tageButtonValues[] = {1,1,1,1,1,1,1};      // 0: nicht aktiv, 1: aktiv
bool tageButtonValues[] = {0,1,0,1,1,1,1};        // 0: nicht aktiv, 1: aktiv
//int fuellStand[] = {0, 0, 0, 0, 0, 0, 0};       // 0: nicht voll, 1: voll
int fuellStand[] = {1, 1, 1, 0, 0, 0, 0};         // 0: nicht voll, 1: voll

int status = 0; // 0: waiting for start, 1: turn to nupsi, 2: cut, 3: cut&press, 4: press
int statusSortierer = 0; //0: not in position, 1: in position

bool eingefahren = true;
unsigned long myTime = 0;
int nubsi = 0;
bool didIt = false;

//Servo objekte erstellen
Servo servoSortierer;
Servo servoVorschub; 
Servo servoSchneid;
Servo servoDruck;

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
  startMillisServoVorschub = millis();
  startMillisServoDruck = millis();
  startMillisServoSchneid = millis();

  for (int i = 0; i < 7; i++)
  {
    startMillisButtonsSchutz[i] = millis();
    pinMode(buttonPins[i],INPUT);
  }
  //servos attachen und zurück stellen
  servoVorschub.attach(SERVO_PIN_VORSCHUB);
  servoSchneid.attach(SERVO_PIN_SCHNEID);
  servoDruck.attach(SERVO_PIN_DRUCK); 
  servoSchneid.write(5);
  servoDruck.write(5);

  pinMode(OK_BUTTON, INPUT_PULLUP);

  lightshow(); //lichtwelle
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

      // blau (charlieplexingLeds[0][]) wenn tag # fuellstand leer,
      // grün (charlieplexingLeds[1][]) wenn tag # fuellstand voll
      light(charlieplexingLeds[fuellStand[ledIncrement]][ledIncrement]);
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
      light(charlieplexingLeds[i][j]);
      delay(100);
    }
        for(int j = 6; j>-1; j--){
      light(charlieplexingLeds[i][j]);
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
bool turn_to_nupsi()
/**
 * @brief Schiebt die Blisterfixierung so lange weiter, bis Lichtschranke von Nupsi unterbrochen ist.
 * @param nupsiInLichtschranke Ist Lichtschranke von Nupsi unterbrochen?
 * 
 */
{
  bool nupsiInLichtschranke;
  if (!digitalRead(LICHTSCHRANKE_VORSCHUB)) //Fall: Nupsi unterbricht Lichtschranke nicht
  {
    nupsiInLichtschranke = false;
    //360 grad Servo PWM steuern
    if (currentMillis - startMillisServoVorschub <= (70 - SERVO_VORSCHUB_SPEED))
    {
      servoVorschub.write(0);
    }
    else
    {
      if (currentMillis - startMillisServoVorschub <= SERVO_VORSCHUB_SPEED)
      {
        servoVorschub.write(90);
      }
      else
      {
        startMillisServoVorschub = millis();
      }
    }
  }
  else //Fall: Nupsi unterbricht Lichtschranke
  {
    bool nupsiInLichtschranke = true;
    servoVorschub.write(90); //Servo anhalten
  }
  return nupsiInLichtschranke;
}
bool cut_blister()
{
  bool done;
  if (currentMillis - startMillisServoSchneid >= 2000){
    done = true;
  }
  else if(currentMillis - startMillisServoSchneid >= 1000){
    servoSchneid.write(5);
    done = false;
  }
  else{
    servoSchneid.write(65);
    done = false;
  }
  return done;
}
bool press_pill()
{
  bool done;
  if (currentMillis - startMillisServoDruck >= 2000){
    done = true;
  }
  else if(currentMillis - startMillisServoDruck >= 1000){
    servoDruck.write(5);
    done = false;
  }
  else{
    servoDruck.write(110);
    done = false;
  }
  return done;
}
void loop()
{
  currentMillis = millis(); // aktuelle Zeit speichern
  // delay(2000);
  waiting_for_start();
  // LED_schalten();

  waiting_for_start();
  if (turn_to_nupsi() == true)
  {
    cut_blister();
  }
  for (int i = 0; i < (PILLS_IN_BLISTER - 2); i++)
  {
    if (turn_to_nupsi() == false)
    {
      cut_blister();
      press_pill();
    }
  }
  if (turn_to_nupsi() == false)
  {
    press_pill();
  }
  // Auswerfen der Blisterhalterung
}