// ant.c - for working with antenna module
#include <com.h>
#include <ant.h>

// on surfaced ctdPos depth gpsLong[20] gpsLat[20] 
AntennaData ant = {
  false, false, 1.5, 0.0, "123:45.6789 W", "45:67.8900 N" 
};

/*
 * start antenna, wait until ant responds
 * read depth
 * sets: .depth
 */
void antInit(void){
  DBG0("\nantInit()")

} // antInit

/*
 * AntMode(G|I|S)
 * Switch antenna module between SBE39 TD, Iridium and GPS.
 * ^A Antenna G|I * ^B Binary byte * ^C Connect I|S * ^D powerDown I|S
 */
int AntMode(char r) {
  static char ant='-', dev='-';
  char a, d;
  DBG2("\t|AntMode(%c)", r)
  DevSelect(DEVA);
  // select ant SBE16, switch ant device
  switch (r) {
    case 'G': { a='G'; d='I'; break; }
    case 'I': { a='I'; d='I'; break; }
    case 'S': { a=ant; d='S'; break; }
    default: { // bad case
      flogf("\nError DevSelect(%c): bad choice", r);
      return -1;
    }
  }
  if (d!=dev) {
    // turn on, connect
    TUTxPutByte(devicePort, 3, true);  // ^C connect device
    TUTxPutByte(devicePort, d, true);
    dev=d;
    delayms(3000); // wait 3 sec for device start
  }
  if (a!=ant) {
    TUTxPutByte(devicePort, 1, true);  // ^A antenna
    TUTxPutByte(devicePort, a, true);  // G I
    ant=a;
    delayms(1000); // wait 1 sec to settle antenna switch noise
  }
  return 0;
} //AntMode

