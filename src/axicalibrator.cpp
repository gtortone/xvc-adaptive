#include "axicalibrator.h"

AXICalibrator::AXICalibrator(AXIDevice *d) {
   dev = d;
}

void AXICalibrator::printDebug(std::string msg, int lvl) {
   if (debugLevel >= lvl)
      std::cout << msg << std::endl;
}

const char* AXICalibrator::detectDevice(void) {

   printDebug("AXICalibrator::detectDevice start", 1);

   DeviceDB devDB(0);
   uint32_t tempId;
   const char *tempDesc;

   // read id code at low clock frequency with delay sweep
   dev->setClockDiv(MAX_CLOCK_DIV);

   for(int cdel=0; cdel<MAX_CLOCK_DELAY; cdel++) {

      dev->setClockDelay(cdel);
      tempId = dev->probeIdCode();
      tempDesc = devDB.idToDescription(tempId);

      if (tempDesc) {
         refIdCode = tempId;
         return tempDesc;
      }
   }

   printDebug("AXICalibrator::detectDevice end", 1);

   return nullptr;
}

void AXICalibrator::start(void) {

   printDebug("AXICalibrator::start start", 1);

   const char *devDesc;

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

      dev->setClockDiv(cdiv);

      minDelay = -1;
      maxDelay = -1;
      validPoints = 0;
      cfreq = 100000000 / ((cdiv + 1) * 2);

      for(cdel=0; cdel<MAX_CLOCK_DELAY; cdel++) {

         dev->setClockDelay(cdel);

         uint32_t idcode = dev->probeIdCode();

         if(idcode == refIdCode) {

            if(debugLevel) {
               char msg[128];
               sprintf(msg, "AXICalibrator::startCalibration: idcode OK - clkdelay: %d - clkdiv: %d - clkfreq: %d", cdel, cdiv, cfreq);
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
            sprintf(msg, "AXICalibrator::startCalibration: idcode OK - clkdelay: %d - clkdiv: %d - clkfreq: %d - points: %d",
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

   printDebug("AXICalibrator::start end", 1);
   std::cout << "I: calibration finished" << std::endl;
}

void AXICalibrator::print(void) {

   printDebug("AXICalibrator::print start", 1);
   
   std::vector<calibItem>::iterator it;

   for(it=calibList.begin(); it!=calibList.end(); it++)
      printf("id:%d DIV:%d DLY:%d CLK:%d VALID:%d\n", it->id, it->clkDiv, it->clkDelay, it->clkFreq, it->validPoints);

   printDebug("AXICalibrator::print end", 1);
}

bool AXICalibrator::setById(int id) {

   printDebug("AXICalibrator::setById start", 1);

   bool found = false;
   unsigned int i=0;

   for(i=0; i<calibList.size(); i++) {

      if(calibList[i].id == id) {

         found = true;
         dev->setClockDelay(calibList[i].clkDelay);
         dev->setClockDiv(calibList[i].clkDiv);
         break;
      }
   }

   if(verbose) {
      if(found)
         printf("AXICalibrator::setById id found: id:%d DIV:%d DLY:%d CLK:%d\n", 
               calibList[i].id, calibList[i].clkDiv, calibList[i].clkDelay, calibList[i].clkFreq);
      else
         printf("AXICalibrator::setById id not found: id:%d\n", id);
   }

   printDebug("AXICalibrator::setById end", 1);

   return found;
}

void AXICalibrator::setByClock(int freq) {

   printDebug("AXICalibrator::setByClock start", 1);

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

   dev->setClockDelay(calibList[index].clkDelay);
   dev->setClockDiv(calibList[index].clkDiv);

   if(verbose) 
      printf("AXICalibrator::setByClock id found req(%d) delta(%d): id:%d DIV:%d DLY:%d CLK:%d\n", 
            freq, delta, id, calibList[index].clkDiv, calibList[index].clkDelay, calibList[index].clkFreq);

   printDebug("AXICalibrator::setByClock end", 1);
}
