#ifndef AXICALIBRATOR_H
#define AXICALIBRATOR_H

#include <vector>

#include "axidevice.h"
#include "devicedb.h"

/*
   AXICalibrator is a class to calibrate an AXI device using clock divisor
   and clock delay probing device for a valid idcode
*/

typedef struct {
   int id;
   int clkDiv;
   int clkDelay;
   int clkFreq;
   int validPoints;
} calibItem;

class AXICalibrator {

public:
   AXICalibrator(AXIDevice *d);
   ~AXICalibrator();

   void setDebugLevel(int lvl) { debugLevel = lvl; }; 
   void setVerbose(bool v) { verbose = v; };

   const char* detectDevice(void);
   void start(void);
   void print(void);
   bool setById(int id);
   void setByClock(int freq);

private:
   AXIDevice *dev;
   int debugLevel = 0;
   bool verbose = false;
	std::vector<calibItem> calibList;
	uint32_t refIdCode;

   void printDebug(std::string msg, int lvl);
};

#endif

