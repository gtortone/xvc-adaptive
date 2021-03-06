#ifndef XVCDRIVER_H
#define XVCDRIVER_H

#include <iostream>
#include <stdio.h>
#include <vector>

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
	XVCDriver() {};
	~XVCDriver() {};

   void setDebug(bool v) { debug = v; };
   void setVerbose(bool v) { verbose = v; };
   bool hasCalibration(void) { return calibration; };

   virtual void setDelay(int v) = 0;
   virtual void setClockDiv(int v) = 0;

	virtual void shift(int nbits, unsigned char *buffer, unsigned char *result) = 0;
   int getIdCode(void);

   void startCalibration(void);
   void printCalibrationList(void);
   bool setCalibrationById(int id);
   void setCalibrationByClock(int freq);

protected:
   bool debug = false;
   bool verbose = false;
   bool calibration = false;
   int delay;
   int clkdiv;

private:
   std::vector<calibItem> calibList;
};

#endif
