#include "ftdidevice.h"

FTDIDevice::FTDIDevice() {
    
   calibration = false;
}

FTDIDevice::~FTDIDevice() {
}

void FTDIDevice::shift(int nbits, unsigned char *buffer, unsigned char *result) {
}
