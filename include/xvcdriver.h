#ifndef XVCDRIVER_H
#define XVCDRIVER_H

#include <stdio.h>

/*
	XVCDriver is an abstract class to specialize with a driver that use hardware 
	primitives to send/receive TMS,TDI/TDO to a device
*/

class XVCDriver {

public:
	XVCDriver();
	~XVCDriver() {};

   void setVerbose(bool v) { verbose = v; };

	virtual void shift(int nbits, unsigned char *buffer, unsigned char *result) = 0;
   int getIdCode(void);

protected:
   bool verbose = false;
};

#endif
