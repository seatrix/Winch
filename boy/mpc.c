// mpc.c - hardware, mpc specific; pam x 4
#include <main.h>

#define WTMODE nsStdSmallBusAdj // choose: nsMotoSpecAdj or nsStdSmallBusAdj
#define SYSCLK 16000 // Clock speed: 2000 works 160-32000 kHz Default: 16000
#define PAM_BAUD 9600

MpcInfo mpc;

//
// Interrupts:
//
// IRQ2 Wakes up from the sleep mode while waiting for ASC string from COM2
// IRQ3
// IRQ5 Interrupt to stop the program
//


// Enable watch dog  HM 3/6/2014
short CustomSYPCR = WDT105s | HaltMonEnable | BusMonEnable | BMT32;
// #define CUSTOM_SYPCR 

//
// Set IO pins, set SYSCLK
// ?? walk thru to verify all actions
//
void mpcInit(void) {
  ushort nsRAM, nsFlash, nsCF;
  short waitsFlash, waitsRAM, waitsCF, nsBusAdj;
  short rx, tx;
  int i;
  uchar iopins[] = {27, 28, 31, 32, 33, 34, 35, 48, 50, 0};
  uchar systempins[] = {15, 16, 17, 18, 19, 20, 0};
  uchar outputpins[] = {1, 19, 21, 22, 23, 24, 25, 26, 29, 30, 37, 42, 0};
  flogf("\nset outputs low");
  for (i=0; outputpins[i]!=0; i++) {
    PIOClear(outputpins[i]);
    flogf(" %d", outputpins[i]);
  }
  // PIOMirrorList(outputpins);
  // setup pam port, shared by wispr and science s16 sbe16
  rx = TPUChanFromPin(PAM_RX);
  tx = TPUChanFromPin(PAM_TX);
  mpc.pamPort = TUOpen(rx, tx, PAM_BAUD, 0);
  if (mpc.pamPort==NULL)
    utlStop("mpcInit() pam open fail");
  mpcPamDev(null_pam);

  TMGSetSpeed(SYSCLK);
  CSSetSysAccessSpeeds(nsFlashStd, nsRAMStd, nsCFStd, WTMODE);
  CSGetSysAccessSpeeds(&nsFlash, &nsRAM, &nsCF, &nsBusAdj);
  CSGetSysWaits(&waitsFlash, &waitsRAM, &waitsCF); // auto-adjusted
  flogf(
      "\n%ukHz nsF:%d nsR:%d nsC:%d adj:%d WF:%-2d WR:%-2d WC:%-2d SYPCR:%02d",
      TMGGetSpeed(), nsFlash, nsRAM, nsCF, nsBusAdj, waitsFlash, waitsRAM,
      waitsCF, *(uchar *)0xFFFFFA21);
} // mpcInit

///
// turn pam on/off
void mpcPamPwr(MpcPamType pam, bool on) {
  static char *self="mpcPamPwr";
  DBGN("p:%d on:%d", pam, on);
  mpcPamDev(pam);
  if (on) mpcPamPulse(WISPR_PWR_ON);
  else mpcPamPulse(WISPR_PWR_OFF);
} // mpcPamPwr

///
// pam port shares rx/tx between com3, com4
// switch between devices on pam port, clear 
void mpcPamDev(MpcPamType pam) {
  DBG0("mpcPamDev(%d)", pam);
  if (pam==mpc.pamDev) return;
  PIOClear(PAM_12);   //29
  PIOClear(PAM_2);    //30
  PIOClear(PAM_34);   //24
  PIOClear(PAM_4);    //25
  switch (pam) {
  case wsp1_pam:
    PIOSet(PAM_12);     //29
    break;
  case wsp2_pam:
    PIOSet(PAM_12);     //29
    PIOSet(PAM_2);      //30
    break;
  case wsp3_pam:
    PIOSet(PAM_34);     //24
    break;
  case sbe16_pam:
    PIOSet(PAM_34);     //24
    PIOSet(PAM_4);      //25
    break;
  case null_pam:
    break;
  } // switch
  utlDelay(200);
  TUTxFlush(mpc.pamPort);
  TURxFlush(mpc.pamPort);
  utlPet();
  mpc.pamDev = pam;
  return;
} // mpcPam

///
void mpcPamPulse(int pin) {
  PIOSet(pin);
  // RTCDelayMicroSeconds((long)100);
  utlDelay(100);
  PIOClear(pin);
} // mpcPamPulse

///
// pam port is shared
void mpcPamPort(Serial *port) {
  *port = mpc.pamPort;
} // mpcPort

///
// capture interrupt, used to wake up
static void IRQ4_ISR(void) {
  PIORead(IRQ4RXD);     // console
  RTE();
} // IRQ4_ISR

///
static void IRQ5_ISR(void) {
  PIORead(IRQ5);        // pam ??
  RTE();
} // IRQ5_ISR

///
// spurious interrupt, ignore
static void spur_ISR(void) {
  RTE();
} // spur_ISR

///
// Sleep until keypress or wispr
void mpcSleep(void) {
  ciflush(); // flush any junk
  flogf("\nmpcSleep() at %s", utlDateTime());
  // Install the interrupt handlers that will break us out by "break signal"
  // ?? this should be a one time action, we toggle pins int|I/O
  IEVInsertAsmFunct(IRQ4_ISR, level4InterruptAutovector);
  IEVInsertAsmFunct(IRQ5_ISR, level5InterruptAutovector); // PAMPort ??
  IEVInsertAsmFunct(spur_ISR, spuriousInterrupt);

  PITSet51msPeriod(PITOff); // disable timer (drops power)
  CTMRun(false);            // turn off CTM6 module
  SCITxWaitCompletion();    // let any pending UART data finish
  EIAForceOff(true);        // turn off the RS232 driver
  QSMStop();                // shut down the QSM
  CFEnable(false);          // turn off the CompactFlash card

  // make it an interrupt pin
  PinBus(IRQ4RXD);          // console
  PinBus(IRQ5);             // wispr

  utlPet();      // another reprieve
  TMGSetSpeed(1600);
  while (PinTestIsItBus(IRQ4RXD) && PinTestIsItBus(IRQ5)) {
    // we loop here on spurious interrupt
    //*HM050613 added to reduce current when Silicon System CF card is used
    //*(ushort *)0xffffe00c=0xF000; //force CF card into Card-1 active mode

    LPStopCSE(FullStop); // we will be here until interrupted
    utlPet();
  }

  CSSetSysAccessSpeeds(nsFlashStd, nsRAMStd, nsCFStd, WTMODE);
  TMGSetSpeed(SYSCLK);

  // make it an I/O pin
  PIORead(IRQ4RXD);
  PIORead(IRQ5);

  EIAForceOff(false); // turn on the RS232 driver
  QSMRun();           // bring back the QSM
  CFEnable(true);     // turn on the CompactFlash card
  ciflush();          // discard any garbage characters
  flogf(", wakeup at %s", utlTime());
  putflush(); 
} // mpcSleep

void mpcStop(){}
