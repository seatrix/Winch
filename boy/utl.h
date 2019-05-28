// utl.h - utility and shared stuff
#ifndef H_UTL
#define H_UTL

#define BUFSZ 4096

// allow up to .05 second between chars, normally chars take .001-.016
#define CHAR_DELAY 50
#define utlDelay(x) RTCDelayMicroSeconds((long)(x)*1000L);
#define SETTLE 200

// faux exception processing: Exc(10);  -->  print # and char *rets, return
#define Exc(X_VALUE) { all.exc=X_VALUE; goto X_LABEL; }
#define Except X_LABEL: flogf("\n%s() exception=%d, %s", self, all.exc, rets);

typedef TUPort * Serial;

// sync with utlInit()
typedef enum {
    ant_err, boy_err, cfg_err, s16_err, 
    gps_err, ngk_err, rud_err, wsp_err, 
    phase_err, log_err, 
    sizeof_err} ErrType;

typedef struct UtlInfo {
  bool ignoreCon;             // ignore console input
  char *buf;
  char *errName[sizeof_err];
  char *ret;
  char *str;
  int errCnt[sizeof_err];
} UtlInfo;

typedef struct AllInfo {
  char *buf;
  char *str;
  char *ret;                // returned by some char *utlFuncs()
  int cycle;                // RiseCallFallData cycles
  int exc;                  // exception value
  int starts;               // number of starts (VEEPROM)
  time_t startCycle;        // cycle start
  time_t startProg;         // program start
} AllInfo;

// the globals below are used by all modules // malloc'd in utlInit()
extern AllInfo all;

char *utlDate(void);
char *utlDateTime(void);
char *utlDateTimeS16(void);
char *utlDateTimeFmt(time_t secs);
char *utlExpect(Serial port, char *buf, char *expect, int wait);
char *utlNonPrint (char *in);        // format unprintable string
char *utlNonPrintBlock (char *in, int len);
char *utlTime(void);
int utlLogOpen(int *log, char *base);
int utlLogClose(int *fd);
int utlMatchAfter(char *out, char *str, char *sub, char *set);
int utlTrim(char *str);
int utlRead(Serial port, char *in);
int utlReadWait(Serial port, char *in, int wait);
void utlCloseErr(char *str);
void utlErr( ErrType err, char *str);
void utlInit(void);
void utlLogPathName(char *path, char *base, int day);
void utlLogTime(void);
void utlNap(int sec);
void utlPet(void);
void utlSleep(void);
void utlStop(char *out);
void utlTestLoop(void);
void utlX(void);
void utlWrite(Serial port, char *out, char *eol);
void utlWriteBlock(Serial port, char *out, int len);

// void utlDelay(int milli);
#endif
