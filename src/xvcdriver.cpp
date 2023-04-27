#include "xvcdriver.h"
#include <vector>

XVCDriver::XVCDriver(void) {
}

void XVCDriver::printDebug(std::string msg, int lvl) {
   if (debugLevel >= lvl)
      std::cout << msg << std::endl;
}

uint32_t XVCDriver::probeIdCode(void) {

   printDebug("XVCDriver::probeIdCode start", 1);

   std::vector<unsigned char> buffer;
   unsigned char result[4];
   uint32_t *idcode;
   int nbits;

   // TEST-LOGIC-RESET
   buffer = {0xFF, 0x00};
   nbits = 5;
   shift(nbits, buffer.data(), result);

   // RUN-TEST/IDLE
   buffer = {0x00, 0x00};
   nbits = 1;
   shift(nbits, buffer.data(), result);

   // SHIFT-IR
   buffer = {0x03, 0x00};
   nbits = 4;
   shift(nbits, buffer.data(), result);

   // set IDcode instruction and navigate to EXIT-IR on last bit
   uint32_t tms = 1 << (irlen-1);
   uint32_t tdi = idcmd;
   int nbytes = (irlen + 7) / 8;
   for(int i=0; i<nbytes; i++) {
      buffer[i] = (tms >> 8*i) & 0x000000FF;
      buffer[i+nbytes] = (tdi >> 8*i) & 0x000000FF;
   }
   nbits = irlen;
   shift(nbits, buffer.data(), result);

   // UPDATE-IR
   buffer = {0x01, 0x00};
   nbits = 2;
   shift(nbits, buffer.data(), result);

   // SHIFT-DR
   buffer = {0x01, 0x00};
   nbits = 3;
   shift(nbits, buffer.data(), result);

   buffer = {0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00};
   nbits = 32;
   shift(nbits, buffer.data(), result);

   idcode = reinterpret_cast<uint32_t *>(result);
   
   printDebug("XVCDriver::probeIdCode end", 1);

   return *idcode;
}

uint32_t XVCDriver::scanChain(void) {

   printDebug("XVCDriver::scanChain start", 3);

   std::vector<unsigned char> buffer;
   unsigned char result[6];
   uint32_t idcode32;
   uint64_t *idcode64;
   int nbits;

   // TMS
   // 0000.0000.0000.0000.0000.0000.0000.0000.0000.0000.0101.1111
   // TDI
   // 0000.0001.1111.1111.1111.1111.1111.1111.1111.1110.0000.0000

   buffer = {0x5F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x01};
   nbits = 41;
   shift(nbits, buffer.data(), result);

   idcode64 = reinterpret_cast<uint64_t *>(result);
   idcode32 = (uint32_t) ( (*idcode64 >> 9) & (0xFFFFFFFF) );

   char msg[128];
   sprintf(msg, "XVCDriver::scanChain result = 0x%X", idcode32);
   printDebug(msg, 2);

   printDebug("XVCDriver::scanChain end", 3);
   return idcode32; 
}
