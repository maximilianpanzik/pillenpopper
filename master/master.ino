#include <CapacitiveSensor.h>
#include <Servo.h>
#include <Wire.h>              // Wire Bibliothek einbinden
#include <LiquidCrystal_I2C.h> // Vorher hinzugefügte LiquidCrystal_I2C Bibliothek einbinden
#include <stdio.h>
#include <string.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

// A-E: LED Pins für Charlieplexing
#define E 7
#define D 8
#define C 9
#define B 10
#define A 11

// servo pins
#define SERVO_PIN_SORTIERER 12
#define SERVO_PIN_VORSCHUB A0
#define SERVO_PIN_SCHNEID A1
#define SERVO_PIN_DRUCK A2

// lichtschranken pins
#define LICHTSCHRANKE_PILLDROP A7 //A7 nur als analog in möglich
#define LICHTSCHRANKE_VORSCHUB A6 //A7 nur als analog in möglich

// button pins
#define OK_BUTTON A3
int buttonPins[7] = {0, 1, 2, 3, 4, 5, 6};
#define CAPA_CLOCK 13

// Einstellungen
#define LED_PERIODE 1
#define BUTTON_SCHUTZ_PERIODE 1000
#define BUTTON_TAG_ABTAST_PERIODE 100
#define BUTTON_OK_ABTAST_PERIODE 100
#define LICHTSCHRANKE_PILLDROP_ABTAST_PERIODE 100
#define SERVO_VORSCHUB_SPEED 60 // max = 70
#define AUSWERFZEIT 5000        // in ms
#define BUTTON_SCHWELLE 200     //
#define LICHTSCHRANKE_SCHWELLE_PILLDROP 150
#define LICHTSCHRANKE_SCHWELLE_VORSCHUB 500

unsigned long currentMillis;               // vergangene Zeit in ms seit Programmstart
unsigned long startMillisLed;              // aktuelle LED periode
unsigned long startMillisButtonsSchutz[7]; // aktuellte Button Schutz Periode (entprellen)
unsigned long startMillisButtonsTagAbtast; // aktuelle Tage Button abtast Periode
unsigned long startMillisButtonsOkAbtast;  // aktuelle Ok Button abtast Periode
unsigned long startMillisLSPilldropAbtast; // aktuelle Lichtschranke Pilldrop abtast Periode
unsigned long startMillisServoVorschub;    // aktuelle Servo Vorschub Periode
unsigned long startMillisServoDruck;       // aktuelle Servo Druck Periode
unsigned long startMillisServoSchneid;     // aktuelle Servo Schneid Periode
unsigned long startMillisAuswerfen;        // aktuelle auswerf Periode

int charlieplexingLeds[2][7][2] = // schaltplan, um LED # mit charlieplexing blau oder grün zu schalten
    {
        {{C, A}, {C, B}, {B, C}, {B, D}, {B, E}, {E, A}, {E, B}}, // blau
        {{B, A}, {A, B}, {A, C}, {A, D}, {A, E}, {D, A}, {D, B}}  // grün
};      //  1       2       3       4       5        6       7

// operationsvariablen
int ledIncrement = 0; // aktuell schaltende LED

bool tageButtonValues[7]; // 0: nicht aktiv, 1: aktiv
bool fuellStandBox[7];    // 0: nicht voll, 1: voll
//  bool tageButtonValues[7] = {1,1,1,1,1,1,1}; // 0: nicht aktiv, 1: aktiv
//  bool fuellStandBox[7] = {0,0,0,0,0,0,0};    // 0: nicht voll, 1: voll


int blisterPosition; // Pille über Schneidestempel (0-6) 0: Blister noch nicht im System 6: Pille 5 unter Druck Stempel

int status; // 0: waiting for start, 1: turn to nupsi, 2: cut&press, 3: auswerfen, 4: Abbruch
int sortiererPosition; // 0: not in position, 1: in position

// Servo objekte erstellen
Servo servoSortierer;
Servo servoVorschub;
Servo servoSchneid;
Servo servoDruck;

//capacitive Button Objekte erstellen
CapacitiveSensor tageButtons[7] = {CapacitiveSensor(13, 0),CapacitiveSensor(13, 1),CapacitiveSensor(13, 2),CapacitiveSensor(13, 3),CapacitiveSensor(13, 4),CapacitiveSensor(13, 5),CapacitiveSensor(13, 6)};
CapacitiveSensor okButton = CapacitiveSensor(13, OK_BUTTON);

void setup()
{
  //Serial.begin(9600);

  pinMode(A, INPUT);
  pinMode(B, INPUT);
  pinMode(C, INPUT);
  pinMode(D, INPUT);
  pinMode(E, INPUT);
  pinMode(LICHTSCHRANKE_PILLDROP, INPUT);
  pinMode(LICHTSCHRANKE_VORSCHUB, INPUT);
  //pinMode(OK_BUTTON, INPUT);
  // for (int i = 0; i < 7; i++)
  // {
  //   pinMode(buttonPins[i], INPUT);
  // }

  // timer perioden starten
  startMillisLed = millis();
  startMillisButtonsTagAbtast = millis();
  startMillisButtonsOkAbtast = millis();
  startMillisServoVorschub = millis();
  startMillisServoDruck = millis();
  startMillisServoSchneid = millis();
  for (int i = 0; i < 7; i++)
  {
    startMillisButtonsSchutz[i] = millis();
  }

for (int i = 0; i<7; i++){
  fuellStandBox[i] = 0;
  tageButtonValues[i] = 1;
  }
  status = 0;
  blisterPosition = 0;

  // LCD
  lcd.init();          // Im Setup wird der LCD gestartet
  lcd.backlight();     // Hintergrundbeleuchtung einschalten (lcd.noBacklight(); schaltet die Beleuchtung aus).
  lcd.setCursor(0, 0); // Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile.
  lcd.print("Hallo! Geraet   ");
  lcd.setCursor(0, 1); // In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile.
  lcd.print("wird initiiert  ");

  // servos attachen und zurück stellen
  servoVorschub.attach(SERVO_PIN_VORSCHUB);
  servoSchneid.attach(SERVO_PIN_SCHNEID);
  servoDruck.attach(SERVO_PIN_DRUCK);
  servoSortierer.attach(SERVO_PIN_SORTIERER);
  servoSchneid.write(5);
  servoDruck.write(5);
  servoVorschub.write(90);
  servoSortierer.write(0);

  lightshow(); // lichtwelle
  LCD_schalten();
}
void LCD_schalten()
{
  switch (status)
  {
  case 0:                // waiting for start
    lcd.setCursor(0, 0); // Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile.
    lcd.print("Tage waehlen und");
    lcd.setCursor(0, 1); // In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile.
    lcd.print("mit ok starten  ");
    break;
  case 1:                // turn to nupsi
    lcd.setCursor(0, 0); // Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile.
    lcd.print("Blister wird    ");
    lcd.setCursor(0, 1); // In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile.
    lcd.print("eingezogen      ");
    break;
  case 2:                // cut&press
    lcd.setCursor(0, 0); // Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile.
    lcd.print("Box wird        ");
    lcd.setCursor(0, 1); // In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile.
    lcd.print("befuellt        ");
    break;
  case 3:                // auswerfen
    lcd.setCursor(0, 0); // Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile.
    lcd.print("Blister wird    ");
    lcd.setCursor(0, 1); // In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile.
    lcd.print("ausgeworfen     ");
    break;
  case 4:                // abbruch
    lcd.setCursor(0, 0); // Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile.
    lcd.print("Abbruch! Aktoren");
    lcd.setCursor(0, 1); // In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile.
    lcd.print("fahren zurueck  ");
    break;
  }
}
void sortierer_positionieren()
{
  /**
   * @brief positioniert sortierer abhängig von ausgewählten tagen und fuellständen
   *
   */

  int winkeltabelle[] = {0, 30, 60, 90, 120, 160, 180};
  // erste unbefüllte zu befüllende kammer finden
  for (int i = 0; i < 7; i++)
  {
    if ((tageButtonValues[i] == 1) && (fuellStandBox[i] == 0))
    {
      servoSortierer.write(winkeltabelle[i]);
      sortiererPosition = i;
      break;
    }
  }
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
   * Füllstatus (int fuellStandBox []) schaltet alle LED_PERIODE ms eine LED
   *
   **/
  // if (currentMillis - startMillisLed >= LED_PERIODE)
  // {
  //   startMillisLed = currentMillis;
    if (tageButtonValues[ledIncrement])
    { // wenn Tag # button aktiv

      // blau (charlieplexingLeds[0][]) wenn tag # fuellStandBox leer,
      // grün (charlieplexingLeds[1][]) wenn tag # fuellStandBox voll
      light(charlieplexingLeds[fuellStandBox[ledIncrement]][ledIncrement]);
    }
    // incrementieren wenn <6, sonst 0
    // ledIncrement = (ledIncrement < 6) ? ledIncrement++ : 0;
    if (ledIncrement < 6)
    {
      ledIncrement++;
    }
    else
    {
      ledIncrement = 0;
    }
//  }
}
void lightshow()
/**
 * @brief leuchtet alle LEDs auf
 *
 */
{
  for (int i = 0; i < 2; i++)
  {
    for (int j = 0; j < 7; j++)
    {
      light(charlieplexingLeds[i][j]);
      delay(100);
    }
    for (int j = 6; j > -1; j--)
    {
      light(charlieplexingLeds[i][j]);
      delay(100);
    }
  }
  // alles aus schalten
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
    
    //Serial.println("Buttonvalues:");
    for (int i = 1; i < 7; i++)
    
    {
      //Serial.println(tageButtonValues[i]);
      //if (currentMillis - startMillisButtonsSchutz[i] >= BUTTON_SCHUTZ_PERIODE)
      //{        
        
        //startMillisButtonsSchutz[i] = currentMillis; // Button Schutz Periode resetten
        if (tageButtons[i].capacitiveSensor(30) > BUTTON_SCHWELLE) // Button Selection umkehren wenn Button gedrückt
        {
          tageButtonValues[i] = !tageButtonValues[i];
        }
        Serial.println(currentMillis-millis());
        startMillisButtonsTagAbtast = currentMillis; // abtast Periode resetten
      //}
    }
  }
}
bool auf_neuen_blister_warten()
{
  //!!
  return 1;
}
bool blister_auswerfen()
{
  /**
   * 
   * 
   */
  if (currentMillis - startMillisAuswerfen > AUSWERFZEIT)
  {
    return true;
  }
  else
  {
    // 360 grad Servo PWM steuern
    if (currentMillis - startMillisServoVorschub <= (70 - SERVO_VORSCHUB_SPEED))
    {
      servoVorschub.write(180);
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
    return false;
  }
}
bool abfrage_ok_button()
{
  /**
   * @brief fragt periodisch den OK Button Wert ab und gibt ihn zurück
   * @param buttonOkValue   Button Value (out)
   *
   */
  bool buttonOkValue = false;
  if (millis() - startMillisButtonsOkAbtast >= BUTTON_OK_ABTAST_PERIODE)
  {
    startMillisButtonsOkAbtast = millis(); // abtast Periode resetten
    buttonOkValue = (okButton.capacitiveSensor(30) > BUTTON_SCHWELLE);
    //Serial.println(buttonOkValue);
  }
  return buttonOkValue;
}
void abfrage_pilldrop_lichtschranke()
{
  /**
   * @brief fragt periodisch den LS Wert ab und updatet fuellstand
   *
   */
  if (currentMillis - startMillisLSPilldropAbtast >= LICHTSCHRANKE_PILLDROP_ABTAST_PERIODE)
  {
    startMillisLSPilldropAbtast = millis(); // abtast Periode resetten
    if(analogRead(LICHTSCHRANKE_PILLDROP > LICHTSCHRANKE_SCHWELLE_PILLDROP)){
    fuellStandBox[sortiererPosition] = 1;
  }
  }
}
bool warte_auf_start()
{
  /**
   * @brief Liest Tag Button Values aus, speichert diese und gibt die eingestellten Tage
   * via LED Front an User aus.
   * @param okButtonValue periodische Abfrage von ok button (out)
   */
  bool okButtonValue = abfrage_ok_button();
  update_tage_button_selection();
  return okButtonValue;
}
bool abfrage_fuellstand()
{
  /**
   * @brief return true wenn gewünschter füllstand erreicht ist
   *
   */
  if (memcmp(fuellStandBox, tageButtonValues, sizeof(fuellStandBox)) == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}
bool vorschub_bis_nupsi()
/**
 * @brief Schiebt die Blisterfixierung so lange weiter, bis Lichtschranke von Nupsi unterbrochen ist.
 * @param nupsiInLichtschranke Ist Lichtschranke von Nupsi unterbrochen?
 *
 */
{
  bool nupsiInLichtschranke;
  if (analogRead(LICHTSCHRANKE_VORSCHUB) < LICHTSCHRANKE_SCHWELLE_VORSCHUB) // Fall: Nupsi unterbricht Lichtschranke nicht
  {
    nupsiInLichtschranke = false;
    // 360 grad Servo PWM steuern
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
  else // Fall: Nupsi unterbricht Lichtschranke
  {
    nupsiInLichtschranke = true;
    servoVorschub.write(90); // Servo anhalten
  }
  //Serial.print("nupsiInLichtschranke:");
  //Serial.println(analogRead(LICHTSCHRANKE_VORSCHUB));
  return nupsiInLichtschranke;
}
bool cut_blister()
{
  /**
   * 
   * @brief fährt schneide Servo vor, nach einer sekunde zurück und gibt nach zwei sekunden true zurück
   * 
   */ 
  bool done;
  if (currentMillis - startMillisServoSchneid >= 2000)
  {
    done = true;
  }
  else if (currentMillis - startMillisServoSchneid >= 1000)
  {
    servoSchneid.write(5);
    done = false;
  }
  else
  {
    servoSchneid.write(65);
    done = false;
  }
  return done;
}
bool press_pill()
{
   /**
   * 
   * @brief fährt ausdrück Servo vor, nach einer sekunde zurück und gibt nach zwei sekunden true zurück
   * 
   */
  bool done;
  if (currentMillis - startMillisServoDruck >= 2000)
  {
    done = true;
  }
  else if (currentMillis - startMillisServoDruck >= 1000)
  {
    servoDruck.write(5);
    done = false;
  }
  else
  {
    servoDruck.write(110);
    done = false;
  }
  return done;
}
void testPillensortierer(){
  tageButtonValues[0] = 0;
  tageButtonValues[1] = 0;
  tageButtonValues[2] = 0;
  sortierer_positionieren();
  sortierer_positionieren();
  delay(1000);
    lcd.setCursor(0, 0); // Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile.
    lcd.print("sortierer sollte");
    lcd.setCursor(0, 1); // In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile.
    lcd.print("auf 1 stehen    ");
  delay(1000);
  fuellStandBox[0] = 1;

  sortierer_positionieren();
    delay(1000);
    lcd.setCursor(0, 0); // Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile.
    lcd.print("sortierer sollte");
    lcd.setCursor(0, 1); // In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile.
    lcd.print("auf 2 stehen    ");
      delay(1000);
    fuellStandBox[1] = 1;
    fuellStandBox[2] = 1;
      sortierer_positionieren();
  delay(1000);
    lcd.setCursor(0, 0); // Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile.
    lcd.print("sortierer sollte");
    lcd.setCursor(0, 1); // In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile.
    lcd.print("auf 4 stehen    ");
  delay(5000);
    lcd.setCursor(0, 0); // Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile.
    lcd.print("Test startet von");
    lcd.setCursor(0, 1); // In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile.
    lcd.print("vorne           ");
    delay(2000);

  
}
void testAusdrucken(){
  status = 2; // press&cut
  LCD_schalten();
  startMillisServoDruck = currentMillis;
  startMillisServoSchneid = currentMillis;
  while (1)
  {
    Serial.println("presse");
    bool donep = press_pill();
    bool donec = cut_blister();
    if (donep && donec)
    {
      status = 1; // blister positionieren
      LCD_schalten();
      break;
    }
  }
  delay(2000);
  lcd.setCursor(0, 0); // Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile.
  lcd.print("ausdruck test   ");
  lcd.setCursor(0, 1); // In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile.
  lcd.print("beendet         ");
}
void testVorschub()
{
}
void testOkButton()
{
  if (okButton.capacitiveSensor(30) > BUTTON_SCHWELLE)
  {
    lcd.setCursor(0, 0); // Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile.
    lcd.print("Gedueckt        ");
    lcd.setCursor(0, 1); // In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile.
    lcd.print("                ");
  }
  else
  {
    lcd.setCursor(0, 0); // Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile.
    lcd.print("Nicht gedueckt  ");
    lcd.setCursor(0, 1); // In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile.
    lcd.print("                ");
  }
  delay(500);
  Serial.println(okButton.capacitiveSensor(30));
}
void ablauf()
{
  currentMillis = millis(); // aktuelle Zeit speichern
  // LCD_schalten();
  LED_schalten();
  //sortierer_positionieren();
  // Serial.print("Status:");
  // Serial.println(status);
  abfrage_pilldrop_lichtschranke();

  if (abfrage_fuellstand() && !(status == 4))
  {
    status = 3; // auswerfen
    LCD_schalten();
    startMillisAuswerfen = currentMillis; // auswerfen wenn fuellstand erreicht
  }

  switch (status)
  {
  case 0:                  // wait for start
    if (warte_auf_start()) // true wenn ok button gedrückt
    {
      status = 1; // blister positionieren
      LCD_schalten();
    }
    break;
  case 1:                    // blister positionieren

    if (abfrage_ok_button()) // wenn ok gedrückt -> abbruch
    {
      status = 4; // abbruch
      LCD_schalten();
      break;
    }

    if (blisterPosition < 6) // blisterPosition: Pille über Schneidestempel (0-6) 0: Blister noch nicht im System; 6: Pille 5 unter Druck Stempel
    {
      if (vorschub_bis_nupsi()) // true wenn nupsi in lichtschranke
      {
        blisterPosition++;
        status = 2; // press&cut
        LCD_schalten();
        startMillisServoDruck = currentMillis;
        startMillisServoSchneid = currentMillis;
      }
    }
    else
    {
      status = 3; // auswerfen
      LCD_schalten();
      startMillisAuswerfen = currentMillis;
    }

    break;
  case 2: // press&cut
    if (abfrage_ok_button())
    {
      status = 4; // abbruch
      LCD_schalten();
      break;
    }
    if (blisterPosition == 1)
    {
      if (cut_blister())
      {
        status = 1; // blister positionieren
        LCD_schalten();
      }
    }
    else if (blisterPosition == 6)
    {
      if (press_pill())
      {
        status = 1; // blister positionieren
        LCD_schalten();
      }
    }
    else
    {
      if (press_pill() && cut_blister())
      {
        status = 1; // blister positionieren
        LCD_schalten();
      }
    }

    break;
  case 3: // auswerfen
    if (abfrage_ok_button())
    {
      status = 4;
      LCD_schalten();
      break;
    }
    if (blister_auswerfen())
    {

      if (abfrage_fuellstand())
      {
        setup(); // reset
      }
      else
      {
        blisterPosition = 0;
        status = 1; // neuen blister einziehen
        LCD_schalten();
      }
    }
    break;
  case 4: // abbruch
    servoDruck.write(5);
    servoSchneid.write(5);
    servoVorschub.write(90);

    delay(2000);
    lcd.setCursor(0, 0); // Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile.
    lcd.print("Auswerfen mit   ");
    lcd.setCursor(0, 1); // In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile.
    lcd.print("ok bestaetigen  ");

    while (1)
    {
      if (abfrage_ok_button())
      {
        break;
      }
    }
    status = 3;
    LCD_schalten();
    startMillisAuswerfen = currentMillis;
    break;
  }
  //Serial.print("Zeit für Durchlauf");
  //Serial.println(millis() - currentMillis);
}
void loop()
{
  ablauf();
  //currentMillis = millis();
  // light(charlieplexingLeds[1][0]);
  // setup();
  // testOkButton();
  // testPillensortierer();
  //testAusdrucken();
}
