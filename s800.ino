
#include <EEPROM.h>
#include "sim800l.h"
#include "SoftwareSerial.h"


int TEMP_PIN = A7; // termo resistor
int LED = 11; // my Pinout for light
int EXPOWER = 10; // pin for detects external power
int HEATERPIN=9; // when relay connected
int HEATER_REVERSE=1; // digitalWrite(HIGH) for heater OFF

int heaterMode = 0; // no heating HEATER_REVERSE independent
void prn(char *fmt, ... );

// on relay if we need it
void setHeater( int on ) { heaterMode=on; 
  //prn("++++ Setting Heater +++++ %d ++++++\r\n",on);
  if (HEATER_REVERSE) on=!on; digitalWrite( HEATERPIN, on?HIGH:LOW); }

int debug = 0;

byte targetTemp = 0; // no control - need to place in the EEPROM

char api_key[16+1]; //="1K1MMTUL5452YPV1";
#define  NEXT_RUN 3600 /* make run for report new state every XXX seconds */



SoftwareSerial mySerial(8,7);
sim800l gprs(6,&mySerial); // modem on software serial


void eepromReadBuf(int addr,byte *buf,int size) { for(;size>0;size--,addr++,buf++) buf[0]=EEPROM.read(addr);}
void eepromWriteBuf(int addr,byte *buf,int size) { for(;size>0;size--,addr++,buf++) EEPROM.write(addr,buf[0]);}

void setTargetTemp(byte newTemp) { targetTemp=newTemp; EEPROM.write(16,targetTemp);}

void setup() {
 pinMode( LED, OUTPUT);
 pinMode(EXPOWER,INPUT);
 pinMode(HEATERPIN,OUTPUT);
  
  mySerial.begin(19200);  // open gprs port
  Serial.begin(19200); // console
    
    //eepromWriteBuf(0,"YK1MMTUL5452YPV1",16); //setTargetTemp( 0);  do it once!
    
  eepromReadBuf(0,api_key,16); api_key[16]=0; // now read it
  targetTemp = EEPROM.read(16);  // read temp

  prn("API_KEY=%s targetTemp=%d\n",api_key,targetTemp);

  gprs.c = &Serial; gprs.con_mode=con_inchar| con_outchar ;//| con_line; // attach console
     
  gprs.at("I");
  if (!debug) gprs.ip_attach("internet.mts.ru");

  delay(2000); // wait for modem to init
  Serial.println("done setup");

}

char a[200];


void strcatd(char *str,double v) { dtostrf( v, 2,2, str+strlen(str)); }
double getTemp() { return Therm (analogRead(TEMP_PIN));}

int my_update() {
  if (debug) return 0;
  updateHeater();
   strcpy(a,"http://api.thingspeak.com/update.json?api_key="); // NoMore 5 fields, some limitiations inside sim800?
   strcat(a,api_key); // api_key
   strcat(a,"&field1=");  strcatd(a, readVcc()/100.); 
   strcat(a,"&field2=");  strcatd(a, getTemp() ); 
  // strcat(a,"&field3=");  strcatd(a, gprs.rssi() );
   strcat(a,"&field4=");  strcatd(a, targetTemp );
   strcat(a,"&field5=");  strcatd(a, digitalRead(EXPOWER ) ); // external power 0-1
   strcat(a,"&field6=");  strcatd(a, heaterMode ); // heater 0-1
  Serial.print("URL="); Serial.println(a);
   prn("STRLEN=%d\r\n",strlen(a));
   return gprs.wget(a); // run it
}

int run = 0; // when ti run update
int stop_wait = 0;

int ppower = -1;

void updateHeater() {
if (targetTemp) { // temp is set
    double t = getTemp();
    int pHeater = heaterMode; // remember
    if (t>=(targetTemp+1)) setHeater( 0 ); // off heater
    if (t<=(targetTemp-1)) setHeater( 1 ); // on it
    if (pHeater != heaterMode ) { // changed !
       run = 0; // update request now
       }
    }
}

void loop() {
  int expower = digitalRead(EXPOWER);

if (ppower!=expower) { // changed!
  ppower=expower; run = 0; // report it!
  
}

updateHeater();
  if (debug) {
     int expower = digitalRead(EXPOWER);
     //prn("expower=%d\r\n",expower);
     //delay(1000);
     //return;
     
     gprs.run();
     if (gprs.clip[0]) {
        Serial.println("\n\nCALL FROM:"); Serial.println(gprs.clip);
         // if (strcmp(gprs.clip,"+79151999003")==0) digitalWrite(LED,HIGH);
         // if (strcmp(gprs.clip,"+79032725531")==0) digitalWrite(LED,LOW);
        if (strcmp(gprs.clip,"+79032725531")==0) {
          if (!gprs.ata()) gprs.ath(); // answer a call
        } else  gprs.ath(); // drop a call
       }

  if (gprs.dtmf[0]) {
     switch(gprs.dtmf[0]) {
       case '1': digitalWrite(LED,HIGH); gprs.play_dtmf("11"); break;
       case '2': digitalWrite(LED,LOW);  gprs.play_dtmf("11"); break;
       default:
          prn("Unknown command %c",gprs.dtmf[0]);
          gprs.play_dtmf("81"); // no
          
     }
     gprs.dtmf[0]=0; // clear all      
     }
       
     return ;
  }
gprs.run(); 
if (gprs.clip[0]) {
        Serial.println("\n\nCALL FROM:"); Serial.println(gprs.clip);
        if (strcmp(gprs.clip,"+79151999003")==0) { gprs.ath(); run =0; } // if my call - than force update
        if (strcmp(gprs.clip,"+79032725531")==0) {     if (!gprs.ata()) gprs.ath(); } // open controlled channel
    }
if (gprs.dtmf[0]) {
   char *dtmf = gprs.dtmf;
   int ll =strlen(dtmf);
   if (dtmf[ll-1]=='#') {
     // done command
     if (dtmf[0]=='*') { // set target temperature
       setTargetTemp( atoi(dtmf+1) );
       updateHeater();
       }
     run = 0; // force update
     gprs.ath();
     }
  }
  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    byte ch = Serial.read();
    if (ch == 'w') gprs.wget("http://ya.ru");
    if (ch == 'u') my_update();
    if (ch== 't') gprs.wget("http://api.thingspeak.com/update.json?api_key=Y31691C0KOZAI3Z1&field1=10&field2=20&field3=30");
  }
  {int i; for(i=0;i<1000 && !stop_wait;i++) { gprs.run(); delay(1);}}
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



