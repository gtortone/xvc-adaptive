#include "xvcdriver.h"

XVCDriver::XVCDriver() {
	;
}

int XVCDriver::getIdCode(void) {

	unsigned char buffer[8];
   unsigned char result[4];
   int *idcode;
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

   idcode = reinterpret_cast<int*>(result);

   //printf("%X-%X-%X-%X\n", result[0], result[1], result[2], result[3]);

   return *idcode; 
}
