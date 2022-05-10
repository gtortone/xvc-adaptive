#ifndef FTDIDEVICE_H
#define FTDIDEVICE_H

#include <iostream>
#include <algorithm>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <libftdi1/ftdi.h>
#include <usb.h>

#include "xvcdriver.h"
#include "devicedb.h"

/*
    FTDIDevice is a device driver based on FT2232H USB controller
*/

// default FTDI minimodule VID/PID
#define DEFAULT_VID	0x0403
#define DEFAULT_PID	0x6010

#define MAX_DATA     4096

#define MAX_CFREQ_DIV5_ON  6000000
#define MIN_CFREQ_DIV5_ON  91.55

#define MAX_CFREQ_DIV5_OFF 30000000
#define MIN_CFREQ_DIV5_OFF 457.76

#define DIV5_ON      true
#define DIV5_OFF     false

#define POS_EDGE     0x00
#define NEG_EDGE     MPSSE_READ_NEG    // 0x04

class FTDIDevice : public XVCDriver {

typedef struct {
   char oper; // 0-byte shift, 1-bit shift, 2-TMS bit shift
   int len;   // length of the operation
} data_desc;

public:
   FTDIDevice(int vid, int pid, enum ftdi_interface interface, const char *serial, char *busconf, bool v=false, int dl=0);
   ~FTDIDevice();

   bool detect(void);
   int getDivisorByFrequency(bool div5, int freq);
   int getFrequencyByDivisor(bool div5, int div);

   void setClockDiv(bool div5, int value);
   void setClockFrequency(int freq);
   void setTDOPosSampling(bool value);

   void readBytes(unsigned int len, unsigned char *buf);
   void shift(int nbits, unsigned char *buffer, unsigned char *result);

private:
   struct ftdi_context *ftdi;
   int samplingEdge = NEG_EDGE;
};

#endif
