// iridFile.c
#include <utl.h>
#include <gps.h>
#include <mpc.h>
#include <ant.h>
#include <sys.h>
#include <tmr.h>
#include <boy.h>
#include <cfg.h>

extern GpsInfo gps;
extern BoyInfo boy;
extern SysInfo sys;

void main(void){
  // Serial port;
  // char c;
  char *buff;
  int l, len, cnt;
  int i, r;
  i=0;
  sysInit();
  mpcInit();
  antInit();
  gpsInit();
  //
  antStart();
  gpsStart();
  //
  len = boy.testSize;
  cnt = boy.testCnt;
  cprintf("\nlength boy.testSize=%d, count boy.testCnt=%d ", len, cnt);
  cprintf("\nbaud gps.rudBaud=%d", gps.rudBaud);
  buff = malloc(len);
  // antSwitch(gps_ant);
  // gpsStats();
  antSwitch(irid_ant);
  if (iridSig()) return;
  if (iridDial()) return;
  if (iridProjHdr()) return;
  /*
  for (i=1; i<=cnt; i++) {
    memset(buff, 0, len);
    sprintf(buff, "%d of %d =%d @%d [%d]", 
      i, cnt, len, gps.rudBaud, gps.sendSz);
    buff[len-1] = 'Z';
    r = iridSendBlock(buff, len, i, cnt);
    cprintf("(%d)\n", r);
    // utlDelay(500);
  }
   */
  iridSendFile("test\\test.log");
  iridLandResp(utlBuf);
  if (strstr(utlBuf, "cmds"))
    r = iridLandCmds(utlBuf, &l);
  utlBuf[l] = 0;
  strcpy(buff, utlBuf);
  utlDelay(500);
  utlWrite(gps.port, "done", "");
  utlDelay(500);
  iridHup();
  iridSig();
  flogf("\n%s\n", utlTime());
  flogf("\nsetting '%s'", utlNonPrint(buff));
  cfgString(buff);
  flogf("\nsys.program = %s", sys.program);
  // antSwitch(gps_ant);
  // gpsStats();
  gpsStop();
  antStop();
}
