// utl.h - utility and shared stuff
#define UTL_H

#define STRSZ 128
#define BUFSZ 4096
#define C_DRV ('C'-'A')
#define null NULL
#define SETTLE 200
// allow up to .05 second between chars, normally chars take .001-.016
#define CHAR_DELAY 50

// PINS
#define MDM_PWR 21
#define ANT_PWR 22  // antenna module Power pin (1=ON, 0=OFF)
#define ANT_SEL 23  // set = antMod cf2, clear = a3la
#define PAM_34 24   // 24 cannot be on with 29
#define PAM_4 25    // 24&25 -> pam4 (sbe16)
#define HPSENS 26   // power for pressure sensor
#define PAM_TX 27   // com2,3: (pam_12)(pam_34) 
#define PAM_RX 28   // selected by 24, 29
#define PAM_12 29   // 24 cannot be on with 29
#define PAM_2 30    // 29&30 -> wsp2
#define ANT_TX 31   // com1: ant mod, uMPC and A3LA
#define ANT_RX 32   // selected by 23
#define MDM_RX 33   // acoustic modem
#define MDM_TX 35   // 
#define WISPR_PWR_ON 37
#define WISPR_PWR_OFF 42
#define MDM_RX_TTL 48
#define MDM_TX_TTL 50

#include <cfxpico.h> // Persistor PicoDOS Definitions
#include <cfxad.h>
#include <dirent.h>   // PicoDOS POSIX-like Directory Access Defines
#include <dosdrive.h> // PicoDOS DOS Drive and Directory Definitions
#include <fcntl.h> // PicoDOS POSIX-like File Access Definitions
#include <stat.h> // PicoDOS POSIX-like File Status Definitions
#include <termios.h> // PicoDOS POSIX-like Terminal I/O Definitions
#include <unistd.h> // PicoDOS POSIX-like UNIX Function Definitions
// std c
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dbg.h>

// sync with utlInit()
typedef enum {
    ant_err, boy_err, cfg_err, ctd_err, gps_err, ngk_err, rud_err, wsp_err, 
    log_err, 
    sizeof_err} ErrType;

typedef struct EngInfo {
  char gpsStart[64];
  char gpsDrift[64];
  char lat[64];
  char lng[64];
  char program[64];
  char riseStart[64];
  char riseDone[64];
  float dockD;
  float oceanCurr;
  float surfD;
  int detect;
  int cycle;
} EngInfo;

typedef struct UtlInfo {
  bool ignoreCon;             // ignore console input
  char *buf;
  char *errName[sizeof_err];
  char *ret;
  char *str;
  int errCnt[sizeof_err];
} UtlInfo;

typedef TUPort * Serial;

// the globals below are used by all modules // malloc'd in utlInit()
extern char *utlBuf;
extern char *utlStr;
extern char *utlRet;      // returned by some char *utlFuncs()
extern EngInfo eng;

char *utlDate(void);
char *utlDateTime(void);
char *utlDateTimeBrief(void);
char *utlExpect(Serial port, char *buf, char *expect, int wait);
char *utlNonPrint (char *in);        // format unprintable string
char *utlNonPrintBlock (char *in, int len);
char *utlTime(void);
int utlLogFile(char *base);
void utlLogPathName(char *path, char *base, int day);
int utlMatchAfter(char *out, char *str, char *sub, char *set);
int utlTrim(char *str);
int utlRead(Serial port, char *in);
int utlReadWait(Serial port, char *in, int wait);
void utlDelay(int milli);
void utlErr( ErrType err, char *str);
void utlInit(void);
void utlLogTime(void);
void utlNap(int sec);
void utlPet(void);
void utlSleep(void);
void utlStop(char *out);
void utlX(void);
void utlWrite(Serial port, char *out, char *eol);
void utlWriteBlock(Serial port, char *out, int len);
