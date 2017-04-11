#include "sim800l.h"



void prn(char *fmt, ... ){
        char buf[128]; // resulting string limited to 128 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(buf, 128, fmt, args);
        va_end (args);
        Serial.print(buf);
}

void sim800l::_std_answer() {
char *p; int i;

if ((con_mode & con_line) && c) {  c->print("LINE:");   c->println((char*)line);  } // line debug
if (strncmp(line,"+CLIP: \"",8)==0) {
   p=line+8;
   for(i=0;(i<sizeof(clip)-1) && (p[i]!='"')&&(p[i]!=',');i++) clip[i]=p[i];
   clip[i]=0;
   //Serial.println("
 }
if (strncmp(line,"+DTMF: ",7)==0) {
  p=line+7; while (*p && *p==' ') p++;
  if (*p) { i=strlen(dtmf); if (i+1<sizeof(dtmf)) {dtmf[i]=*p; dtmf[i+1]=0;}  } // Add a DTMF
  //Serial.print("COLLEDTED_DTMF:"); Serial.println(dtmf);
  }
if ((strncmp(line,"+HTTPREAD",9)!=0) && ll>0 && http_expect) { // eat answer as responce
   //prn("expect %d bytes, here %d\n",http_expect,ll);
   if (ll>sizeof(http_buf)) ll=sizeof(http_buf);
   if (ll>http_expect) ll = http_expect;
   memcpy(http_buf,line,ll);
   http_expect = 0;
   return;
   }
if (strcmp(line,"OK")==0) ok++;
else if (strcmp(line,"ERROR")==0) err++;
else if (strcmp(line,"NO CARRIER")==0) {err++; clip[0]=0; dtmf[0]=0;} // done call
else if (strcmp(line,"BUSY")==0) {err++; clip[0]=0; dtmf[0]=0;} // done call
else if (strncmp(line,"+CSQ:",5)==0) _csq=atoi(line+5); // signal quality
else if (strncmp(line,"+SAPBR: 1,1,\"",13)==0)  { // ip reporting
      //Serial.println("SAPBR_LINE HERE");
      p=line+13;
      for(i=0;i+1<sizeof(_ip)&&p[i]!='"';i++) _ip[i]=p[i];
      _ip[i]=0;
      if (strcmp(_ip,"0.0.0.0")==0) _ip[0]=0; // clear it
      //Serial.println(_ip);
     }
else if (strncmp(line,"+HTTPACTION: ",13)==0) {
     int act;
     //Serial.println("http_action here");
     //prn("line+13=%s\n",line+13);
     if (sscanf(line+13,"%d,%d,%d",&act,&http_resp,&http_len)==3) {
          //prn("here action %d-%d-%d\n",act,http_resp,http_len);
        }
     }
line[0]=0; ll=0;
}

char *sim800l::gets() { // gets returns line or NULL
while (s->available()) {
// have modem output
  byte ch = s->read();
  if ( (con_mode & con_outchar) && c) c->write(ch); // push on console
  switch(ch) {
    case '\n':  //ignore
    case '\r': line[ll]=0; return line;  break; // new line
    default:
         if (ll+1<sizeof(line)) { line[ll]=ch; ll++; line[ll]=0;} // adding line character
    }
}
return 0;
}

void sim800l::run() { // main loop procedure
if (gets()) { // new line here
    if (line[0]) _std_answer(); // check std answer
    }
if (c) { // have console - 
    if (c->available()) {  s->write(c->read()); }
    }  
}

byte sim800l::at(char *cmd,int ta=MODEM_DEF_TIMEOUT) {
int i; 
ok=0; err=0;
if (cmd) {
  s->print("AT"); s->print(cmd); s->print("\r"); // push a command
  } 
for(i=0;i<ta;i++) {
   int j;
   for(j=0;j<10;j++) {
   if (gets()) _std_answer(); // check answers
   if (ok) return 1;
   if (err) return 0;
   delay(1);
   }
   }
return 0; // timeout returns 0;
}

byte sim800l::csq() {
  _csq=0;
  at("+CSQ");
  return _csq;
}

int sim800l::rssi() {
int r = csq();
return r*2-113;
}

byte sim800l::ip() { // update IP address
  _ip[0]=0;
  if (!at("+SAPBR=2,1")) return 0; 
  return _ip[0]!=0;
}

byte sim800l::atf(char *fmt,...) {
  va_list va;
  va_start(va,fmt);
  vsnprintf(line,sizeof(line)-1,fmt,va);
  va_end(va);
  return at(line);
}

byte sim800l::ip_attach(char *apn=MODEM_DEF_APN) {
  at("+SAPBR=3,1,\"Contype\",\"GPRS\"");
  atf("+SAPBR=3,1,\"APN\",\"%s\"",apn);
  if (ip()) return 1; // OK 
  if (!at("+SAPBR=1,1")) return 0; // request bearer failed
  return ip();
}

byte sim800l::http_done() { return at("+HTTPTERM");}

byte sim800l::http_connect(char *url,int ta=HTTP_DEF_TIMEOUT) {
int i;
at("e1");
  http_done();
  at("+HTTPINIT"); // sometimes failed, but action than successed
   // if (!atf("+HTTPPARA=\"URL\",\"%s\"",url)) return 0;
   s->print("AT+HTTPPARA=\"URL\",\"");
   s->print(url);
   s->print("\"\r");
   if (!at(0)) {
     // prn("HTTP para failed\n");
     return 0;
   }
http_resp=0; http_len=0;
  if (!at("+HTTPACTION=0")) {
     //prn("Http action failed\n");
     return 0; //
  }
 for(i=0; (i<ta) && (http_resp==0);i++) {
   int j;
   for(j=0;(j<100)&&(http_resp==0);j++) {
     run();
     delay(10);
     }
   //prn("i=%d http_resp=%d\n",i,http_resp);
   }
//prn("Done connect\n");
return http_resp==200; // Serial.println("Ok, wait for HTTP action");
}

int sim800l::http_read(char *buf,int len) { // Text just after atf before OK
int b=0,l=0,L=http_len,r=0;
at("e0"); // no echo
len--; // for zero terminated
for(b=0,l=0;(len>0)&&(L>0);b+=l,L-=l,len-=l,buf+=l,r+=l) {
  l=40; if (l>L) l=L; // read by 80 bytes but not more than we have
  http_expect=l;
  if (!atf("+HTTPREAD=%d,%d",b,l)) return -1;
  if (http_expect !=0) return -2; // not filled buffer
  memcpy(buf,http_buf,l);
  }
*buf=0;
return len; // ok
}



byte sim800l::wget(char *url,char *out=0,int size=0) {
if (csq()==0) return 0; // no GSM coverage
if (!ip()) { // try connect....
   //Serial.println("no ip, try connect");
   ip_attach();
   if (!ip()) return 0; // failed
   }
//Serial.println("OK, now begin http...");
if (!http_connect(url)) return 0;
//prn("OK, http_connected -> read %d bytes\n",http_len);
if (!out) {  http_done();   return 1; } // done ok
int r = http_read(out,size);
http_done();
if (r>0) return 1; // done
return 0;
}

