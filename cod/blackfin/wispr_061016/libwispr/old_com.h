/*
 *  com.h
 */
#ifndef _COM_H
#define _COM_H

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>

#include "rtc.h"

/* modes */
#define COM_GAIN 0x01
//#define COM_GAIN1 0x02
#define COM_GPS 0x04
#define COM_DETECT 0x08
#define COM_TIME 0x10

#define COM_PARKED 0x80
#define COM_STAT 0x100
#define COM_SURFACE_DETECT 0x200

/* states */
#define COM_EXIT 0
#define COM_RUN 1
#define COM_PAUSE 2
#define COM_RESET 3
#define COM_SLEEP 4

#define COM_MESSAGE_SIZE 128

#define COM_MESSAGE_PREFIX '$'
#define COM_MESSAGE_SUFFIX '*'

// gps data type
struct gps_struct {
   float lat;
   float lon;
   float depth;
   time_t time; // msg time
};
typedef struct gps_struct gps_t;

struct com_struct {
   int mode;
   int state;
   time_t time; // msg time
   char file[64];  // log file to save messages
   gps_t gps;
   int detx;
   int gain;
};

typedef struct com_struct com_t;

//---------------------------------------------------------
/* com message functions */
extern int com_open(char *device, speed_t baud);
extern int com_close();
extern void com_reset ();
extern int com_read_msg(char *buf, int n);
extern int com_write_msg(char *buf, int n);
extern int com_write_raw(char *buf, int n);
extern int com_parse_msg(char *buf, int len, com_t *com);
extern int com_send_dtx(int max_ndtx);
extern void *com_handle_msg(void *arg);
extern void com_reset_msg();


#endif /* _COM_H */
