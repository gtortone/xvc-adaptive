#include "xvcdriver.h"

XVCDriver::XVCDriver(void) {
}

void XVCDriver::printDebug(std::string msg, int lvl) {
   if (debugLevel >= lvl)
      std::cout << msg << std::endl;
}

const char* XVCDriver::detectDevice(void) {

   printDebug("XVCDriver::detectDevice start", 1);

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
   
   printDebug("XVCDriver::detectDevice end", 1);

   return nullptr; 
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

void XVCDriver::startCalibration(void) {

   printDebug("XVCDriver::startCalibration start", 1);

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

   int id = 0;       // calibration id
   int cdiv = 0;     // clock divisor
   int cdel = 0;     // clock delay
   int cfreq;        // clock frequency
   int validPoints;  // consecutive valid measurements
   calibItem cal;

   int minDelay, maxDelay;
   int currIter = 0;
   int interval = 5;

   for(cdiv=MAX_CLOCK_DIV; cdiv>0; cdiv--) {

      currIter = float(MAX_CLOCK_DIV - cdiv) / MAX_CLOCK_DIV * 100;
      if(currIter >= interval) {
         std::cout << interval << "%..." << std::flush;
         interval += 5;
      }

      setClockDiv(cdiv);

      minDelay = -1;
      maxDelay = -1;
      validPoints = 0;
      cfreq = 100000000 / ((cdiv + 1) * 2);

      for(cdel=0; cdel<MAX_CLOCK_DELAY; cdel++) {

         setDelay(cdel);

         uint32_t idcode = probeIdCode();

         if(idcode == refIdCode) {

            if(debugLevel) {
               char msg[128];
               sprintf(msg, "XVCDriver::startCalibration: idcode OK - clkdelay: %d - clkdiv: %d - clkfreq: %d", cdel, cdiv, cfreq);
               printDebug(msg, 2);
            }

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

         if(debugLevel) {
            char msg[128];
            sprintf(msg, "XVCDriver::startCalibration: idcode OK - clkdelay: %d - clkdiv: %d - clkfreq: %d - points: %d",
                  cal.clkDelay, cal.clkDiv, cal.clkFreq, cal.validPoints);
            printDebug(msg, 2);
         }

         calibList.push_back(cal);

      } else {

         // it is no useful to continue with higher frequency
         std::cout << std::endl;
         std::cout << "I: clock frequency equal or higher " << cfreq << " Hz skipped" << std::endl;
         break;           
      }

   } // end for loop (clock divisor)

   printDebug("XVCDriver::startCalibration end", 1);
   std::cout << "I: calibration finished" << std::endl;
}

void XVCDriver::printCalibrationList(void) {

   printDebug("XVCDriver::printCalibrationList start", 1);
   
   if(!hasCalibration()) {
      std::cout << "E: device does not support calibration" << std::endl;
      return;
   }

   std::vector<calibItem>::iterator it;

   for(it=calibList.begin(); it!=calibList.end(); it++)
      printf("id:%d DIV:%d DLY:%d CLK:%d VALID:%d\n", it->id, it->clkDiv, it->clkDelay, it->clkFreq, it->validPoints);

   printDebug("XVCDriver::printCalibrationList end", 1);
}

bool XVCDriver::setCalibrationById(int id) {

   printDebug("XVCDriver::setCalibrationById start", 1);

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

   printDebug("XVCDriver::setCalibrationById end", 1);

   return found;
}

void XVCDriver::setCalibrationByClock(int freq) {

   printDebug("XVCDriver::setCalibrationByClock start", 1);

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

   printDebug("XVCDriver::setCalibrationByClock end", 1);
}
