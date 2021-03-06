// ngk.c
#include <utl.h>
#include <ngk.h>
#include <sys.h>
#include <tmr.h>

#define MDM_BAUD 4800L
#define BUOY_ID '2'
#define WINCH_ID '1'    // aka ngk.winchId
#define EOL "\r\n"

NgkInfo ngk = {
  { "null",
    // ngk.msgStr[] as if sent to buoy; change ID before sending to winch
    "#B,02,00", "%B,02,00", "#F,02,00", "%F,02,00", "#R,02,03", "%R,02,00",
    "#W,02,00", "%W,02,00", "#S,02,00", "%S,02,00", "#R,02,00", "mangled",
  },
  { "null",
    "buoyCmd", "buoyRsp", "fallCmd", "fallRsp", "riseCmd", "riseRsp",
    "statCmd", "statRsp", "stopCmd", "stopRsp", "surfCmd", "mangled",
  }
}; // remainder of struct is 0 filled

///
// power on amodem
// sets: ngk.port
void ngkInit(void) {
  short mdmRX, mdmTX;
  Serial p;
  DBG0("ngkInit()");
  mdmRX = TPUChanFromPin(MDM_RX);
  mdmTX = TPUChanFromPin(MDM_TX);
  PIORead(MDM_RX_TTL);              // tpu->rs232 is pin 33->48->47
  // Power up the DC-DC for the Acoustic Modem Port
  ngkStop();
  utlDelay(SETTLE);
  ngkStart();
  utlDelay(SETTLE);
  // PIOClear(MDM_TX_TTL);             // tpu->rs232 is pin 35->50->49
  p = TUOpen(mdmRX, mdmTX, MDM_BAUD, 0);
  if (p == 0)
    utlStop("\nERR\t|ngkInit() Bad ngk serial port");
  else {
    TUTxFlush(p);
    TURxFlush(p);
  }
  ngk.port = p;
} // ngkInit

///
// power on
void ngkStart(void){
  PIOSet(MDM_PWR);
} // ngkStart

///
// power off
void ngkStop(void){
  PIOClear(MDM_PWR);
} // ngkStop

///
// send message to winch via amodem
// do not consume 'OK' response
// sets: .send[] .lastSend 
void ngkSend(MsgType msg) {
  char str[128];
  // copy msgStr and change id character // e.g. "#R,0X,00"
  strcpy(str, ngk.msgStr[msg]);
  str[4] = '0' + ngk.winchId;
  flogf("\n\t\t+ngkSend(%s) at %s", str, utlTime());
  TUTxWaitCompletion(ngk.port);
  TURxFlush(ngk.port);
  utlWrite(ngk.port, str, EOL);
  ngk.send[msg]++;
  ngk.lastSend = msg;
} // ngkSend

///
// wait for and log ngk response
MsgType ngkRecvWait(MsgType *msg, int wait) {
  tmrStart(ngk_tmr, wait);
  while (!tmrExp(ngk_tmr))
    if (ngkRecv(msg)!=null_msg)
      break;
  return *msg;
} // ngkRecvWait

///
// get winch message if available and parse it
// respond immediately to stopcmd buoycmd
// uses: ngk.expect
// sets: ngk.on ngk.expect .lastRecv 
// returns: msg
MsgType ngkRecv(MsgType *msg) {
  *msg = null_msg;
  if (!ngkRead(utlBuf)) 
    return *msg;
  *msg = msgParse(utlBuf);
  // amodem will repeat message if not OK
  if (*msg!=mangled_msg) 
    utlWrite(ngk.port, "OK", EOL);
  flogf("\n\t\t+ngkRecv(%s)", utlBuf);
  flogf(" %s %s", ngk.msgName[*msg], utlTime());
  if (*msg==buoyCmd_msg) {     // async, invisible
    ngkBuoyRsp();
    *msg = null_msg;
  }
  return *msg;
} // ngkRecv

///
// read char-fully. may have multiple msg, we want one. garbage.
// look for %# if we get that, read 7 more bytes with timeout
bool ngkRead(char *str) {
  int i;
  char c;
  str[0]=0;
  // look for message start in input queue
  while (true) {
    if (TURxQueuedCount(ngk.port)==0) return false;
    c = TURxGetByte(ngk.port, false);
    if (c=='%' || c=='#') break;
  }
  str[0] = c;
  for (i=1; i<8; i++) 
    str[i] = TURxGetByteWithTimeout(ngk.port, (short)CHAR_DELAY * 2);
  str[8]=0;
  return true;
} // ngkRead

///
// match against ngk.msgStr[]
// sets: (*msgP) ngk.recv[]
// returns: msgtype
MsgType msgParse(char *str) {
  MsgType m;
  // str must be 8 chars
  if (strlen(str)!=8)
    return mangled_msg;
  // match 
  for (m=null_msg+1; m<mangled_msg; m++)
    if (strstr(str, ngk.msgStr[m])!=NULL) 
      break;
  ngk.recv[m]++;
  if (m==mangled_msg)           // no match or invalid
    utlErr(ngk_err, str);
  return m;
} // msgParse

char * ngkMsgName(MsgType msg) {
  return ngk.msgName[msg];
} // ngkMsgName

///
// buoyRsp buoyRsp, fall and shutdown ??
void ngkBuoyRsp(void) {
  ngkSend(buoyRsp_msg);
  utlDelay(20);
  // ngkSend(fallCmd_msg);
  // BIOSReset();
} // ngkBuoyRsp

///
void ngkFlush(void) { 
  while (ngkRead(utlStr)) 
    flogf("\nngkFlush() \t| flushed out %s", 
      ngkMsgName( msgParse(utlStr) ));
  TURxFlush(ngk.port);
  TUTxFlush(ngk.port);
} // ngkFlush
