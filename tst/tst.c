// ctdTst.c
#include <utl.h>
#include <ctd.h>
#include <mpc.h>
#include <sys.h>
#include <wsp.h>

extern CtdInfo ctd;

void main(void){
  sysInit();
  mpcInit();
  ngkInit();
  cprintf("");
  while (true) {
  wspStart();
  wspStop();
}
