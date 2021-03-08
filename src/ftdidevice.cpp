#include "ftdidevice.h"

FTDIDevice::FTDIDevice() {
    
   setName("FTDI");
}

FTDIDevice::~FTDIDevice() {
}

void FTDIDevice::shift(int nbits, unsigned char *buffer, unsigned char *result) {
}
