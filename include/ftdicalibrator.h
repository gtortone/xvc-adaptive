#ifndef FTDICALIBRATOR_H
#define FTDICALIBRATOR_H

#include <vector>

#include "ftdidevice.h"
#include "ftdisetup.h"

/*
   AXICalibrator is a class to calibrate an AXI device using clock divisor
   and clock delay probing device for a valid idcode
*/

class FTDICalibrator {

public:
   FTDICalibrator(FTDIDevice *d);
   ~FTDICalibrator();

   void setDebugLevel(int lvl) { debugLevel = lvl; }; 
   void setVerbose(bool v) { verbose = v; };

   void start(FTDISetup *setup, int minFreq, int maxFreq, int loop);

private:
   FTDIDevice *dev;
   int debugLevel = 0;
   bool verbose = false;

   void printDebug(std::string msg, int lvl);
};

#endif

