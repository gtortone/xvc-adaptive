#ifndef FTDIDEVICE_H
#define FTDIDEVICE_H

#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#include "xvcdriver.h"

/*
    FTDIDevice is a device driver based on FT2232H USB controller
*/

class FTDIDevice : public XVCDriver {

public:
   FTDIDevice();
   ~FTDIDevice();

   void setDelay(int v) {};
   void setClockDiv(int v) {};
   void shift(int nbits, unsigned char *buffer, unsigned char *result);
};

#endif
