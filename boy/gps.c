// antTst.c
#include <utl.h>
#include <gps.h>
#include <ant.h>
#include <tmr.h>

#define EOL "\r"

void gpsTst(void){
  char *here;
  int gpsT=90, iridT=120;
  Serial port;
  port = antPort();
  // a3laStart()
  antDevice(cf2_dev);
  TUTxPutByte(port, 3, false);
  TUTxPutByte(port, 'I', false);
  antDevice(a3la_dev);
  utlExpect(port, utlBuf, "COMMAND", 12);
  DBG1("'%s'", utlBuf)
  tmrStart(ant_tmr, gpsT);
  while (!tmrExp(ant_tmr)) {
    utlWrite(port, "AT+PD", EOL);
    utlExpect(port, utlBuf, "OK", 12);
    if (!strstr(utlBuf, "Invalid Position"))
      break;
    if (utlMatchAfter(utlStr, utlBuf, "Satellites Used=", "0123456789")) 
      flogf(" Sats=%s", utlStr);
    utlNap(3);
    utlX();
  } // while timer
  // replace crlf
  for (here=utlBuf; *here; here++) 
    if (*here=='\r' || *here=='\n')
      *here = '.';
  flogf("\n%s\n%d seconds\n", utlBuf, gpsT-tmrQuery(ant_tmr));
  // switch to irid
  antDevice(cf2_dev);
  antSwitch(irid_ant);
  antDevice(a3la_dev);
  flogf("\nCSQ\n");
  tmrStart(ant_tmr, iridT);
  while (!tmrExp(ant_tmr)) {
    utlWrite(port, "AT+CSQ", EOL);
    utlExpect(port, utlBuf, "OK", 12);
    // replace crlf
    for (here=utlBuf; *here; here++) 
      if (*here=='\r' || *here=='\n')
        *here = '.';
    flogf("%s\n", utlBuf);
    utlX();
  } // while timer
  antDevice(cf2_dev);
  TUTxPutByte(port, 3, false);
  TUTxPutByte(port, 'I', false);
}
