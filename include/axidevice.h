#ifndef AXIDEVICE_H
#define AXIDEVICE_H

#include <iostream>
#include <stdexcept>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#include "xvcdriver.h"
#include "devicedb.h"

/*
    AXIDevice is a device driver based on AXI4-JTAG IP core
*/

#define  MAX_CLOCK_DIV     255
#define  MAX_CLOCK_DELAY   1024
#define  MAP_SIZE          0x10000

typedef struct {
   uint32_t length_offset;
   uint32_t tms_offset;
   uint32_t tdi_offset;
   uint32_t tdo_offset;
   uint32_t ctrl_offset;
   uint32_t delay_offset;
   uint32_t tck_ratio_div2_min1_offset;
} jtag_t;

class AXIDevice : public XVCDriver {

public:
   AXIDevice(bool v=false, int dl=0);
   ~AXIDevice();

   bool detect(void);
   void setClockDelay(int v);
   void setClockDiv(int v);
   void shift(int nbits, unsigned char *buffer, unsigned char *result);

private:
   int fd;
   volatile jtag_t *ptr;

   int clkdiv, clkdel;
};

#endif
