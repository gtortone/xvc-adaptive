#ifndef XVCDRIVER_H
#define XVCDRIVER_H

#include <iostream>
#include <stdio.h>

/*
   XVCDriver is an abstract class to specialize with a driver that use hardware 
   primitives to send/receive TMS,TDI/TDO to a device
*/

class XVCDriver {

public:
   XVCDriver();
   ~XVCDriver() {};

   std::string getName(void) { return name; };
   void setDebugLevel(int lvl) { debugLevel = lvl; };
   void setVerbose(bool v) { verbose = v; };

   virtual void shift(int nbits, unsigned char *buffer, unsigned char *result) = 0;
   uint32_t scanChain(void);
   uint32_t probeIdCode(void);

protected:
   int debugLevel = 0;
   bool verbose = false;

   void setName(std::string n) { name = n; };
   void printDebug(std::string msg, int lvl);

private:
   std::string name;
};

#endif
