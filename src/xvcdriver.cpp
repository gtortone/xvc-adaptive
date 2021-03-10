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

	// TEST-LOGIC-RESET
  	buffer[0] = 0xFF;
   buffer[1] = 0x00;
   nbits = 5;
   shift(nbits, buffer, result);

	// RUN-TEST/IDLE
	buffer[0] = 0x00;
   buffer[1] = 0x00;
   nbits = 1;
   shift(nbits, buffer, result);

	// SHIFT-IR
	buffer[0] = 0x03;
	buffer[1] = 0x00;
	nbits = 4;
	shift(nbits, buffer, result);

	// set IDcode instruction and navigate to EXIT-IR on last bit
   uint16_t word = 0;
   word = (1 << irlen);
	buffer[0] = word & 0x00FF;
   buffer[1] = (word & 0xFF00) >> 8;
   word = idcmd;
	buffer[2] = word & 0x00FF;
   buffer[3] = (word & 0xFF00) >> 8;
	nbits = irlen;
	shift(nbits, buffer, result);
	 
	// UPDATE-IR
	buffer[0] = 0x01;
	buffer[1] = 0x00;
	nbits = 2;
	shift(nbits, buffer, result);

	// SHIFT-DR
	buffer[0] =	0x01;
	buffer[1] = 0x00;
	nbits = 3;
	shift(nbits, buffer, result);

	buffer[0] = 0x00;
   buffer[1] = 0x00;
   buffer[2] = 0x00;
   buffer[3] = 0x80;
   buffer[4] = 0x00;
   buffer[5] = 0x00;
   buffer[6] = 0x00;
   buffer[7] = 0x00;
   nbits = 32;
   shift(nbits, buffer, result);

	idcode = reinterpret_cast<uint32_t *>(result);
   
	// TEST-LOGIC-RESET
  	buffer[0] = 0xFF;
   buffer[1] = 0x00;
   nbits = 5;
   shift(nbits, buffer, result);

   printDebug("XVCDriver::probeIdCode end", 1);

	return *idcode;
}

uint32_t XVCDriver::scanChain(void) {

   printDebug("XVCDriver::scanChain start", 1);

   unsigned char buffer[12];
   unsigned char result[6];
   uint32_t idcode32;
   uint64_t *idcode64;
   int nbits;

   // TMS
   buffer[0] = 0x5F;       // 0000.0000.0000.0000.0000.0000.0000.0000.0000.0000.0101.1111
   buffer[1] = 0x00;
   buffer[2] = 0x00;
   buffer[3] = 0x00;
   buffer[4] = 0x00;
   buffer[5] = 0x00;

   // TDI
   buffer[6] = 0x00;       // 0000.0001.1111.1111.1111.1111.1111.1111.1111.1110.0000.0000
   buffer[7] = 0xFE;
   buffer[8] = 0xFF;
   buffer[9] = 0xFF;
   buffer[10] = 0xFF;
   buffer[11] = 0x01;         
   
   nbits = 41;
   shift(nbits, buffer, result);

   idcode64 = reinterpret_cast<uint64_t *>(result);
   idcode32 = (uint32_t) ( (*idcode64 >> 9) & (0xFFFFFFFF) );

   char msg[128];
   sprintf(msg, "XVCDriver::scanChain result = 0x%X", idcode32);
   printDebug(msg, 2);

   printDebug("XVCDriver::scanChain end", 1);
   return idcode32; 
}
