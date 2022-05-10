#include "Wire.h"
#include <stdio.h>
#include <string.h>

#define DS3231_I2C_ADDRESS 0x68

byte year, month, day, hour, minute;
byte second;
int state = LOW;
int const sentenceSize = 20;
int sentenceIndex = 1;
char sentence[sentenceSize];

/*
* El siguiente código fue obtenido de: https://www.instructables.com/Synchronise-DS3221-RTC-with-PC-via-Arduino/
* Permite sincronizar la hora del timer DS3231 con nuestro equipo, a traves de una comuniación serial establecida 
* con ayuda del script adjunto SendTime.bat 
* @author yosoufe
*/

// Convierte un decimal en codigo binario decimal
byte decToBcd(byte val){
  return ( (val / 10 * 16) + (val % 10) );
}

// Convierte un decimal en codigo binario a numero decimal
byte bcdToDec(byte val){
  return ( (val / 16 * 10) + (val % 16) );
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
}

void loop() {
  synchTime();
}

/*
 * Esta función espera un mensaje a través del puerto serial,
 * Si existe una trama en espera en el buffer serial, entonces lo lee e intenta extraer 
 * la fecha y hora envíada desde el .bat antes mencionado
 */
void synchTime(void) {
  if (Serial.available()) {
    char ch = Serial.read();
    if (sentenceIndex <= sentenceSize && ch != '\n' && ch != '\r' && ch != ',') {
      sentence [sentenceIndex] = ch;
      sentenceIndex++;
    } else {
      sentence[sentenceIndex] = '\0';
      applySentence(sentence, sentenceIndex - 1);
      sentenceIndex = 1;
    }
  }
}

/*
 * Esta función espera un mensaje a través del puerto serial,
 * Si existe una trama en espera en el buffer serial, entonces lo lee e intenta extraer 
 * la fecha y hora envíada desde el .bat antes mencionado
 * @params 
 *    sent recibe un caracter que representa si el valor bcd es de minuto,hora,segundo,etc
 *         si sent = B entonces se presenta la hora actual del timer
 */
void applySentence (char* sent, int leng) {
  switch (sent[1]) {
    case 83:    //S = second
      {
        char secondString[3];
        strcpy(secondString, &sent[2]);
        second = atoi(secondString);
        writeOnAddress(second , 0x00);
        break;
      }

    case 68:    //D = Minute  (Daghigheh in Persian)
      {
        char minuteString[3];
        strcpy(minuteString, &sent[2]);
        minute = atoi(minuteString);
        writeOnAddress(minute , 0x01);
        break;
      }
    case 72:    //H = Hour
      {
        char hourString[3];
        strcpy(hourString, &sent[2]);
        hour = atoi(hourString);
        writeOnAddress(hour , 0x02);
        break;
      }


    case 84:   //T = Day Of Month (Tag in German)
      {
        char dayString[3];
        strcpy(dayString, &sent[2]);
        day = atoi(dayString);
        writeOnAddress(day , 0x04);
        break;
      }

    case 77:  /// M = Month
      {
        char monthString[3];
        strcpy(monthString, &sent[2]);
        month = atoi(monthString);
        writeOnAddress(month , 0x05);
        break;
      }

    case 74:   /// J = Year (Jahr in German)
      {
        char yearString[3];
        strcpy(yearString, &sent[4]);
        year = atoi(yearString);
        writeOnAddress(year , 0x06);
        toggleLED();
        break;
      }

    case 66:  ///B Write Time on Serial: You should Write "B," on serial to get time
      displayTime();
      break;
  }
}

/*
 * Esta función guarda en el timer los valores respectivos de la hora y fecha actual
 * @params 
 *    value es el valor en byte que se guardará
 *    address es la posición de memoria que ocupará value:
 *        0x00: segundos, 0x01: minutos, 0x02: horas
 *        0x04: dia, 0x05: hora, 0x06: año
 */
void writeOnAddress(byte value, int address)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(address);
  Wire.write(decToBcd(value));
  Wire.endTransmission();
}

/**
 * Este método mantiene el dispositivo arduino en estado de reposo durante 8S por iteración
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
 * @return Imprime por consola.
*/
void displayTime()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  // send it to the serial monitor
  Serial.print(hour, DEC);
  // convert the byte variable to a decimal number when displayed
  Serial.print(":");
  if (minute < 10)
  {
    Serial.print("0");
  }
  Serial.print(minute, DEC);
  Serial.print(":");
  if (second < 10)
  {
    Serial.print("0");
  }
  Serial.print(second, DEC);
}

/**
 * Este método enciendo un LED indicando que se recibió la hora y fecha por serial
*/
void toggleLED () {
  if (state == LOW) {
    state = HIGH;
  } else {
    state = LOW;
  }
  digitalWrite(13, state);
}
