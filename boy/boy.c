// boy.c
// Acoustic Real-Time Sensor ARTS, aka LARA

#include <com.h>
#include <boy.h> 

#include <ant.h> 
#include <ctd.h>
#include <mpc.h>
#include <ngk.h>
#include <pwr.h>
#include <sys.h>
#include <wsp.h>

BuoyInfo boy;

//
// deploy or reboot, then loop over phases data/rise/call/drop
//
void boyMain(int starts) {
  boy.phase = boy.startPhase;
  if (starts>1) 
    boyReboot();
  flogf("\nboyMain(): starting with phase %d", boy.phase);
    
  while (true) {
    boy.phaseT=time(0);
    switch (boy.phase) {
    case deploy_pha:
      deployPhase();
      break;
    case data_pha: // data collect by WISPR
      dataPhase();
      break;
    case rise_pha: // Ascend buoy, check for current and ice
      risePhase();
      break;
    case call_pha: // Call home via Satellite
      callPhase();
      break;
    case drop_pha: // Descend buoy, science sampling
      dropPhase();
      break;
    } // switch
  } // while true
} // boyMain() 

//
// 
//
void boyInit() {
} // boyInit

//
// figure out whats happening, continue as possible
// load info from saved previous phase
// ask antmod for our velocity
// sets: boy.phase
//
void boyReboot() {
} // reboot()

//
// wispr recording and detecting, buoy is docked to ngk
// data is gathered for about 24hours (data_tmr)
// wsp powers down for % of each hour (wispr_tmr)
// sets: boy.phaseStartT 
// uses: data_tmr duty_tmr
//
void dataPhase(void) {
  flogf("\n\t|dataPhase()");
  // sleep needs a lot of optimizing to be worth the trouble
  // Sleep();
  wspStop();
} // dataPhase

//
// collect and organize data files.
// turn on ant, free space check, transfer files from buoy to antmod
// ascend. check angle due to current, up midway, re-check angle, surface.
// sets: boy.alarm[]
//
void risePhase(void) {
  flogf("\n\t|risePhase()");
  float depth, depthStart, sideways, targetD, velocity;
  time_t riseT;
  MsgType rsp;
  //
  dataFiles();
  antInit();
  depthStart = depth = antDepth();
  // if current is too strong
  if (oceanCurrChk()) {
    sysAlarm(bottomCurr_alm);
    boy.phase = data_pha;
    return;
  }
  targetD = boy.currCheckD;
  riseT = time(0);
  ngkSend( riseCmd_msg );
  while ((depth = antDepth()) > targetD) {
    // start rise (or retry if ngk timeout)
    // ngk: "going up" or "stopped"
    ngkRecv(&rsp);
    switch (rsp) {
    case null_msg: break;
    case riseRsp_msg: // rise ack
      tmrStop(ngk_tmr);
      // start velocity measure
      riseT = time(0);
      depthStart = antDepth();
      break;
    case dropRsp_msg: // unexpected 
      flogf("\nERR\t|risePhase() ngk unexpected drop at %03.1f m", depth);
      boy.phase = drop_pha;      // go down, try again tomorrow
      return;
    case stopRsp_msg: // unexpected 
      flogf("\nERR\t|risePhase(): ngk unexpected stop at %03.1f m", depth);
      boy.phase = drop_pha;      // go down, try again tomorrow
      return;
    case timeRsp_msg: // timeout
      sysAlarm(ngkTimeout_alm);
      amodem.timeout[riseCmd_msg] += 1;
      if (depthStart-depth < 3) {
        // not rising from dock. log, reset, retry 5 times or abort
        if (retry++ < 5) { // retry
          flogf("\n\t|risePhase() timeout on ngk, retry rise cmd %d", retry); 
          riseT = time(0);
          ngkSend( riseCmd_msg );
        } else { // abort
          flogf("\nERR\t|risePhase() timeout on ngk, %d times, abort", retry); 
          boy.phase = drop_pha;
          return;
        }
      } else { // depth 
        // odd, we are rising; log but ignore
        flogf("\n\t|risePhase() timeout on ngk, but rising so continue..."); 
      } // depth
      break;
    } // switch
  } // while (depth>midway)
  // algor: midway. figure velocity, stop
  ngk.lastRiseV = (depthStart-depth) / (time(0)-riseT);
  if (ngk.firstRiseV==0)
    ngk.firstRiseV = ngk.lastRiseV;
  ngkSend( stopCmd_msg );
  // algor: current check. rise to surface, checking response
  if (oceanCurrChk()) {
    sysAlarm(midwayCurr_alm);
    boy.phase = drop_pha;
    return;
  }
  // 
  // go to surface. same loop but stop cmd expected
  // 
  while (!antSurf()) {
    switch (ngkRecv(&r)) {
    case null_msg: break;
    case riseRsp_msg: // rise ack
      tmrStop(ngk_tmr);
      // start velocity measure
      riseT = time(0);
      depthStart = antDepth();
      break;
    case dropRsp_msg: // unexpected
      flogf("\nERR\t|risePhase() ngk unexpected drop at %03.1f m", antDepth());
      boy.phase = drop_pha;      // go down, try again tomorrow
      return;
    case stopRsp_msg: // slack auto-stop
      continue;
    case timeRsp_msg: // timeout
      sysAlarm(ngkTimeout_alm);
      amodem.timeout[riseCmd_msg] += 1;
      if (depthStart-antDepth() < 3) {
        // not rising from dock. log, reset, retry 5 times or abort
        if (retry++ < 5) { // retry
          flogf("\n\t|risePhase() timeout on ngk, retry rise cmd %d", retry);
          riseT = time(0);
          ngkSend( riseCmd_msg );
        } else { // abort
          flogf("\nERR\t|risePhase() timeout on ngk, %d times, abort", retry);
          boy.phase = drop_pha;
          return;
        }
      } else { // depth
        // odd, we are rising; log but ignore
        flogf("\n\t|risePhase() timeout on ngk, but rising so continue...");
      } // depth
      break;
    } // switch
  } // while depth>surfaceD
  //
  // ant mod surfaced, floats may be below still
  // antSurfOp() expects stop within some secs
  // start warming gps, we don't need depth until dropP
  // were files transfered at end of dataPhase ??
} // risePhase

//
// turn off sbe, on irid/gps (takes 30 sec). 
// read gps date, loc. 
//
void callPhase(void) {
} // callPhase

//
//
void dropPhase(void) {
} // dropPhase


  /****
//
// phase Three
// Testing iridium/gps connection. 
// If failed, release ngk cable another meter or two.
// repeat to minimum CTD depth.
//
void phase3(void) {
  // global: static char uploadfile[] = "c:00000000.dat
  // global ulong PwrOff PwrOn
  short result = 0;
  int gpsFails = 0;
  short count = 0;
  static short IridCallsNoParams = 0;
  char filenum[9] = "00000000";
  flogf("\n\t|phase THREE");
  boy.phaseStartT=time(0);

  if (WISPR_Status()) {
    WISPRSafeShutdown();
  }
  wisprInit(false);

  // should do this at boot
  if (boy.RESTART) { 
    ParseStartupParams(true); 
  } 

  // ?? why?
  Time(&PwrOff); 
  PwrOff -= PwrOn;
  // VEEPROM: SystemParameters MPC;
  sprintf(&uploadfile[2], "%08ld.dat", MPC.FILENUM);
  cprintf("\n\t|File Number: %08ld", MPC.FILENUM);
  // writefile 1) MPC 2) Ngk Info 3) Ngk Status
  // v
  WriteFile(PwrOff);
  // Init New LogFile, set PwrOn which is start of dataxint cycle
  Time(&PwrOn);


  while (result <= 0) { 
    // -1=false gps, -2=false irid, 1=success 2=fake cmds 3=real cmds
    // DBG( Incoming_Data();)
    result = IRIDGPS(); 

    if (result >= 1 || gpsFails > 4) {
      // IRIDIUM Successful success/fake/real/5th, next phase
      boy.phase = 4;
    }
    if (result == 1 || result == 2) { 
      // Upload Success / Commands
      IridCallsNoParams++; // call sessions w/o Params
      flogf("\n\t|Successful IRID Call: %d", IridCallsNoParams);
      if (IridCallsNoParams > 3) {
        // Load default parameters in "default.cfg" file
        ParseStartupParams(true); 
      } // calls>3
    } // Upload Success / Commands
    else if (result == 3) { 
      // Real Commands
      ParseStartupParams(false);
      IridCallsNoParams = 0;
      flogf("\n\t|Successful IRID Call: %d", IridCallsNoParams);
    } // Real Commands
    else if (result == -2) {
      // GPS Success, IRID Fail
      flogf("\n\t|phase3(): Failed Iridium Transfer");
      IridCallsNoParams++;
      if (IridCallsNoParams > 3) {
        IridCallsNoParams = 0;
        ParseStartupParams(true);
      } // calls>3
      boy.phase = 4;
      break;
    } // GPS Success, IRID Fail
    else if (result == -1) {
      gpsFails++;
      // Bad GPS- GPS fails usually from bad reception.
      flogf("\n\t|phase3(): Failed GPS attempt: %d", gpsFails);
      if (gpsFails >= 5) {
        flogf("\n\t|Exiting phase3()");
        // ?? close ports?
        break;
      } // >=5
      // removed section, go up further
    } // Bad GPS- GPS fails usually from bad reception
  } // while result<=0

  // in recovery, stay on surface 
  if (NIGK.RECOVERY) boy.phase = 1; 
  // NIGK.RECOVERY may be cleared by Params load 
  
  boy.RESTART = false;
  MPC.FILENUM++;
  sprintf(filenum, "%08ld", MPC.FILENUM);
  VEEStoreStr(FILENUM_NAME, filenum);
  create_dtx_file(MPC.FILENUM);
  CTD_CreateFile(MPC.FILENUM); 
  boy.TDEPTH = NIGK.TDEPTH;

  if (WISP.DUTYCYCL > 50) {
    wisprInit(true);
    WISPRPower(true);
  }

  boy.DATA = false;

} // phase3 //
//
// phase4
//
void phase4(void) {

  float depthChange = 0.0;
  float velocity, descentvelocity;
  ulong timeChange = 0, DescentStart, DescentStop;
  ulong timecheck = 0;
  static float prevDepth = 0;
  int interval = 1;

  flogf("\n%s|phase_Four():", Time(NULL));
  boy.phaseStartT=time(0);
  amodemInit(true);

  boy.SURFACED = false;

  PrintSystemStatus();
  // sanity check
  CTD_AverageDepth(9, &velocity);
  if (boy.BUOYMODE != 0) {
    Ngk_Stop();
    WaitForWinch(0);
    flogf("\nErr phase4(): buoy was in motion");
  }
  //
  // turn off antenna, which selects buoy ctd
  DevSelect(DEVX);
  // redundant?
  CTD_Select(DEVB);

  // Now descend.
  if (boy.BUOYMODE != 2) {
    boy.TOPDEPTH = boy.depth;
    DescentStart = Ngk_Descend();
    WaitForWinch(2);
    CTD_Sample();
  } else
    CTD_Sample();

  ADD IN CHECK HERE. if WaitForWinch returns false because of timeout and lack
  of AModem Response: then check CTD Average depth and make sure Velocity>0.25
  (descending.)
  If not, then call Winc_Descend Again. After 10, wait for an hour
//

  prevDepth = boy.depth;

  while (boy.BUOYMODE == 2) {
    // reading a sample triggers a new one, in p4
    Incoming_Data();

    // Receive Stop command from Ngk...
    if (boy.BUOYMODE == 0)
      DescentStop = (time(NULL) - (ulong)(NIGK.DELAY));

    timecheck = time(NULL);
    // Check depth change every 60 seconds... This is an out of the while loop.
    // Hopefully it will only come here when AModem stops boy
    // doesn't hear the Ngk serial coming.
    if (timecheck - DescentStart > interval * 180) { // ?? known prob w ctd read
      flogf("\n\t|phase4() Check depth change");
      prevDepth -= boy.depth;
      if (prevDepth < 0)
        prevDepth = prevDepth * -1.0;
      if (prevDepth < 3) {
        flogf("\nERROR|Depth change less than 3 meters... Ngk hasn't "
              "responded STOP");
        DescentStop = Ngk_Stop();
        WaitForWinch(0);
      }
      prevDepth = boy.depth;
      interval++;
    }
  } // while mode==2

  if (boy.BUOYMODE == 0)
    boy.dockDepth = boy.depth;

  // Descent Velocity;
  descentvelocity = CTD_CalculateVelocity();
  flogf("\n\t|Calculated Descent Velocity: %.2fm/s", descentvelocity);

  // Total Vertical depth change. total time change, calculate estimated
  // velocity.
  depthChange = boy.dockDepth - boy.TOPDEPTH;
  boy.DESCENTTIME = (short)(DescentStop - DescentStart);
  flogf("\n\t|Rate of Descent: %fMeters/Minute",
        (depthChange / ((float)boy.DESCENTTIME / 60.0)));
  flogf("\n\t|User calculated Cable Payout: %fMeters",
        ((float)boy.DESCENTTIME / 60.0) * NIGK.FRATE);
  flogf("\n\t|Time for Descent: %d", boy.DESCENTTIME);
  PrintSystemStatus();
  powDelay(2);
  amodemInit(false);

  boy.phase = 1;
  boy.DATA = false;
} // phase_Four

//
// phase0 deploy buoy sequence
// on ship, sinking, at bottom wait boy.settleTime, gather info, phase2
// set: boy.dockDepth
//
void phase0(void) {
  DBG0("phase0()")
  // global ctd.depth, boy.runStart
  short checkDepth=30; 
  time_t deployMax=boy.runStart + (2*60*60);

  flogf("\n%s\t|deploy(): wait until >10m", Time(NULL))
  while (boy.depth<10.0) {
    ctdSample();
    while (!incoming());         // wait for input
    DBG2(" P0 %.1f ", ctd.depth)
    powDelay(checkDepth);
    // too long? give up, shut down - assume accidental start
    if (time(0) > (deployMax))
      shutdown("\nERR P0() exceeded max deploy time");
  }
  // checkDepth=30 secs; if no change for settleTime=120 secs, then deployed
  float prevDepth=0.0, bigChange=1.0;
  short settle=120, noChange=0;
  while (noChange < settle)
    powDelay(checkDepth);
    ctdSample();
    if (CTD_Data()) nowD=boy.depth;
    else  flogf("\nERR in Phase0() - no CTD data");
    if (abs(prevDepth-boy.depth) > big) { // changed
      flogf("\n%s\t|Phase0(): depth change %4.1f", Time(NULL), (nowD-thenD));
      changeless=0;
    } else changeless++;
    if (time(0) > (deployMax))
      shutdown("\nERR P0() exceeded max deploy time");
    // no big change for checkDepth secs
    prevDepth = ctd.depth;
    noChange += checkDepth;
  }
  boy.dockDepth = ctd.depth;
  // do nothing for 3 minutes
  powDelay(3*60);
  flogf("\n%s\t|P0: deployed", Time(NULL));
  // rise
  boy.phase=2;
} // phase0()

//
// int Incoming_Data()
// ?? very fragile, caution
// called by phase1,2,3,4
//
int Incoming_Data(void) {
  bool incoming = true;
  static int count = 0;
  int value = 0; // Need to update this if we ever need to return a legit value.

  DBG0("Incoming_Data\t")
  switch (boy.phase) {
  // Case 0: Only at startup when MPC.STARTUPS>0
  case 0:
    while (incoming) {
      AD_Check();
      if (tgetq(NIGKPort)) {
        AModem_Data();
      } else if (tgetq(devicePort)) { // ?? very messy handling of ctd
        // DBG1("CTD Incoming")
        CTD_Data();
        if ((!boy.SURFACED && boy.phase > 1) || boy.BUOYMODE > 0 ||
            boy.phase == 0) // if not surfaced (targetD depth not reached.) and
                             // ngk is moving (not stopped)
          CTD_Sample();
      } else if (cgetq()) {
        // DBG1("Console Incoming")
        Console(cgetc());
      } else
        incoming = false;
    }
    break;

  // CASE 1: MOORED AUH
  case 1:

    while (incoming) {
      DBG1("Incoming\t")
      AD_Check();
      // Data coming from WISPR Board
      if (tgetq(PAMPort)) {
        // DBG1("WISPR Incoming")
        WISPR_Data();
      } else if (tgetq(devicePort)) {
        CTD_Data();
      }
      // Console Wake up.
      else if (cgetq()) {
        // DBG1("Console Incoming")
        Console(cgetc());
      }
      // No more incoming data
      else
        incoming = false;
    }
    break;

  // CASE 2&4: WINCH IN ACTION. Incorporate AMODEM in here
  case 2:
  case 4:

    while (incoming) {
      DBG1("Incoming\t")
      // ?? does adcheck need to run between each incoming? how often?
      AD_Check();
      if (tgetq(PAMPort)) {
        DBG1("WISPR Incoming")
        WISPR_Data();
      } else if (tgetq(NIGKPort)) {
        // ??? not hearing on bench test
        DBG1("NIGK Incoming")
        AModem_Data();
      } else if (tgetq(devicePort)) {
        DBG1("CTD Incoming")
        CTD_Data();
        if ((!boy.SURFACED && (boy.phase == 2 || boy.phase == 4)) ||
            boy.BUOYMODE > 0) // if not surfaced (targetD depth not reached.)
                               // and ngk is moving (not stopped)
          CTD_Sample();
      } else if (cgetq()) {
        DBG1("Console Incoming")
        Console(cgetc());
      }
      // No more incoming data
      else
        incoming = false;
    }
    break;

  // CASE 3: GPSIRID
  case 3:
    while (incoming) {
      DBG1("Incoming\t")
      AD_Check();
      if (tgetq(PAMPort)) {
        // DBG1("WISPR Incoming")
        WISPR_Data();
      } else if (cgetq()) {
        // DBG1("Console Incoming")
        Console(cgetc());
      } else
        incoming = false;
    }
    break;

  default:
    // Lost ngk phase. Get boy System Status decided which phase it best
    // belongs.

    cprintf("default switch");
    break;
  }

  PutInSleepMode = true;

  return value;

} // Incoming_Data
//
// void Console
// Platform Specific Console Communication
//
void Console(char in) {
  // are there side effects from any subroutines?
  // shutdown from here
  short c;

  DBG1("Incoming Char: %c", in)
  delayms(2);
  switch (boy.phase) {
  case 1:
    switch (in) {
    case 'I':
    case 'i':
      WISPRPower(true);
      break;

    case 'E':
    case 'e':
      WISPRSafeShutdown();
      break;

    case 'D':
    case 'd':
      flogf("\n\t|REQUEST # DTX FROM WISPR?");
      c = cgetc() - 48;
      WISPRDet(c);
      break;

    case 'F':
    case 'f':
      WISPRDFP();
      break;

    case 'W':
    case 'w':
      flogf("\n\t|CHANGE TO WISPR #?");
      c = cgetc() - 48;
      ChangeWISPR(c);
      break;

    case 'P':
    case 'p':
      flogf("\n\t|CHANGE OF phase #?");
      c = cgetc() - 48;
      flogf(" phase: %d", c);
      boy.phase = c;
      break;

    case 'x':
      // boy.ON = false;
      // boy.DATA = boy.DATA ? false : true;
      shutdown();
      break;
    case '2':
      boy.DATA = boy.DATA ? false : true;
      break;
    }
    break;

  case 2:
  case 4:
    switch (in) {
    case 'W':
    case 'w':
      NgkConsole();
      break;
    case 'P':
    case 'p':
      flogf("\n\t|CHANGE OF phase #?");
      c = cgetc() - 48;
      boy.phase = c;
      break;
    case 'T':
    case 't':
      flogf("\n\t|Take CTD Sample");
      CTD_Sample();
      break;
    case 'x':
      // boy.ON = false;
      // boy.DATA = boy.DATA ? false : true;
      shutdown();
      break;
    case 'a':
    case 'A':
      PrintSystemStatus();
      break;
    case 'S':
    case 's':
      boy.SURFACED = true;
      break;
    }
    break;

  case 3:
    switch (in) {
    case 'x':
      // boy.ON = false; // exit from GPSIRID
      shutdown();
      break;
    case 'P':
    case 'p':
      flogf("\n\t|Change of phase #?");
      c = cgetc() - 48;
      boy.phase = c;
      break;
    }
    break;
  }

  // ?? PutInSleepMode = true;
  return;
}

//
// ExtFinishPulseRuptHandler		IRQ5 logging stop request interrupt
//
IEV_C_FUNCT(ExtFinishPulseRuptHandler) {
#pragma unused(ievstack) // implied (IEVStack *ievstack:a0) parameter

  PinIO(IRQ5);

  PinRead(IRQ5);

} // ExtFinishPulseRuptHandler

//
// !! this routine is run for every PIT interrupt!
//* plus other stuff in the loop that calls Sleep(), in phase1()
// Usually sleeps until power interrupt PIT (20Hz), 
// but can also break on serial ints irq4 (cons), irq5 (pam), or spurious
// note, spurious could occur on other pins 
//
void Sleep(void) {
  // these handlers just set the pin to I/O mode; wakeup, destroys data
  IEVInsertAsmFunct(IRQ4_ISR, level4InterruptAutovector); // Console Interrupt
  IEVInsertAsmFunct(IRQ4_ISR, spuriousInterrupt);
  IEVInsertAsmFunct(IRQ5_ISR, level5InterruptAutovector); // PAMPort Interrupt
  // IEVInsertAsmFunct(IRQ5_ISR, spuriousInterrupt);
  //
  SCITxWaitCompletion();    // clear serial buffers
  EIAForceOff(true);        // turn off rs232 transmitters
  CFEnable(false);          // disable CF card
  //
  PinBus(IRQ4RXD); // Console Interrupt
  while ((PinTestIsItBus(IRQ4RXD)) == 0)
    PinBus(IRQ4RXD);
  PinBus(IRQ5); // PAMPort Interrupt
  while ((PinTestIsItBus(IRQ5)) == 0)
    PinBus(IRQ5);
  //
  while (PinRead(IRQ5) && PinRead(IRQ4RXD) && !data)
    LPStopCSE(FastStop); // we will be here until interrupted
  //
  EIAForceOff(false); // turn on the RS232 driver
  CFEnable(true);     // turn on the CompactFlash card
  //
  PIORead(IRQ4RXD);
  while ((PinTestIsItBus(IRQ4RXD)) != 0)
    PIORead(IRQ4RXD);
  PIORead(IRQ5);
  while ((PinTestIsItBus(IRQ5)) != 0)
    PIORead(IRQ5);
  //
  PutInSleepMode = false;
  //
  // DBG2(".")
  delayms(10);
} // Sleep


//
// static void Irq3ISR(void)
//
static void IRQ3_ISR(void) {
  PinIO(IRQ3RXX);
  RTE();
} // Irq3ISR //
//
// static void Irq2RxISR(void) CTD/Seaglider/ IRIDIUM
//
static void IRQ2_ISR(void) {
  PinIO(IRQ2);
  RTE();
} // Irq2RxISR //
//
// static void IRQ4_ISR(void) CONSOLE
//
static void IRQ4_ISR(void) {
  PinIO(IRQ4RXD);
  RTE();
} // Irq2ISR
//
// static void IRQ5_ISR(void) WISPR
//
static void IRQ5_ISR(void) {
  PinIO(IRQ5);
  RTE();
} // Irq5ISR
//
// WriteFile      The Data File For Lara
1) Initially upload MPC parameters
2) Ngk Info
3) Ngk Status
//
ulong WriteFile(ulong TotalSeconds) {
  // global uploadfile, WriteBuffer
  long BlkLength = BUFSZ;
  int filehandle;
  struct stat info;
  char detfname[] = "c:00000000.dtx";
  char logfile[sizeof "00000000.log"];
  int byteswritten = 0;
  int bytesWritten = 0;
  long maxupload;
  ulong LoggingTime;

  DBG1("\t|WriteFile(%s)", uploadfile)
  filehandle = open(uploadfile, O_WRONLY | O_CREAT | O_TRUNC);

  if (filehandle <= 0) {
    flogf("\nERROR  |WriteFile(): open error: errno: %d", errno);
    return -1;
  }
  DBG(else flogf("\n\t|WriteFile: %s Opened", uploadfile);)

  //*** boy Write ***//
  // ?? should this have geo loc?
  sprintf(WriteBuffer, "boy Program Ver:%.1f\naa:bb.cccc North ddd:ee.ffff "
                       "West\nFileNumber: %ld\nStarts:%d of "
                       "%d\nWriteTime:%s\nSeconds:%lu\nDetection "
                       "Interval:%3d\nData Inteval: %d\n\0",
          PROG_VERSION, MPC.FILENUM, MPC.STARTUPS, MPC.STARTMAX, Time(NULL),
          TotalSeconds, MPC.DETINT, MPC.DATAXINT);

  flogf("\n%s", WriteBuffer);
  bytesWritten = write(filehandle, WriteBuffer, strlen(WriteBuffer));
  DBG2("BytesWritten: %d", bytesWritten)

  // Only comes here if not rebooted.
  if (TotalSeconds != 0) {
    //*** Ngk Info   ***//
    // blk 
    Ngk_Monitor(filehandle);
    delayms(50);
    memset(WriteBuffer, 0, BUFSZ);

    //*** Ngk Status ***//
    sprintf(WriteBuffer, "%s\n\0", PrintSystemStatus());
    bytesWritten = write(filehandle, WriteBuffer, strlen(WriteBuffer));
    DBG2("BytesWritten: %d", bytesWritten)
  }
  // Else, coming from reboot. Name the PowerLogging File.
  else {
    ADSFileName(MPC.FILENUM);
    delayms(50);
  }

  //*** Power Monitoring Upload ***//
  Power_Monitor(TotalSeconds, filehandle, &LoggingTime);
  delayms(50);
  Setup_ADS(true, MPC.FILENUM + 1, BITSHIFT);
  DOS_Com("del", MPC.FILENUM, "PWR", NULL);
  delayms(500);

  //*** WISPR File Upload ***//
  WISPRWriteFile(filehandle);
  delayms(50);

//*** CTD File Upload ***
#ifdef CTDSENSOR
  if (ctd.UPLOAD || TotalSeconds == 0) {
    sprintf(&detfname[2], "%08ld.ctd", MPC.FILENUM);
    delayms(50);
    DBG1("\t|WriteFile:%ld ctd file: %s", MPC.FILENUM, detfname)
    stat(detfname, &info);
    if (info.st_size > (long)(IRID.MAXUPL - 1000))
      maxupload = IRID.MAXUPL - 1000;
    else
      maxupload = 0;
    Append_Files(filehandle, detfname, false, maxupload);
  }
  // Despite being upload, move CTD file to archive
  DOS_Com("move", MPC.FILENUM, "CTD", "CTD");
  delayms(50);
#endif
  //*** MPC.LOGFILE upload ***// Note: occurring only after reboot.
  if (TotalSeconds == 0) { //||MPC.UPLOAD==1)
    sprintf(logfile, "%08ld.log", MPC.FILENUM);
    DBG1("\t|WriteFile: %ld log file: %s", MPC.FILENUM, logfile)
    stat(logfile, &info);
    if (info.st_size > (long)(IRID.MAXUPL - 2000))
      maxupload = IRID.MAXUPL - 2000;
    else
      maxupload = 0;
    Append_Files(filehandle, logfile, false, maxupload);
    delayms(50);
  }
  delayms(50);
  DOS_Com("move", MPC.FILENUM, "LOG", "LOG");

  // Close File
  close(filehandle);
  delayms(25);

  // Return number of seconds of time on
  if (TotalSeconds == 0)
    return LoggingTime;
  else
    return 0;

} // WriteFile

//
// PrintSystemStatus()
//
char *PrintSystemStatus(void) {
  // global stringout
  sprintf(stringout, "boy: "
                        "%d%d%d%d\ndockDepth:%5.2f\nCURRENTDEPTH:%5."
                        "2f\nTOPDEPTH:%5.2f\nTARGETDPETH:%d\nAVG.VEL:%5."
                        "2f\nCTDSAMPLES:%d\n\0",
          boy.DATA ? 1 : 0, boy.SURFACED ? 1 : 0, boy.phase, boy.BUOYMODE,
          boy.dockDepth, boy.depth, boy.TOPDEPTH, boy.TDEPTH, boy.AVGVEL,
          boy.CTDSAMPLES);
  flogf("\n%s", stringout);
  delayms(100);

  return stringout;
}

//
// wait currChkSettle, buoy ctd, ant td, compute
// uses: ngk.boy2ant
//
float oceanCurr() {
  float aD, cD, a, b, c;
  // usually called while antMod is on
  antMode(idle_mod);
  mpcDevSwitch(ctd_dev);
  cD=ctdDepth();
  mpcDevSwitch(ant_dev);
  aD=antDepth();
  // a^2 + b^2 = c^2
  a=cD-aD;
  c=ngk.boy2ant;
  b=sqrt(pow(c,2)-pow(a,2));
  return b;
}

//
// uses: boy.sidewaysMax
//
bool oceanCurrChk() {
  flogf("\n\t| ocean current ");
  sideways = oceanCurr();
  flogf(" @%.1f=%.1f ", antDepth(), sideways);
  if (sideways>boy.sidewaysMax) {
    flogf("too strong, cancel ascent");
    return true;
  }
  return false;
}

//
// boyDiskSpace Returns the free space in kBytes
//
long boyDiskFree(void) {
  long freeSpacekB;
  long freeSectors;
  long totalSectors;
  //
  boy.diskFree = DSDFreeSectors('C' - 'A');
  boy.diskSize = DSDDataSectors('C' - 'A');
  return boy.diskFree/2;
} // boyDiskFree

//
// shutdown buoy, sleep, reset
//
void boyShutdown(void) {
  WISPRSafeShutdown();
  PIOClear(ANTENNAPWR); 
  PIOClear(AMODEMPWR); 
  sysSleepUntilWoken();
  BIOSReset();
}
