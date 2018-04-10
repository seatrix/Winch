// utl.c - utility stuff
#include <utl.h>
#ifndef SYS_H
#include <sys.h>
#endif
#ifndef PWR_H
#include <pwr.h>
#endif

// allow up to .05 second between chars, normally chars take .001-.016
#define CHAR_DELAY 50

// the globals below are used by all modules, malloc'd in utlInit()
// utlRet is returned by some char *utlFuncs()
char *utlBuf, *utlStr, *utlRet;     

UtlInfo utl;

///
// malloc static buffers (heap is 384K, stack only 16K)
void utlInit(void) {
  utl.buf = malloc(BUFSZ);
  utl.str = malloc(BUFSZ);
  utlBuf = malloc(BUFSZ);
  utlStr = malloc(BUFSZ);
  utlRet = malloc(BUFSZ);
}

void utlDelay(int x) { 
  RTCDelayMicroSeconds((long)x*1000); 
}

///
// sets: (*line)
int utlTrim(char *line) {
  char c;
  int len = strlen(line);
  // trim off crlf at end 
  c = line[len-1]; if (c=='\r' || c=='\n') line[--len] = 0;
  c = line[len-1]; if (c=='\r' || c=='\n') line[--len] = 0;
  return len;
}

///
// put string to serial; queue, don't block, it should all buffer
// uses: utl.str
void utlWrite(Serial port, char *out, char *eol) {
  int len, delay, sent;
  strcpy(utl.str, out);
  if (eol!=NULL)
    strcat(utl.str, eol);
  len = strlen(utl.str);
  delay = CHAR_DELAY + (int)TUBlockDuration(port, (long)len);
  sent = (int)TUTxPutBlock(port, utl.str, (long)len, (short)delay);
  DBG1(">>=%d", sent)
  DBG2(">>'%s'", utlNonPrint(utl.str))
  if (len!=sent) 
    flogf("\nERR\t|utlWrite(%s) sent %d of %d", out, sent, len);
}

///
// read all the chars on the port, with a normal char delay
// char *in should be BUFSZ
// returns: len
int utlRead(Serial port, char *in) {
  int len = 0;
  if (TURxQueuedCount(port)<1) return 0;
  // len = (int) TURxGetBlock(port, in, (long)BUFSZ, (short)CHAR_DELAY);
  for (len=0; len<BUFSZ; len++) {
    in[len] = TURxGetByteWithTimeout(port, (short)CHAR_DELAY);
    if (in[len]<0) break;
  }
  in[len]=0;            // string
  DBG1("<<=%d", len)
  DBG2("<<'%s'", utlNonPrint(in))
  return len;
}

///
// delay up to wait seconds for first char, null terminate
// assumes full string arrives promptly after a delay of several seconds
// return: length
int utlReadWait(Serial port, char *in, int wait) {
  int len;
  in[0] = TURxGetByteWithTimeout(port, (short) wait*1000);
  utlPet(); // could have been a long wait
  if (in[0]<=0) {
    // timeout
    in[0]=0;
    return 0;
  } 
  // rest of input, note utlRead exits if nothing queued
  for (len=1; len<BUFSZ; len++) {
    in[len] = TURxGetByteWithTimeout(port, (short)CHAR_DELAY);
    if (in[len]<0) break;
  }
  in[len]=0;            // string
  DBG1("<<(%d)=%d", len)
  DBG2("<<'%s'", utlNonPrint(in))
  return len;
}

// ?? check out __DATE__, __TIME__

///
// HH:MM:SS now
// returns: global static char *utlRet
char *utlTime(void) {
  struct tm *tim;
  time_t secs;

  time(&secs);
  tim = localtime(&secs);
  sprintf(utlRet, "%02d:%02d:%02d",
          tim->tm_hour, tim->tm_min, tim->tm_sec);
  return utlRet;
} // utlTime

///
// Date String // MM-DD-YY 
// returns: global static char *utlRet
char *utlDate(void) {
  struct tm *tim;
  time_t secs;
  
  time(&secs);
  tim = localtime(&secs);
  sprintf(utlRet, "%02d-%02d-%02d", tim->tm_mon,
          tim->tm_mday, tim->tm_year - 100);
  return utlRet;
} // utlDate

///
// MM-DD-YY HH:MM:SS 
// returns: global static char *utlRet
char *utlDateTime(void) {
  struct tm *tim;
  time_t secs;
  time(&secs);
  tim = localtime(&secs);
  sprintf(utlRet, "%02d-%02d-%02d %02d:%02d:%02d", tim->tm_mon,
          tim->tm_mday, tim->tm_year - 100, tim->tm_hour,
          tim->tm_min, tim->tm_sec);
  return utlRet;
} // utlDateTime

///
// MMDDYYYYHHMMSS 
// returns: global static char *utlRet
char *utlDateTimeBrief(void) {
  struct tm *tim;
  time_t secs;
  time(&secs);
  tim = localtime(&secs);
  sprintf(utlRet, "%02d%02d%04d%02d%02d%02d", tim->tm_mon,
          tim->tm_mday, tim->tm_year + 1900, tim->tm_hour,
          tim->tm_min, tim->tm_sec);
  return utlRet;
} // utlDateTimeBrief

///
// format non-printable string; null terminate
// returns: global static char *utlRet
char *utlNonPrint (char *in) {
  char ch, *out = utlRet;
  int i, o;
  // walk thru input until 0 or BUFSZ
  i = o = 0;
  while (in[i] && o<BUFSZ-6) {
    ch = in[i++];
    if ((ch<32)||(ch>126)) {
      // non printing char
      sprintf(out+o, " x%02X ", ch);
      o += 5;     // five char hex ' x1A '
    } else 
      out[o++] = ch;
  }
  out[o] = 0;
  return (out);
} // printsafe

void utlPet() { TickleSWSR(); }              // pet the watchdog

///
// takes a base name and makes a full path, opens file, writes dateTime
// ?? moves existing file to backup dir
// rets: fileID
int utlLogFile(char *fname) {
  int log;
  char *path[64];
  strcpy(path, "log\\");
  strcat(path, fname);
  strcat(path, ".log");
  log = open(path, O_APPEND | O_CREAT | O_RDWR);
  if (log<=0) {
    sprintf(utl.str, "FATAL | utlLogFile(%s): open failed", fname);
    utlErr(file_err, utl.str);
    return 0;
  } else {
    sprintf(utl.str, "\n---  %s ---\n", utlDateTime());
    write(log, utl.str, strlen(utl.str));
    return log;
  }
} // utlLogFile

///
// ?? tbd sophist err handling, allow limit by type
void utlErr( ErrType err, char *str) {
  utl.err[err]++;
  flogf(str);
}

///
// nap called often
void utlNap(int sec) {
  pwrNap(sec);
} // utlNap

///
// stop called often
void utlStop(char *out) {
  sysStop(out);
} // utlStop
