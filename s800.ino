
#include "sim800l.h"
#include "SoftwareSerial.h"

char *api_key="VZ3OYZQHO07104VN";
#define  NEXT_RUN 20 /* make run for report new state every XXX seconds */

void prn(char *fmt, ... );

SoftwareSerial mySerial(8,7);
sim800l gprs(6,&mySerial); // modem on software serial

void setup() {
  mySerial.begin(19200);  // open gprs port
  Serial.begin(19200); // console

  //gprs.c = &Serial; gprs.con_mode=con_inchar| con_outchar ;//| con_line; // attach console

  gprs.at("I");
  gprs.ip_attach("internet.mts.ru");

  delay(3000); // wait for modem to init


}

char a[200];


void strcatd(char *str,double v) { dtostrf( v, 2,2, str+strlen(str)); }

int my_update() {
   strcpy(a,"http://api.thingspeak.com/update.json?api_key=");
   strcat(a,api_key); // api_key
   strcat(a,"&field1=");  strcatd(a, readVcc()/100.); 
   strcat(a,"&field2=");  strcatd(a, Therm (analogRead(A7))); 
   strcat(a,"&field3=");  strcatd(a, gprs.rssi() );
   
  Serial.print("URL="); Serial.println(a);
   return gprs.wget(a); // run it
}

int run = 0; // when ti run update

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    byte ch = Serial.read();
    if (ch == 'w') gprs.wget("http://ya.ru");
    if (ch == 'u') my_update();
    if (ch== 't') gprs.wget("http://api.thingspeak.com/update.json?api_key=Y31691C0KOZAI3Z1&field1=10&field2=20&field3=30");
  }
  delay(1000);
  run--;
  if (run<=0) {
    if (my_update()) run = NEXT_RUN;
  }
 // gprs.run();
}

// helpers

int readVcc() 
{
  ADMUX = (1<<REFS0) | (1<<MUX3) | (1<<MUX2) | (1<<MUX1); 
  delay(10);
  ADCSRA |= _BV(ADSC);
  while (ADCSRA & (1<<ADSC)); 
  uint8_t low  = ADCL; 
  uint8_t high = ADCH;
  long result = (high<<8) | low;
  result = 1125300L / result /10 ; // Calculate Vcc (in mV); 1125300 = 1.1*1023*100 0
  return result; 
}

double Therm(int RawADC) {  //Function to perform the fancy math of the Steinhart-Hart equation
 
double Temp;
#ifdef thermo_pulldown
  Temp = log(((10240000/RawADC) - 10000)); // Pull -down
#else
   Temp =log(10000.0/(1024.0/RawADC-1)) ; // Pull-up
#endif
 
 Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp ))* Temp );
 Temp = Temp - 273.15;              // Convert Kelvin to Celsius
 //Temp = (Temp * 9.0)/ 5.0 + 32.0; // Celsius to Fahrenheit - comment out this line if you need Celsius
 return Temp;
}



