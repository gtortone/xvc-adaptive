#ifndef XVCDRIVER_H
#define XVCDRIVER_H

#include <iostream>
#include <vector>
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
   void startBypass(void);
   uint32_t probeBypass(const uint32_t value); 
   std::vector<uint32_t> probeBypass(const std::vector<uint32_t> data);
   bool isDetected(void) { return detected; };

   uint32_t getIdCode(void) { return idcode; };
   int getIdCmd(void) { return idcmd; };
   int getIrLen(void) { return irlen; };
   std::string getDescription(void) { return desc; }

protected:
   uint32_t idcode;
   int idcmd, irlen;
   std::string desc;

   int debugLevel = 0;
   bool verbose = false;
   bool detected = false;

   void setName(std::string n) { name = n; };
   void printDebug(std::string msg, int lvl);

private:
   std::string name;
};

#endif
