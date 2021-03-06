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
   setClockDiv(MAX_CLOCK_DIV);
   int refIdCode = getIdCode();
   if(verbose)
      printf("XVCDriver::startCalibration reference idcode: 0x%X\n", refIdCode);
  
   int id = 0;       // calibration id
   int cdiv = 0;     // clock divisor
   int cdel = 0;     // clock delay
   int cfreq;        // clock frequency
   int validPoints;  // consecutive valid measurements
   calibItem cal;

   int minDelay, maxDelay;

   for(cdiv=MAX_CLOCK_DIV; cdiv>0; cdiv--) {

      setClockDiv(cdiv);

      minDelay = -1;
      maxDelay = -1;
      validPoints = 0;
      cfreq = 100000000 / ((cdiv + 1) * 2);

      for(cdel=0; cdel<MAX_CLOCK_DELAY; cdel++) {

         setDelay(cdel);

         int idcode = getIdCode();

         if(idcode == refIdCode) {

            if(verbose)
               printf("XVCDriver::startCalibration idcode OK - clkdelay: %d - clkdiv: %d - clkfreq: %d\n",
                  cdel, cdiv, cfreq);

            if(minDelay == -1) {
               minDelay = cdel;
            }

            maxDelay = cdel;
            validPoints++;
         }

      }  // end for loop (clock delay)

      if(minDelay != -1) {    // we found a valid clkdelay/clkdiv combination...
         
         cal.id = id++;
         cal.clkDiv = cdiv;
         cal.clkDelay = (minDelay == maxDelay)?minDelay:(maxDelay + minDelay) / 2;
         cal.clkFreq = cfreq;
         cal.validPoints = validPoints;

         if(verbose)
            printf("XVCDriver::startCalibration idcode OK - clkdelay: %d - clkdiv: %d - clkfreq: %d - points: %d\n",
                  cal.clkDelay, cal.clkDiv, cal.clkFreq, cal.validPoints);

         calibList.push_back(cal);

      } else break;           // it is no useful to continue with higher frequency

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

   std::vector<calibItem>::iterator it;

   for(it=calibList.begin(); it!=calibList.end(); it++)
      printf("id:%d DIV:%d DLY:%d CLK:%d VALID:%d\n", it->id, it->clkDiv, it->clkDelay, it->clkFreq, it->validPoints);
}

bool XVCDriver::setCalibrationById(int id) {

   if(!hasCalibration()) {
      if(verbose)
         std::cout << "E: device does not support calibration" << std::endl;
      return false;
   }
   
   bool found = false;
   unsigned int i=0;

   for(i=0; i<calibList.size(); i++) {

      if(calibList[i].id == id) {

         found = true;
         setDelay(calibList[i].clkDelay);
         setClockDiv(calibList[i].clkDiv);
         break;
      }
   }

   if(verbose) {
      if(found)
         printf("XVCDriver::setCalibrationById id found: id:%d DIV:%d DLY:%d CLK:%d\n", 
               calibList[i].id, calibList[i].clkDiv, calibList[i].clkDelay, calibList[i].clkFreq);
      else
         printf("XVCDriver::setCalibrationById id not found: id:%d\n", id);
   }

   return found;
}

void XVCDriver::setCalibrationByClock(int freq) {

   if(!hasCalibration()) {
      if(verbose)
         std::cout << "E: device does not support calibration" << std::endl;
   }

   unsigned int i = 0;
   int id = 0;
   int delta = 0;
   bool init = false;

   for(i=0; i<calibList.size(); i++) {
      
      if (!init) {
         init = true;
         id = calibList[i].id;
         delta = abs(freq - calibList[i].clkFreq);
         continue;
      }

      if(delta > abs(freq - calibList[i].clkFreq)) {
         delta = abs(freq - calibList[i].clkFreq);
         id = calibList[i].id;
      }
   }

   setDelay(calibList[i].clkDelay);
   setClockDiv(calibList[i].clkDiv);

   if(verbose) 
      printf("XVCDriver::setCalibrationByClock id found req(%d) delta(%d): id:%d DIV:%d DLY:%d CLK:%d\n", 
            freq, delta, id, calibList[id].clkDiv, calibList[id].clkDelay, calibList[id].clkFreq);
}
