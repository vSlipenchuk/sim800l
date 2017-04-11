#include <Arduino.h>

#define MODEM_DEF_TIMEOUT 6000 /* in 10 msec */
#define HTTP_DEF_TIMEOUT  60 /* in seconds */
#define MODEM_DEF_APN "internet"

enum { // debug flags
  con_outchar = 1,
  con_line = 2, // print whole line
  con_inchar  = 4
  };

class sim800l {
 public:
  byte ll; byte line[254];  // collected line before flash
  byte reset_pin, lr,  ok, err; // rpin  line_ready and ok/err counters

  char clip[14]; // incoming call if detected
  char dtmf[14]; // dtmf command collecting

  byte _csq; // GSM coverage
  char _ip[16]; // ip addr if GPRS connected

  int http_resp,http_len; // answers on HTTP_ACTION
  int http_expect; // wait tofill in http_buffer
  char http_buf[80]; // read http resp buffer
  
  Stream *s; // comport stream, must be created and inited
  Stream *c; // console stream, can be null if no debug need
  byte con_mode;
 
  sim800l(byte _reset_pin, Stream *_s):reset_pin(_reset_pin) {
    s=_s;
  };
  char *gets(); // non blocked, returns NULL if no line
  void run(); // called for read-write
  void _std_answer(); //called internally to process new line
  byte  at(char *cmd,int ta=MODEM_DEF_TIMEOUT); // at command returns 1 on OK, 0 on error
  byte  atf(char *fmt,...);
  
  //CALL
  void ath() { at("h0"); clip[0]=0; dtmf[0]=0; } // drop a call
  byte ata() {  dtmf[0]=0; clip[0]=0; return at("A")&&at("+ddet=1"); } // answer and enable dtmf detection
  byte play_dtmf(char *_dtmf) { while(*_dtmf) { if (!atf("+VTS=%c",*_dtmf)) return 0;  _dtmf++; } return 1; }
  byte playOK() { return play_dtmf("11");}
  byte playERR() { return play_dtmf("81");}
  // GSM
  byte csq(); // update GSM csq coverage...
  int  rssi(); /// CSQ in dBm
  byte ip(); // update IP address (if have GPRS coverage)
  byte ip_attach(char *apn=MODEM_DEF_APN);
  // http
  byte http_connect(char *url,int ta=HTTP_DEF_TIMEOUT);
  int  http_read(char *buf,int len);
  byte http_done();  // close http session
  byte wget(char *url,char *out=0,int size=0); // get URL , responce?
};

