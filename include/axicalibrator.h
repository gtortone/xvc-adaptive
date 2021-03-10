#ifndef AXICALIBRATOR_H
#define AXICALIBRATOR_H

#include <vector>

#include "axidevice.h"
#include "axisetup.h"
#include "devicedb.h"

/*
   AXICalibrator is a class to calibrate an AXI device using clock divisor
   and clock delay probing device for a valid idcode
*/

#define  HYSTERESIS    5

class AXICalibrator {

public:
   AXICalibrator(AXIDevice *d);
   ~AXICalibrator();

   void setDebugLevel(int lvl) { debugLevel = lvl; }; 
   void setVerbose(bool v) { verbose = v; };

   void start(AXISetup *setup);

private:
   AXIDevice *dev;
   int debugLevel = 0;
   bool verbose = false;

   void printDebug(std::string msg, int lvl);
};

#endif

