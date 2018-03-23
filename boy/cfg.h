// cfg.h
#define SYS_CFG "SYS.CFG"

typedef struct CfgInfo {
  char file[32];
  char wild[32];
  int len;
} CfgInfo;
static void cfgSet( void *ptr, char type, char *val );
static void cfgVee(void);
static bool cfgCmp(char *a, char*b);

bool cfgString(char *str);
int cfgRead(char *file);
void cfgDump(void);
void cfgInit(void);
