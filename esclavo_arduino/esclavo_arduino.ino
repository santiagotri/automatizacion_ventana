#include <Servo.h>

Servo myservo; 


//ESTOS PINS SE PUEDEN CAMBIAR DE ACUERDO A LOS CANALES DIGITALES USADOS
int pos = 0;    
const int piezoPin = 8;
const int pinServo = 9;

const int relay1 = 53;
const int relay2 = 52;

const int pinA = 31;
const int pinB = 33;
const int pinC = 35;
const int pinD = 37;
const int pinE = 39;

const int pinF = 41;
const int pinG = 43;

bool A = false;
bool B = false;
bool C = false;
bool D = false;

int posicionVentana = 4;

//ESTOS SON LOS TIEMPOS DE APERTURA, DEBEN SER ACOMODADOS PARA CADA ACTUADOR LINEAL
int tiemposApertura [] = {6500, 6500, 6500, 10000};
// 0-1, 1-2, 2-3, 3-4

//ESTOS SON LOS TIEMPOS DE CERRADO, DEBEN SER ACOMODADOS PARA CADA ACTUADOR LINEAL
int tiemposCerrado [] = {10000, 6750, 6800, 6800};
// 0-1, 1-2, 2-3, 3-4

void cerrar_persianas() {
  for (pos = 25; pos <= 160; pos += 2) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(20);                       // waits 15ms for the servo to reach the position
  }
  delay(1000);
  myservo.write(150);
  tonoCerrado();
}


void abrir_persianas() {
  for (pos = 150; pos >= 0; pos -= 2) { // goes from 180 degrees to 0 degrees
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(20);                       // waits 15ms for the servo to reach the position

  }
  delay(1000);
  myservo.write(25);
  tonoAbierto();
}

void mover_ventana(int nuevaPosicionVentana) {
  bool sentido = false; //true abrir false cerrar
  int tiempo = 0;
  if (posicionVentana < nuevaPosicionVentana) {
    for (int i = posicionVentana; i < nuevaPosicionVentana; i++) {
      tiempo += tiemposApertura[i];
    }
    sentido = true;
  } else if (posicionVentana > nuevaPosicionVentana) {
    for (int i = posicionVentana - 1; i >= nuevaPosicionVentana; i--) {
      tiempo += tiemposCerrado[i];
    }
    sentido = false;
  }
  posicionVentana = nuevaPosicionVentana;
  mover_brazo(tiempo, sentido);
}

void mover_brazo(int ms, bool sentido) {
  if (sentido) digitalWrite(relay2, LOW);
  else if (!sentido) digitalWrite(relay1, LOW);
  int msTemp = ms;
  int intervaloEntreSonido = 2000;
  int duracionSonido = 200;
  tone(piezoPin, 1760, 500);//La
  while (msTemp>intervaloEntreSonido) {
    delay(intervaloEntreSonido-duracionSonido);
    tone(piezoPin, 2349, duracionSonido);//Re
    msTemp= msTemp-intervaloEntreSonido;
  }
  delay (msTemp);
  digitalWrite(relay2, HIGH);
  digitalWrite(relay1, HIGH);
}

void interpretarLecturas() {
  if (B) {
    //Ventana
    if (A) {
      //Abrir
      if (!C && !D) {
        //VENTANA_25%
        Serial.println("Abriendo ventana al 25%");
        mover_ventana(1);
      } else if (!C && D) {
        //VENTANA_50%
        Serial.println("Abriendo ventana al 50%");
        mover_ventana(2);
      } else if (C && !D) {
        //VENTANA_75%
        Serial.println("Abriendo ventana al 75%");
        mover_ventana(3);
      } else if (C && D) {
        //VENTANA_100%
        Serial.println("Abriendo ventana al por completo");
        mover_ventana(4);
      }
    } else if (!A) {
      //Cerrar
      if (C && D) {
                
        //CERRAR_VENTANA_EXTRAORDINARIO
      } else if (C & !D) {
        //CERRAR_VENTANA
        Serial.println("Cerrando ventana");
        mover_ventana(0);
      }
    }
  } else if (!B) {
    //Persiana
    if (A) {
      //Abrir
      if (C && D) {
        Serial.println("Abriendo persianas");
        //ABRIR_PERSIANAS
        abrir_persianas();
      }
    } else if (!A) {
      //Cerrar
      if (!C && D) {
        Serial.println("Cerrando persianas");
        //CERRAR_PERSIANAS
        cerrar_persianas();
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  myservo.attach(pinServo, 400, 2500);

  pinMode(pinA, INPUT);
  pinMode(pinB, INPUT);
  pinMode(pinC, INPUT);
  pinMode(pinD, INPUT);
  pinMode(pinE, INPUT);

  pinMode(pinF, OUTPUT);
  pinMode(pinG, OUTPUT);

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);

  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);

  tonoSuccess();

}

void loop() {

  digitalWrite(pinF, HIGH);
  digitalWrite(pinG, LOW);

  if (digitalRead(pinE) == HIGH) {

    Serial.println("Se ha recibido un envío");
    delay(50);


    if (digitalRead(pinA) == HIGH) A = true;
    else if (digitalRead(pinA == LOW)) A = false;

    if (digitalRead(pinB) == HIGH) B = true;
    else if (digitalRead(pinB == LOW)) B = false;

    if (digitalRead(pinC) == HIGH) C = true;
    else if (digitalRead(pinC == LOW)) C = false;

    if (digitalRead(pinD) == HIGH) D = true;
    else if (digitalRead(pinD == LOW)) D = false;

    Serial.println("Enviando confirmacion de recepcion...");
    digitalWrite(pinG, HIGH);

    interpretarLecturas();

    Serial.println("escuchando de nuevo");
  }


}




void tonoSuccess() {
  tone(piezoPin, 1975, 150); //Si
  delay(150);
  tone(piezoPin, 1661, 150); //la#
  delay(150);
  tone(piezoPin, 1975, 150);//Si
  delay(150);
  tone(piezoPin, 1661, 150); //la#
  delay(150);
  tone(piezoPin, 1479, 250);//fa#
  delay(250);
  tone(piezoPin, 2217, 87);//do#
  delay(87);
  tone(piezoPin, 1975, 150);//Si
  delay(150);
  //tone(piezoPin, 1000, 500);
}

void tonoCerrado() {
  tone(piezoPin, 2349, 150);//Re
  delay(150);
  tone(piezoPin, 1760, 150);//La
  delay(150);

}

void tonoAbierto() {

  tone(piezoPin, 1760, 150);//La
  delay(150);
  tone(piezoPin, 2349, 150);//Re
  delay(150);
}
