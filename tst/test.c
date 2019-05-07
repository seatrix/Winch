// files.c
#include <utl.h>
#include <mpc.h>
#include <sys.h>

#define MAX 12
int fd[MAX];

int first();
int last();
int count();

int first() {
  int i=0;
  for (i=0; i<MAX; i++)
    if (fd[i]) return i;
  return -1;
}
    
int last() {
  int i=0;
  for (i=MAX; i>=0; i--)
    if (fd[i]) return i;
  return -1;
}

int count() {
  int i=0;
  int j=0;
  for (i=0; i<MAX; i++)
    if (fd[i]) j++;
  return j;
}


void main(void){
  char s[128], c;
  int n=0;
  sysInit();
  mpcInit();

  flogf("\n q to exit, c=count, f=first close, l=last close, o=open");
  while (true) {
    if (cgetq()) {
      c=cgetc();
      flogf("%c \n", c);
      if (c=='Q') break;
      if (c=='q') break;
      if (c=='c') {
        flogf("%d open files\n", count());
        continue;
      }
      if (c=='f') {
        utlLogClose(&fd[first()]);
        continue;
      }
      if (c=='l') {
        utlLogClose(&fd[last()]);
        continue;
      }
      if (c=='o') {
        sprintf( s, "t1t%03d", n++ );
        utlLogOpen(&fd[last()+1], s);
        continue;
      }
    }
  }
}
