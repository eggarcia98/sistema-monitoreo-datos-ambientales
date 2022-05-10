// Nodo01 (Transceptor) Primer nodo hijo del Nodo Central

#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#include <Wire.h>
#include <BH1750.h>
#include "DHT.h"
#include "LowPower.h"

#define pin_Lluvia A1
#define DS3231_I2C_ADDRESS 0x68

#define DHTPIN 2 // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

const int valor_Capacitivo_Aire = 800; // Valor calculado con el sensor al aire
const int valor_Capacitivo_Agua = 427 ; // Valor calculado con el sensor sumergido en agua

BH1750 medidorLuz;
RF24 radio(8, 10); // nRF24L01 (CE,CSN)
RF24Network network(radio); // Incluir la radio en la red
const uint16_t this_node = 01; // Dirección de este nodo en formato Octal
const uint16_t node00 = 00;

float data[4]; // Matriz para recibir los datos del nodo011 del nivel 2.
float dataa[8]; // Matriz para enviar los datos del nodo actual y nodo011.
float temp; // Variable para almacenar el valor de Temperatura
float hum; // Variable para almacenar el valor de Humedad
float lux; // Variable para almacenar el valor de Luminosidad
int lluvia; //0 - 1023

/**
 * @author William Joseph Ayala Quinde
 * @author Gabriel Stefano Villanueva Rosero
 * Se modifica el tipo de variable del contador para ahorrar espacio en la memoria flash
 */
byte cont = 0;

// Timer DS3231 Variables
String current_time = "";
int init_hour = 18; // 6PM
int last_hour = 5; // 5AM

byte year, month, day, hour, minute;
byte second;

void setup() {

  Serial.begin(9600);
  SPI.begin();
  Wire.begin();
  radio.begin();
  dht.begin();
  network.begin(120, this_node); //(channel, node address)
  //radio.setDataRate(RF24_1MBPS);
  //radio.setDataRate(RF24_2MBPS);
  radio.setDataRate(RF24_250KBPS);

  //Si el modulo es nrf24l01+PA+LNA           PA,    LNA,   mA
  //radio.setPALevel(RF24_PA_MIN,1); //    -18 dBm, -6dBm, 7 mA
  //radio.setPALevel(RF24_PA_LOW,1); //    -12 dBm, 0dBm,  7.5 mA
  radio.setPALevel(RF24_PA_HIGH, 1); //      -6 dBm,  3dBm,  9 mA
  //radio.setPALevel(RF24_PA_MAX,1); //      0 dBm,  7dBm,  11.3 mA

  medidorLuz.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2);
  /*if (medidorLuz.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("BH1750 Advanced begin"));
    }
    else {
    Serial.println(F("Error initialising BH1750"));
    }
  */
}

void loop() {
   if (!isOperativeTime()){
    enableSleepMode();
  }else{
    network.update(); //Devuelve el tipo de la última carga útil recibida. //las cargas útiles se redirigen, reciben y ocurre toda la acción.
    RF24NetworkHeader header(011); // Encabezado para recibir los datos del Nodo011.
    RF24NetworkHeader header2(node00); // Encabezado para enviar los datos al nodo Gateway.
    leerSensores();
    //recibir_Datos(header);
    enviar_Datos(header2); 
    displayTimeDS3231(); 
  }
}

void leerSensores() {

  temp = dht.readTemperature();
  hum = dht.readHumidity();
  lux = medidorLuz.readLightLevel();
  lluvia = analogRead(pin_Lluvia);  
  if (medidorLuz.measurementReady()) {
    lux = medidorLuz.readLightLevel();
  }
  
  //Datos sensados del nodo actual
  dataa[0] = temp;
  dataa[1] = hum;
  dataa[2] = lux;
  dataa[3] = lluvia;
}

void recibir_Datos(RF24NetworkHeader header) {

  static bool bandera = false;
  network.update();
  while (network.available()) { //True, si hay un mensaje disponible para este nodo.

    network.read(header, &data, sizeof(data)); // Leer los datos del nodo011 devuelve //El número total de bytes copiados en message
    delay(random(5, 20));
    // 011 octal equals to 9 in decimal
    if (header.from_node == 9) { //Si los datos provienen del nodo011, entra al if.
      dataa[4] = data[0]; // Guardar los datos recibidos en el nodo actual.
      dataa[5] = data[1];
      cont = 0; // Reseteo el contador.
      bandera = true; //Para entrar al if

    }
  }

  if ( (network.available() == false) && bandera ) {
    cont++;
    //Serial.println(cont);
    if (
      /**
       * @author William Joseph Ayala Quinde
       * @author Gabriel Stefano Villanueva Rosero
       * Se modifica el valor de comparación del contador para que
       * el cambio en la comunicación se note inmediatamente si no hay mensajes
       * por parte del nodo hijo
       */
      cont == 1) {
      dataa[4] = dataa[5] = 0; // Reset de valores
      cont = 0; // Reset de contador.
      bandera = false; //Para no volver entrar al if.
    }
  }
}

void enviar_Datos(RF24NetworkHeader header) {


  // El tamaño de payload (carga útil) máximo predeterminado es 120 bytes.
  bool ok = network.write(header, &dataa, sizeof(dataa)); // Enviando datos del nodo01 o nodo011 al nodo central.
  delay(random(5, 20));

  if (ok) {

    Serial.println("------------------Nodo 01-------------------");
    Serial.println("Temp: " + String(dataa[0]) + " C \t Hum: " + String(dataa[1]) + " %");
    Serial.println("Luz: " + String(dataa[2]) + " lx" + "\tLluvia: " + String(dataa[3]));
    Serial.println("------------------Nodo 011------------------");
    Serial.println("Temp: " + String(dataa[4]) + " C \t Hum: " + String(dataa[5]) + " % RH");
    Serial.println("!Se envio los datos al Nodo Gateway con exito!\n");

  } else {

    delay(random(5, 20));
    network.write(header, &dataa, sizeof(dataa));
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
