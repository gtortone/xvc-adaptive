#ifndef XVCDRIVER_H
#define XVCDRIVER_H

#include <iostream>
#include <stdio.h>
#include <vector>
#include "devicedb.h"

/*
   XVCDriver is an abstract class to specialize with a driver that use hardware 
   primitives to send/receive TMS,TDI/TDO to a device
*/

#define  MAX_CLOCK_DIV     255
#define  MAX_CLOCK_DELAY   1024

typedef struct {
   int id;
   int clkDiv;
   int clkDelay;
   int clkFreq;
   int validPoints;
} calibItem;

class XVCDriver {

public:
   XVCDriver();
   ~XVCDriver() {};

   void setDebugLevel(int lvl) { debugLevel = lvl; };
   void setVerbose(bool v) { verbose = v; };
   bool hasCalibration(void) { return calibration; };

   virtual void setDelay(int v) = 0;
   virtual void setClockDiv(int v) = 0;

   virtual void shift(int nbits, unsigned char *buffer, unsigned char *result) = 0;
   const char* detectDevice(void);

   void startCalibration(void);
   void printCalibrationList(void);
   bool setCalibrationById(int id);
   void setCalibrationByClock(int freq);

protected:
   int debugLevel = 0;
   bool verbose = false;
   bool calibration = false;
   int delay;
   int clkdiv;

   void printDebug(std::string msg, int lvl);

private:
   std::vector<calibItem> calibList;
   uint32_t probeIdCode(void);
   uint32_t refIdCode;
};

#endif
