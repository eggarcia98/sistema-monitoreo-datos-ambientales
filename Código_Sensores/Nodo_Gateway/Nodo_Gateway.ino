// Nodo 00, Nodo Central o Nodo Gateway
#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#include <SimpleTimer.h>
#include <Wire.h>
#include "LowPower.h"

#define DS3231_I2C_ADDRESS 0x68

RF24 radio(8, 10); // Crea una instancia y envía los pines (CE,CSN) del nrf24l01+
RF24Network network(radio); // Construye la red
const uint16_t this_node = 00; // Dirección del nodo actual en formato Octal

float data[8]; // N° total de sensores del nodo del nivel 1 y nivel 2
String tramaFinal; // Trama de toda la red.
String trama1[5]; // Trama nivel 1; trama1[0] = nodo01, ….. , trama1[4] = nodo05.
String trama2[4]; // Trama nivel 2; trama2[0] = nodo011, ...... , trama2[3] = nodo014.

int cont01 = 0, cont02 = 0, cont03 = 0, cont04 = 0, cont05 = 0;
bool bandera_Red = false;
static bool bandera01 = false, bandera02 = false, bandera03 = false, bandera04 = false, bandera05 = false;

SimpleTimer firstTimer;

// Timer DS3231 Variables
String current_time = "";
int init_hour = 18; // 6PM
int last_hour = 5; // 5AM

byte year, month, day, hour, minute;
byte second;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  radio.begin();
  network.begin(120, this_node); // Canal y dirección del nodo
  radio.setDataRate(RF24_250KBPS); // Velocidad de transmisión de datos
  //radio.setPALevel(RF24_PA_MAX,1); // Nivel PA, estado LNA

  //Si el modulo es nrf24l01+PA+LNA           PA,    LNA,   mA
  //radio.setPALevel(RF24_PA_MIN,1); //    -18 dBm, -6dBm, 7 mA
  //radio.setPALevel(RF24_PA_LOW,1); //    -12 dBm, 0dBm,  7.5 mA
  //radio.setPALevel(RF24_PA_HIGH,1); //   -6 dBm,  3dBm,  9 mA
  radio.setPALevel(RF24_PA_MAX, 1); //       0 dBm,  7dBm,  11.3 mA
}

void loop() {
  if(!isOperativeTime()){
    enableSleepMode();
  }else{
    network.update(); // Mantiene la capa en funcionamiento
    while (network.available() && !bandera_Red) {
      //Serial.println("Nodos hijos detectados...");
      //bandera_Inicio = true;
      firstTimer.setInterval(3000);
      bandera_Red = true; // Para no volver entrar al while?
    }
  
    if (firstTimer.isReady() /*&& bandera_Inicio*/ /*&& !flag*/) {
      firstTimer.reset();
      mostrar_Datos();
      //flag = true; //Para no volver entrar al if.
    }
  
    leer_Red_Sensores();
  }

}

void leer_Red_Sensores() {

  network.update(); //Esta función se debe llamar con regularidad para mantener la capa en funcionamiento. Aquí es donde las cargas útiles se redirigen, reciben y ocurre toda la acción.

  while (network.available()) {  // ¿Hay datos entrantes en el NC?
    RF24NetworkHeader header2;

    //Lee el siguiente header disponible sin avanzar al siguiente mensaje entrante.
    //Útil para cambiar el tipo de mensaje
    //network.peek(header2);

    //Lee la siguiente payload disponible sin avanzar al siguiente mensaje entrante.
    //Útil para hacer una capa de manipulación de paquetes transparente encima de RF24Network .
    //network.peek(header2,&data, sizeof(data));


    network.read(header2, &data, sizeof(data));
    if (header2.from_node == 1) {
      //Serial.print("-1");
      trama1[0] = validar_Datos_n1(data[0], data[1], data[2], data[3]); //Trama nodo01
      //trama2[0] = validar_Datos_n2(data[4], data[5]); //Trama nodo011
      cont01 = 0; // Reseteo el contador.
      bandera01 = true; //Para entrar al if
      //network.peek(header2,&data, sizeof(data));
      //network.peek(header2);
    }


  }
  validar_Trama();

}


void mostrar_Datos() {

  ///*
  Serial.println("\n\t\t     Red de Sensores Ambientales");
  Serial.println(" -----------------------------------------------------------------");
  Serial.println("| N°|  Nodo   |  Temperatura[°C]     Humedad[%]   Luz[lx]   Lluvia");
  Serial.println("| 1 | Nodo01  |\t      " + trama1[0]);
  displayTimeDS3231();
  /*Serial.println("| 2 | Nodo02  |\t      " + trama1[1]);
    Serial.println("| 3 | Nodo03  |\t      " + trama1[2]);
    Serial.println("| 4 | Nodo04  |\t      " + trama1[3]);
    Serial.println("| 5 | Nodo05  |\t      " + trama1[4]);
    Serial.println("| 6 | Nodo011 |\t      " + trama2[0]);
    Serial.println("| 7 | Nodo012 |\t      " + trama2[1]);
    Serial.println("| 8 | Nodo013 |\t      " + trama2[2]);
    Serial.println("| 9 | Nodo014 |\t      " + trama2[3]);
    Serial.println(" -----------------------------------------------------------------");
    //*/


  /*
    tramaFinal = "\n1-" + trama1[0] + //  1-31.00-0-19-1023
          "\n2-" + trama1[1] +      //  2-30.74-0-83-1023
          "\n3-" + trama1[2] +      //  3-30.83-0-55-1023
          "\n4-" + trama1[3] +      //  4-30.27-0-89-1023
          "\n5-" + trama1[4] +      //  5-30.87-0-35-1023
          "\n6-" + trama2[0] +      //  6-30.76-0-41-1023
          "\n7-" + trama2[1] +      //  7-30.91-0-21-1023
          "\n8-" + trama2[2] +      //  8-30.76-0-22-1023
          "\n9-" + trama2[3];       //  9-30.60-0-49-1023
    Serial.println(tramaFinal + "\n");
    //*/
}

String validar_Datos_n1(float sensor1 , float sensor2, float sensor3, float sensor4) {
  String trama1; //sensor1 = temperatura ; sensor2 = %HR ; sensor3 = Radiacion solar ; sensor4 = precipitacion
  if ( sensor1 == 0 && sensor2 == 0 && sensor3 == 0 && sensor4 == 0) {
    trama1 = "";
  } else {
    trama1 = String(sensor1) + "\t\t" + String(int(sensor2)) + "\t" + String(sensor3) + "\t" + String(int(sensor4));
    //trama1 = String(sensor1)+"-"+ String(int(sensor2))+"-"+String(sensor3)+"-"+String(int(sensor4)); //Formato para enviarlo a la raspberry
  }
  return trama1;
}

void validar_Trama() {

  if (bandera01) {
    cont01++;
    //Serial.println(cont01);
    if (
        /**
         * @author William Joseph Ayala Quinde
         * @author Gabriel Stefano Villanueva Rosero 
         * Se modifica el valor de comparación del contador para que
         * el cambio en la comunicación se note inmediatamente si no hay mensajes 
         * por parte del nodo hijo
         */
      cont01 == 2) {
      trama1[0] = trama2[0] = ""; // No muestro nada por serial al nodo01 y nodo011.
      cont01 = 0; // Reseteo el contador.
      bandera01 = false; //Para no volver entrar al if.
    }
  }
}

/**
 * Este método realiza la conversión de un valor BCD a DEC
 * @author Erick García, Debbie Donoso
 * @params val es un valor en byte del decimal en codigo binario
 * @return Un byte que representa un valor decimal (hora|minutos|segudo|dia|mes|año)
*/
byte bcdToDec(byte val){
  return ( (val / 16 * 10) + (val % 16) );
}

/**
 * Este método obtiene la hora actual del timer DS3231 y determina si se encuentra en una hora operativa 
 * @author Erick García, Debbie Donoso
 * @return Verdadero si es hora operativa, falso si no
*/
boolean isOperativeTime(){
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  
  boolean sleep_flag = int(hour) >= init_hour && (int(hour) <= last_hour && int(minute) <= 0 );
  return sleep_flag ;
}

/**
 * Este método mantiene el dispositivo arduino en estado de reposo durante 8S por iteración
 * @author Erick García, Debbie Donoso
 * @return No retorna.
*/
void enableSleepMode(){
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  
  boolean sleep_flag = int(hour) >= init_hour && (int(hour) <= last_hour && int(minute) <= 0 );
  do {
      sleep_flag = int(hour) >= init_hour && (int(hour) <= last_hour && int(minute) <= 0 );
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  } while (sleep_flag);
}

/**
 * Este método mantiene el dispositivo arduino en estado de reposo durante 8S por iteración
 * @author Erick García, Debbie Donoso
 * @params: 
 *    *second puntero hacia un espacio de 1 byte de memoria reservado para almacenar segundos
 *    *minute puntero hacia un espacio de 1 byte de memoria reservado para almacenar minutos
 *    *hour puntero hacia un espacio de 1 byte de memoria reservado para almacenar horas
 *    *dayOfWeek puntero hacia un espacio de 1 byte de memoria reservado para almacenar días de la semana
 *    *dayOfMonth puntero hacia un espacio de 1 byte de memoria reservado para almacenar días del mes
 *    *month puntero hacia un espacio de 1 byte de memoria reservado para almacenar meses
 *    *year puntero hacia un espacio de 1 byte de memoria reservado para almacenar años
 * @return No retorna.
*/
void readDS3231time(byte *second,
                    byte *minute,
                    byte *hour,
                    byte *dayOfWeek,
                    byte *dayOfMonth,
                    byte *month,
                    byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

/**
 * Este método imprime por consola serial la hora y fecha actual del timer DS3231
 * @author Erick García, Debbie Donoso
 * @return Imprime por consola.
*/
void displayTimeDS3231() {
  current_time = "";
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  // send it to the serial monitor
  Serial.print(hour, DEC);
  current_time += String(hour);
  
  // convert the byte variable to a decimal number when displayed
  Serial.print(":");
  current_time += ":";
  if (minute < 10)
  {
    Serial.print("0");
    current_time += "0";
  }
  Serial.print(minute, DEC);
  current_time += String(minute);
  Serial.print(":");
  current_time += ":";
  if (second < 10)
  {
    Serial.print("0");
    current_time += "0";
  }
  Serial.print(second, DEC);
  current_time += String(second);
  Serial.print(" ");
  Serial.print(dayOfMonth, DEC);
  Serial.print("/");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(year, DEC);
  Serial.println("");
  delay(1000);
}
