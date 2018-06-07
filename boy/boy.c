// boy.c

#include <utl.h>
#include <boy.h> 

#include <ant.h> 
#include <ctd.h>
#include <gps.h>
#include <mpc.h>
#include <ngk.h>
#include <pwr.h>
#include <sys.h>
#include <tmr.h>
#include <wsp.h>

BoyInfo boy;

///
// deploy or reboot, then loop over phases data/rise/irid/fall
// sets: boy.phase .phasePrev
void boyMain() {
  int starts, cycle=0;
  PhaseType phaseNext;
  // boy.phase set by sys.cfg
  starts = sysInit();
  mpcInit();
  antInit();
  boyInit();
  ctdInit();
  ngkInit();
  wspInit();
  pwrInit();
  if (starts>1) 
    boy.phase = reboot_pha;
  flogf("\nboyMain(): starting with phase %d", boy.phase);
    
  while (true) {
    utlX();
    flogf("\n!boyMain(): cycle %d of %d\n", cycle, boy.cycleMax);
    if (boy.cycleMax && (cycle >= boy.cycleMax))
      sysStop("cycleLimit");
    sysFlush();                    // flush all log file buffers
    boy.phaseT = time(0);
    switch (boy.phase) {
    case data_pha: // data collect by WISPR
      phaseNext = dataPhase();
      cycle++;
      break;
    case rise_pha: // Ascend buoy, check for current and ice
      phaseNext = risePhase();
      break;
    case irid_pha: // Call home via Satellite
      phaseNext = iridPhase();
      break;
    case fall_pha: // Descend buoy, science sampling
      phaseNext = fallPhase();
      break;
    case deploy_pha:
      phaseNext = deployPhase();
      break;
    case reboot_pha:
      phaseNext = rebootPhase();
      break;
    case error_pha:
      phaseNext = errorPhase();
      break;
    } // switch
    boy.phasePrev = boy.phase;
    boy.phase = phaseNext;
  } // while true
} // boyMain() 

///
// open log
void boyInit(void) {
  DBG0("boyInit()")
  boy.log = utlLogFile(boy.logFile);
} // boyInit

/// 
// ??
// figure out whats happening, continue as possible
// load info from saved previous phase
// ask antmod for our velocity
// sets: boy.phase
PhaseType rebootPhase(void) {
  flogf("\n+rebootPhase()@%s", utlDateTime());
  return fall_pha;
} // reboot()

///
// wispr recording and detecting, buoy is docked to ngk
// data is gathered for about 24hours (data_tmr)
// wsp powers down for % of each hour (wispr_tmr)
// organize data files, transfer data to antmod ??
// sleep needs a lot of optimizing to be worth the trouble
// uses: data_tmr duty_tmr
PhaseType dataPhase(void) {
  int detect;
  flogf("\n+dataPhase()@%s", utlDateTime());
  wspStart(wsp2_pam);
  wspDetect(&detect);
  flogf("\ndataPhase detections: %d", detect);
  wspStorm(utlBuf);
  flogf("\nstorm: %s", utlBuf);
  wspStop();
  return rise_pha;
} // dataPhase

///
// ascend. check angle due to current, up midway, re-check angle, surface.
// sets: boy.alarm[]
PhaseType risePhase(void) {
  bool success;
  flogf("\n+risePhase()@%s", utlDateTime());
  antStart();
  ctdStart();
  ngkStart();
  // if current is too strong at bottom
  if (oceanCurrChk()) {
    sysAlarm(bottomCurr_alm);
    //?? return fall_pha;
  }
  success = rise(boy.currChkD, 0);
  if (!success) {
    flogf("\n\t| rise fails at %3.1f m", antDepth());
    //??  return fall_pha;
  }
  // if current is too strong at midway
  if (oceanCurrChk()) {
    sysAlarm(midwayCurr_alm);
    //?? return fall_pha;
  }
  // surface
  success = rise(antSurfD()-1, 0);
  if (!success) {
    flogf(" | fails at %3.1f m", antDepth());
    //?? return fall_pha;
  }
  // success
  return irid_pha;
} // risePhase


///
// rise to targetD, 0 means surfacing 
// when surfacing, expect stopCmd and don't set velocity 
// 0 target tmr (~60) D-TD / rate * accu
// 1 ngk tmr (16) 2*ngkDelay +2
// 2 2meter tmr (20) ngkDelay+2/rate +3
// uses: .riseRate .riseOrig .riseAccu .riseRetry
// sets: .riseRate
int rise(float targetD, int try) {
  bool twentyB=false, stopB=false, errB=false;
  float nowD, startD, lastD, velo;
  int i, est;        // estimated operation time
  MsgType msg;
  enum {targetT, ngkT, twentyT, threeT};  // local timer names
  DBG0("rise(%3.1f)", targetD)
  nowD = startD = antDepth();
  if (startD < targetD) return 1;
  if (try > boy.riseRetry) return 2;
  // .riseOrig=as tested, .riseRate=seen, .riseAccu=fudgeFactor
  est = (int) ((nowD-targetD) / boy.riseRate * boy.riseAccu);
  tmrStart(targetT, est);
  tmrStart(ngkT, boy.ngkDelay*2);
  tmrStart(twentyT, 20);
  ngkSend(riseCmd_msg);
  flogf("\nrise()\t| riseCmd to winch at %s", utlTime());
  while (!stopB && !errB) {
    // check: target, ngk, 3s, 20s
    nowD = antDepth();
    // arrived?
    if (nowD<targetD) {
      flogf("\nrise()\t| reached targetD %3.1f at %s", nowD, utlTime());
      tmrStop(targetT);
      stopB = true;
      break;
    }
    // op timeout - longer than estimated time * 1.5 (riseAccu)
    if (tmrExp(targetT)) {
      flogf("\nrise()\t| ERR \t| rise timeout %ds @ %3.1f, stop", est, nowD);
      errB = true;
      break;
    }
    // winch
    if (ngkRecv(&msg)!=null_msg) {
      flogf(", %s from winch", ngkMsgName(msg));
      if (msg==stopCmd_msg)
        return -1;
      if (msg==riseRsp_msg)
        tmrStop(ngkT);
    }
    if (tmrExp(ngkT)) {
      flogf("\nrise()\t| no response from winch");
    }
    // 20 seconds
    if (tmrExp(twentyT)) {
      if (startD-nowD < 1.5) {
        // by now we should have moved up 3 meters
        flogf("\nrise()\t| ERR \t| depth %3.1f after 20 seconds", nowD);
        errB = true;
        break;
      } else {
        twentyB = true;
        lastD = nowD;
      }
    }
    // 3 seconds (after 20s)
    if (tmrExp(threeT)) {
      tmrStart(threeT, 3);
      flogf("\nrise()\t| 3s: depth=%3.1f", nowD);
      if (antVelo(&velo)) 
        flogf(" velo=%3.1f", velo);
      if (twentyB) {
        if (lastD<=nowD) {
          flogf("\nrise()\t| ERR \t| not rising, %3.1f<=%3.1f", lastD, nowD);
          lastD = nowD;
        }
      }
    } 
  } // while !stop
  // stop - either normal or due to err
  for (i=0; i<boy.riseRetry; i++) {
    ngkSend(stopCmd_msg);
    if (ngkRecvWait(&msg, boy.ngkDelay*2+2)==stopRsp_msg) break;
  }
  // ?? if (msg!=stopRsp_msg) damnation
  flogf(", stopCmd-->%s", ngkMsgName(msg));
  // retry if error
  if (errB) {
    flogf(", retry %d", ++try);
    return rise(targetD, try);
  } else { 
    // normal stop
    return 0;
  }
} // rise

///
// ??
// turn off sbe, on irid/gps (takes 30 sec). 
// read gps date, loc. 
PhaseType iridPhase(void) {
  flogf("\n+iridPhase()@%s", utlDateTime());
  if (gps.avail) {
    gpsStart();
    gpsStats();
    iridSig();
    gpsStop();
  } else
    flogf("... gps not avail");
  return fall_pha;
} // iridPhase

///
// antAuto, science(log), startT, fallCmd, science(stop)
// while (!docked) {
//  fallcmd, [rsp] or time. 
//  while(moving) {sleep 8} 
//  [stopcmd] or time, rsp. 
//  docked?}
// failMode:=stage of failure 0,1; tryMax[], delay[] indexed by failMode
// if (err>errMax) failMode++ <= failMax
// 20 retries: 5@1sec, 5@10min, 10@1hour
// errMax[failMode], delay[failMode]
PhaseType fallPhase() {
  int err = 0, step = 1;
  int failStage = 0, failMax = 2;
  int errMax[3] = {5, 10, 20}, delay[3] = {1, 10*60, 60*60};
  float depth, startD, fallD;
  time_t fallT;
  PhaseType r = data_pha;
  MsgType msg;
  flogf("\n+fallPhase() %s", utlTime());
  antRingReset();
  startD = depth = antDepth();
  // step1. loop until fallRsp or falling+timeout
  step = 1;
  DBG0("dP:1")
  // ctdAuton(true);
  ngkSend( fallCmd_msg );
  while (step==1) {
    utlX();
    depth = antDepth();
    switch (ngkRecv(&msg)) {
    case null_msg: break;
    case fallRsp_msg:
      // start velocity measure
      fallT = time(0);
      fallD = depth;
      step = 2;
      break; 
    default:
      flogf("\n\t|fallP() unexpected %s at %3.1f m", ngkMsgName(msg), depth);
    } // switch
    // err, delay and retry algorithm 
    if (tmrExp(ngk_tmr)) {
      flogf(" timeout"); 
      // timeout
      if (depth - startD > 2) {  // no antMoving close to surface
        // odd, we are falling but no response; log but allow
        flogf(" but falling"); 
        step = 2;
        break; // while step1
      } // if depth
      if (++err > errMax[failStage] && ++failStage > failMax) {
        // note: ++err, ++failStage as errs exceed errMax
        flogf("\n\t|fallPhase() too many retries, panic");
        // r = error_pha;
        step = 6;
        break; // while step1
      }
      // here: timeout & !falling & errs<max
      // retry fallCmd, use longer delay after more errs
      utlNap(delay[failStage]);
      flogf(" retry fallCmd"); 
      ngkSend( fallCmd_msg );
    } // if timeout
  } // while step1
  // step 2: drop til you stop
  if (step==2)
    step = 3;
  /*
  DBG0("dP:2")
  antFlush();
  utlNap(20);
  while (step==2) {
    utlX();
    depth = antDepth();
    if (antMoving()<=0.0)
      step = 3;
  }
   */ 
  DBG0("dP:3")
  // step 3: stopCmd Rsp
  tmrStart(ngk_tmr, 300);        // ?? 
  while (step==3) {
    utlX();
    depth = antDepth();
    switch (ngkRecv(&msg)) {
    case null_msg: break;
    case stopCmd_msg:
      ngkSend(stopRsp_msg);
      step = 4;
      break;
    default:
      flogf("\n\t|fallP() unexpected %s at %3.1f m", ngkMsgName(msg), depth);
    } // switch
    if (tmrExp(ngk_tmr)) {
      flogf(" 5min timeout");
      step = 4;
    }
  } // while msg
  DBG0("dP:4")
  // step 4: docked?
  if (step==4) {
    depth = antDepth();
    if (!boyDocked(depth)) {
      // if stop but not docked, cable is stuck
      flogf(" not docked at %4.2f", depth);
      utlErr(ngk_err, "fallPhase step 4, we should be docked");
      sysAlarm(cableStuck_alm);
      err++;
    }
    // velocity, finish science
    if (err==0) {
      // skip if we didn't fall clean
      boy.fallVLast = (fallD-depth) / (time(0)-fallT);
      if (boy.fallVFirst==0)
        boy.fallVFirst = boy.fallVLast;
    } // if err
  } // if step4
  // get samples, depends on ant. ctd.logging
  // antGetSamples();
  // ctdGetSamples();
  // turn off ant, clear ngk, clear ctd
  ctdAuton(false);
  ctdStop();
  antStop();
  ngkStop();
  return r;
} // fallPhase

///
// from ship deck to ocean floor
// wait until under 10m, watch until not falling, wait 30s, riseUp()
PhaseType deployPhase(void) {
  float depth, lastD;
  ngkStart();
  antStart();
  antAutoSample(true);
  tmrStart( deploy_tmr, 60*60*2 );
  depth = antDepth();
  flogf("\n+deployPhase()@%s %4.2fm", utlDateTime(), depth);
  // wait until under 10m
  while ((depth = antDepth())<10.0) {
    flogf("\ndeployPhase() at %4.2fm", depth);
    if (tmrExp(deploy_tmr)) 
      sysStop("deployP() 2 hour timeout");
    utlNap(30);
  }
  tmrStop(deploy_tmr);
  flogf("\n\t| %4.2fm>10 so wait until not moving\n", depth);
  lastD = depth;
  utlNap(120);
  // at most 5 min to descend, already waited 2min
  tmrStart( deploy_tmr, 60*5 );
  depth = antDepth();
  // must fall at least 1m in 10 sec
  while (depth-lastD>1.0) {
    utlNap(15);
    if (tmrExp(deploy_tmr)) 
      break;
    lastD = depth;
    depth = antDepth();
  }
  tmrStop(deploy_tmr);
  // we are docked
  depth = antDepth();
  boy.dockD = depth;
  flogf("\n\t| boy.dockD = %4.2f", boy.dockD);
  flogf("\n\t| go to surface, call home");
  utlNap(10);
  return rise_pha;
} // deployPhase

///
// ??
// cable is stuck. up/down tries??, down to dock. 
// go back to normal if resolved ??
PhaseType errorPhase(void) {
  flogf("\n+errorPhase()@%s", utlDateTime());
  return fall_pha;
} // errorPhase

///
// wait currChkSettle, buoy ctd, ant td, compute
// uses: .boy2ant
float oceanCurr() {
  float aD, cD, a, b, c;
  cD=ctdDepth();
  aD=antDepth();
  // pythagoras a^2 + b^2 = c^2
  // b:=horizontal displacement, caused by current
  a=cD-aD;
  c=boy.boy2ant;
  DBG1("aD=%4.2f cD=%4.2f boy2ant=%4.2f", aD, cD, c)
  if (a<0 || c<a) return -1.0;
  b=sqrt(pow(c,2)-pow(a,2));
  DBG1("sideways=%4.2f", b)
  return b;
} // oceanCurr

///
// uses: boy.currMax
bool oceanCurrChk() {
  float sideways;
  flogf("\n\t| oceanCurrChk() ");
  sideways = oceanCurr();
  if (sideways<0) {
    utlErr(logic_err, "oceanCurr invalid value");
    return false;
  }
  flogf(" @%.1f=%.1f ", antDepth(), sideways);
  if (sideways>boy.currMax) {
    flogf("too strong, cancel ascent");
    // ignore current when dbg ?? should be setting
    DBGX(return false;)
    return true;
  }
  return false;
} // oceanCurrChk

///
// shutdown buoy, reflects boyInit
void boyStop(void) {} // ??

///
void boyFlush(void) {} // ??

///
// do not use until 
bool boyDocked(float depth) {
  if (boy.dockD==0.0) return false;
  else return (abs(depth-boy.dockD)<1.0);
}
