// wsp.c sbe16
#include <utl.h>
#include <wsp.h>
#include <mpc.h>
#include <tmr.h>

#define EOL "\r"

WspInfo wsp;

// general note: wsp wants \r only for input, outputs \r\n
// Bug:! TUTxPrint translates \n into \r\n, which sorta kinda works if lucky
//
// \r input, echos input.  \r\n before next output.
// pause 0.33s between \rn and s> prompt.
// wakeup takes 1.045s, writes extra output "SBE 16plus\r\nS>"
// pause between ts\r\n and result = 4.32s
// sbe16 response is just over 3sec in sync, well over 4sec in command

///
// sets: wsp.port .wspPending
void wspInit(void) {
  DBG0("wspInit()")
  mpcPamDev(wsp1_pam);
  wsp.port = mpcPamPort();
  tmrStop(wsp_tmr);
  if (strlen(wsp.logFile))
    wsp.log = utlLogFile(wsp.logFile);
} // wspInit

///
// uses: wsp.card
void wspStart(void) {
  int len;
  mpcPamDev(wsp1_pam);
  len = wspRead(utlBuf);
  //  mpcPamDev(wsp2_pam);
  //utlNap(20);
  //len = wspRead(utlBuf);
} // wspStart

///
// wsp started. interact.
int wspRead(char *buf) {
  int len;
  utlNap(20);
  utlWrite(wsp.port, "$WS?*", EOL);
  len = utlReadWait(wsp.port, buf, 600);
  flogf("\nwspRead()->'%s'", buf);
  return len;
} // wspRead

///
void wspStop(void){
  mpcPamDev(null_pam);
  if (wsp.log) 
    close(wsp.log);
  wsp.log = 0;
} // wspStop

///
// data waiting
bool wspData() {
  DBG1("wD")
  return TURxQueuedCount(wsp.port);
} // wspData

