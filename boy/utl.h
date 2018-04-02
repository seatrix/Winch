// utl.h - utility and shared stuff
#define UTL_H

#define STRSZ 1024
#define BUFSZ 4096
#define C_DRV ('C'-'A')
#define null NULL
#define RS232_SETTLE 100
// allow up to .05 second between chars, normally chars take .001-.016
#define CHAR_DELAY 50

// PINS
#define MDM_PWR 21
#define ANT_PWR 22  // antenna module Power pin (1=ON, 0=OFF)
#define COM1SEL 23  // set = antMod cf2, clear = ctd sbe16 in buoy
#define WISPR3 24
#define WISPR4 25
#define HPSENS 26   // power for pressure sensor
#define WISPR1 29
#define WISPR2 30
#define COM1TX 31
#define COM1RX 32
#define MDM_RX 33
#define MDM_TX 35
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

// the globals below are used by all modules // malloc'd in utlInit()
extern char *utlBuf;
extern char *utlStr;
extern char *utlRet;      // returned by some char *utlFuncs()

typedef struct UtlInfo {
  char *buf;
  char *str;
} UtlInfo;

typedef TUPort * Serial;
char *utlDate(void);
char *utlDateTime(void);
char *utlDateTimeBrief(void);
char *utlNonPrint (char *in);        // format unprintable string
char *utlTime(void);
int utlLogFile(char *fname);
int utlTrim(char *str);
int utlRead(Serial port, char *in);
int utlReadWait(Serial port, char *in, int wait);
void utlDelay(int milli);
void utlInit(void);
void utlNap(int sec);
void utlPet(void);
void utlStop(char *out);
void utlWrite(Serial port, char *out, char *eol);
void utlWriteLines(Serial port, char *out, char *eol);

