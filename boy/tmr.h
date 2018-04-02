// tmr.h interval timer
#define TMR_H

typedef enum {
  null_tmr=0,
  ant_tmr,          // sbe39
  ctd_tmr,          // ctd.pending waiting for ctd response
  deploy_tmr,       // steps during deploy phase0
  data_tmr,         // rise to surface, send data
  init_tmr,         // init components
  rudics_tmr,       // max time to spend on the phone
  winch_tmr,        // win.pending amodem command response
  wispr_tmr,        // wispr duty cycle
  sizeof_tmr
  } TmrType;

bool tmrExp(TmrType tim);
int tmrQuery(TmrType tim);
void tmrStart(TmrType tim, int secs);
void tmrStop(TmrType tim);
TmrType tmrAnyExp(void);

