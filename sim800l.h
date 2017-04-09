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
  
  // GSM
  byte csq(); // update GSM csq coverage...
  int  rssi(); /// CSQ in dBm
  byte ip(); // update IP address (if have GPRS coverage)
  byte ip_attach(char *apn=MODEM_DEF_APN);
  // http
  byte http_connect(char *url,int ta=HTTP_DEF_TIMEOUT);
  int  http_read(char *buf,int len);
  byte wget(char *url); // get URL , responce?
};

