#include "xvcdriver.h"

XVCDriver::XVCDriver(void) {
}

const char* XVCDriver::detectDevice(void) {

   DeviceDB devDB(0);
   uint32_t tempId;
   const char *tempDesc;

   // read id code at low clock frequency with delay sweep
   setClockDiv(MAX_CLOCK_DIV);

   for(int cdel=0; cdel<MAX_CLOCK_DELAY; cdel++) {

      setDelay(cdel);
      tempId = probeIdCode();
      tempDesc = devDB.idToDescription(tempId);

      if (tempDesc) {
         refIdCode = tempId;
         return tempDesc;
      }
   }
   
   return nullptr; 
}

uint32_t XVCDriver::probeIdCode(void) {

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

   return *idcode; 
}

void XVCDriver::startCalibration(void) {

   const char *devDesc;

   if(!hasCalibration()) {
      std::cout << "E: device does not support calibration" << std::endl;
      return;
   }

   // reset previous calibration
   calibList.clear();

   // detect id code 
   devDesc = detectDevice();
   if(devDesc)
      std::cout << "I: detected device: " << devDesc << std::endl;
   else {
      std::cout << "E: no device detected" << std::endl;
      return;
   }

   std::cout << "I: calibration started" << std::endl;

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

         uint32_t idcode = probeIdCode();

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

   std::cout << "I: calibration finished" << std::endl;
}

void XVCDriver::printCalibrationList(void) {
   
   if(!hasCalibration()) {
      std::cout << "E: device does not support calibration" << std::endl;
      return;
   }

   std::vector<calibItem>::iterator it;

   for(it=calibList.begin(); it!=calibList.end(); it++)
      printf("id:%d DIV:%d DLY:%d CLK:%d VALID:%d\n", it->id, it->clkDiv, it->clkDelay, it->clkFreq, it->validPoints);
}

bool XVCDriver::setCalibrationById(int id) {

   if(!hasCalibration()) {
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
      std::cout << "E: device does not support calibration" << std::endl;
      return;
   }

   unsigned int i = 0;
   int id = 0;
   int index = 0;
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
         index = i;
      }
   }

   setDelay(calibList[index].clkDelay);
   setClockDiv(calibList[index].clkDiv);

   if(verbose) 
      printf("XVCDriver::setCalibrationByClock id found req(%d) delta(%d): id:%d DIV:%d DLY:%d CLK:%d\n", 
            freq, delta, id, calibList[index].clkDiv, calibList[index].clkDelay, calibList[index].clkFreq);
}
