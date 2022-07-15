#include <Servo.h> //Die Servobibliothek wird aufgerufen. Sie wird benötigt, damit die Ansteuerung des Servos vereinfacht wird.
#include <Time.h>

const int BUTTON_PIN =  5;

const int SERVO_PIN_VORSCHUB = 6;

const int SERVO_PIN_SCHNEID = 7;

const int SERVO_PIN_DRUCK = 8;

const int sensorPin = 2;

bool eingefahren = true;

unsigned long myTime = 0;

int nubsi = 0;

Servo servovorschub; 
Servo servoschneid;
Servo servodruck;

 

void setup()

{

  Serial.begin(9600);

  servovorschub.attach(SERVO_PIN_VORSCHUB);
  servoschneid.attach(SERVO_PIN_SCHNEID);
  servodruck.attach(SERVO_PIN_DRUCK); 

  servoschneid.write(5);
  servodruck.write(0);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  //servoblau.write(0);


}

 

void loop()

{
  Serial.println(digitalRead(sensorPin));
  if(!digitalRead(sensorPin)){
    if(millis() - myTime <= 10){
      servovorschub.write(0);
    }
    else{
      if(millis() - myTime <= 60){
        servovorschub.write(90);
      }
      else{
        myTime = millis();
      }
    }
    
  }

  else{
    
    servovorschub.write(90); 

    if(nubsi < 5){
    
      // Schneiden
  
      delay(500);
      servoschneid.write(57);
      delay(1000);
      servoschneid.write(5);
      delay(1000);

      servovorschub.write(0);
      delay(10);
      servovorschub.write(90);
    }

    if(nubsi > 0 && nubsi < 6){

      // Ausdrücken
  
      delay(500);
      servodruck.write(85);
      delay(1000);
      servodruck.write(0);
      delay(1000);

      servovorschub.write(0);
      delay(10);
      servovorschub.write(90);
      
    }



    nubsi += 1;
       
    myTime = millis();
    
  }
}


//  Serial.println(digitalRead(sensorPin));
//  if (!digitalRead(sensorPin)) {
//    servoblau.write(0);
//    delay(20);
//    servoblau.write(88);
//    delay(20);
//  }
//  else{
//    servoblau.write(90);
//  }

//  if(digitalRead(BUTTON_PIN)){
//      Serial.println("button pressed");
//      servoblau.write(0);
//  }
//  else{
//    servoblau.write(90);
//  }

//}
//  servoblau.writeMicroseconds(10); 
//  delay(3000);
//  servoblau.write(250); 
//  delay(3000);


  
      
//    if(eingefahren){
//      servoblau.write(180); 
//      eingefahren = false;
//      delay(3000); 
//    }
//    
//    else{
//      servoblau.write(0);
//      eingefahren = true;
//      delay(3000);
//    }
//  }
