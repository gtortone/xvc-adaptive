#ifndef AXICALIBRATOR_H
#define AXICALIBRATOR_H

#include <vector>

#include "axidevice.h"
#include "axisetup.h"

/*
   AXICalibrator is a class to calibrate an AXI device using clock divisor
   and clock delay probing device for a valid idcode
*/

class AXICalibrator {

public:
   AXICalibrator(AXIDevice *d);
   ~AXICalibrator();

   void setDebugLevel(int lvl) { debugLevel = lvl; }; 
   void setVerbose(bool v) { verbose = v; };

   void setHysteresis(int v) { hyst = v; };
   void start(AXISetup *setup);

private:
   AXIDevice *dev;
   int debugLevel = 0;
   bool verbose = false;
   int hyst = 0;

   void printDebug(std::string msg, int lvl);
};

#endif

