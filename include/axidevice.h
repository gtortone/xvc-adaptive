#ifndef AXIDEVICE_H
#define AXIDEVICE_H

#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#include "xvcdriver.h"

/*
    AXIDevice is a device driver based on AXI4-JTAG IP core
*/

#define MAP_SIZE	0x10000

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
	AXIDevice();
	~AXIDevice();

   void setDelay(int v);
   void setClockDiv(int v);
	void shift(int nbits, unsigned char *buffer, unsigned char *result);

private:
	int fd;
	volatile jtag_t *ptr;
};

#endif
