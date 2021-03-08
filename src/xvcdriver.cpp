#include "xvcdriver.h"

XVCDriver::XVCDriver(void) {
}

void XVCDriver::printDebug(std::string msg, int lvl) {
   if (debugLevel >= lvl)
      std::cout << msg << std::endl;
}

uint32_t XVCDriver::probeIdCode(void) {

   printDebug("XVCDriver::probeIdCode start", 1);

   unsigned char buffer[8];
   unsigned char result[4];
   uint32_t *idcode;
   int nbits;

   buffer[0] = 0xFF;
   buffer[1] = 0x00;
   nbits = 5;
   shift(nbits, buffer, result);

   buffer[0] = 0x00;
   buffer[1] = 0x00;
   nbits = 1;
   shift(nbits, buffer, result);

   buffer[0] = 0x01;
   buffer[1] = 0x00;
   nbits = 3;
   shift(nbits, buffer, result);

   buffer[0] = 0x00;
   buffer[1] = 0x00;
   buffer[2] = 0x00;
   buffer[3] = 0x00;
   buffer[4] = 0xFF;
   buffer[5] = 0xFF;
   buffer[6] = 0xFF;
   buffer[7] = 0xFF;
   nbits = 32;
   shift(nbits, buffer, result);

   idcode = reinterpret_cast<uint32_t *>(result);

   printDebug("XVCDriver::probeIdCode end", 1);
   return *idcode; 
}
