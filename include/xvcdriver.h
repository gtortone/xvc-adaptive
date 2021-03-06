#ifndef XVCDRIVER_H
#define XVCDRIVER_H

#include <iostream>
#include <stdio.h>
#include <list>

/*
	XVCDriver is an abstract class to specialize with a driver that use hardware 
	primitives to send/receive TMS,TDI/TDO to a device
*/

typedef struct {
   int id;
   int clkDiv;
   int clkDelay;
   int clkFreq;
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
   bool setCalibrationParams(int id);

protected:
   bool debug = false;
   bool verbose = false;
   bool calibration = false;
   int delay;
   int clkdiv;

private:
   std::list<calibItem> calibList;
};

#endif
