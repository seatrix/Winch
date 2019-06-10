// iridFile.c
#include <main.h>

extern IriInfo iri;
extern IriData irid;
extern BoyInfo boy;
extern SysInfo sys;

void main(void){
  // Serial port;
  // char c;
  char *buff;
  int len, cnt;
  int i, r;
  i=0;
  sysInit();
  mpcInit();
  antInit();
  iriInit();
  //
  antStart();
  iriStart();
  //
  len = dbg.t2;
  cnt = dbg.t1;
  cprintf("\nlength dbg.t2=%d, count dbg.t1=%d ", len, cnt);
  cprintf("\nbaud iri.rudBaud=%d", iri.rudBaud);
  antSwitch(irid_ant);
  if (iriSig()) return;
  if (iriDial()) return;
  if (iriProjHello(all.buf)) return;
  iriSendFile("test\\test.log");
  iriLandResp(all.buf);
  if (strstr(all.buf, "cmds"))
    r = iriLandCmds(all.buf);
  strcpy(buff, all.buf);
  utlDelay(500);
  utlWrite(irid.port, "done", "");
  utlDelay(500);
  iriHup();
  iriSig();
  flogf("\n%s\n", utlTime());
  flogf("\nsetting '%s'", utlNonPrint(buff));
  cfgString(buff);
  flogf("\nsys.program = %s", sys.program);
  // antSwitch(gps_ant);
  // iriStats();
  iriStop();
  antStop();
}
