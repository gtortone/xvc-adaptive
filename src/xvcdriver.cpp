#include "xvcdriver.h"

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

   return *idcode; 
}

void XVCDriver::startCalibration(void) {

   if(!hasCalibration()) {
      if(verbose)
         std::cout << "E: device does not support calibration" << std::endl;
      return;
   }

   // reset previous calibration
   calibList.clear();

	// read id code at low clock frequency
	setDelay(0);
	setClockDiv(255);
	int refIdCode = getIdCode();
	if(verbose)
		printf("XVCDriver::startCalibration reference idcode: 0x%X\n", refIdCode);
  
   int id = 0;       // calibration id
   int cdiv = 0;     // clock divisor
   int cdel = 0;     // clock delay
   int cfreq;        // clock frequency
   calibItem cal;

   int minDelay, maxDelay;

   for(cdiv=0; cdiv<256; cdiv++) {

      setClockDiv(cdiv);

      minDelay = -1;
      maxDelay = -1;
		cfreq = 100000000 / ((cdiv + 1) * 2);

		for(cdel=0; cdel<128; cdel++) {

			setDelay(cdel);

			int idcode = getIdCode();

			if(idcode != refIdCode) {
				
				if(verbose)
					printf("XVCDriver::startCalibration idcode mismatch - ref(%X) read(%X) - clkdelay: %d - clkdiv: %d - clkfreq: %d",
                     refIdCode, idcode, cdel, cdiv, cfreq);
			} else {

            if(minDelay == -1)
               minDelay = cdel;

            maxDelay = cdel;
         }

		}	// end for loop (clock delay)

      if(minDelay != -1) {    // we found a valid clkdelay/clkdiv combination...
         
         cal.id = id++;
         cal.clkDiv = cdiv;
         cal.clkDelay = (maxDelay - minDelay)/2;
         cal.clkFreq = cfreq;

         calibList.push_back(cal);
      }

   } // end for loop (clock divisor)

   if(verbose)
      printf("XVCDriver::startCalibration calibration done\n");
}

void XVCDriver::printCalibrationList(void) {
   
   if(!hasCalibration()) {
      if(verbose)
         std::cout << "E: device does not support calibration" << std::endl;
      return;
   }

   std::list<calibItem>::iterator it;

   for(it=calibList.begin(); it!=calibList.end(); it++)
      printf("id:%d DIV:%d DLY:%d CLK:%d\n", it->id, it->clkDiv, it->clkDelay, it->clkFreq);
}

bool XVCDriver::setCalibrationParams(int id) {

   if(!hasCalibration()) {
      if(verbose)
         std::cout << "E: device does not support calibration" << std::endl;
      return false;
   }
   
   std::list<calibItem>::iterator it;
   bool found = false;

   for(it=calibList.begin(); it!=calibList.end(); it++) {

      if(it->id == id) {

         found = true;
         setDelay(it->clkDelay);
         setClockDiv(it->clkDiv);
         break;
      }
   }

   if(verbose) {
      if(found)
         printf("XVCDriver::setCalibrationParams id found: id:%d DIV:%d DLY:%d CLK:%d\n", it->id, it->clkDiv, it->clkDelay, it->clkFreq);
      else
         printf("XVCDriver::setCalibrationParams id not found: id:%d\n", id);
   }

   return found;
}
