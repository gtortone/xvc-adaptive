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

void AXICalibrator::start(AXISetup *setup) {

   printDebug("AXICalibrator::start start", 1);

   const char *devDesc;

   // reset previous calibration
   setup->clear();

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

   int minDelay, maxDelay;
   int currIter = 0;
   int interval = 5;
   int hyst = 0;
   bool record = false;

   for(cdiv=MAX_CLOCK_DIV; cdiv>=0; cdiv--) {

      currIter = float(MAX_CLOCK_DIV - cdiv) / MAX_CLOCK_DIV * 100;
      if(currIter >= interval) {
         std::cout << interval << "%..." << std::flush;
         interval += 5;
      }

      dev->setClockDiv(cdiv);

      minDelay = -1;
      maxDelay = -1;
      validPoints = 0;
      record = false;
      cfreq = 100000000 / ((cdiv + 1) * 2);
      hyst  = 0 ;

      for(cdel=0; cdel<MAX_CLOCK_DELAY; cdel++) {

         dev->setClockDelay(cdel);

         uint32_t idcode = dev->probeIdCode();

         if(idcode == refIdCode) {

            if(minDelay == -1) {

               if(++hyst >= HYSTERESIS) {
                  if(debugLevel) {
                     char msg[128];
                     sprintf(msg, "AXICalibrator::startCalibration: idcode OK - clkdelay: %d - clkdiv: %d - clkfreq: %d", cdel, cdiv, cfreq);
                     printDebug(msg, 2);
                  }
                  minDelay = cdel;
                  record = true;
               }
            }

            if(record) {
               maxDelay = cdel;
               validPoints++;
            }

         } else if((record) && (minDelay != -1)) { // idcode not valid after a set of valid results...
            break;
         }

      }  // end for loop (clock delay)

      if(minDelay != -1) {    // we found a valid clkdelay/clkdiv combination...

         // check if valid is greater then divisor/2
         if (validPoints >= cdiv/2) {

            AXICalibItem item;
            item.id = id++;
            item.clkDiv = cdiv;
            item.clkDelay = (minDelay == maxDelay)?minDelay:(maxDelay + minDelay) / 2;
            item.clkFreq = cfreq;
            item.validPoints = validPoints;

            if(debugLevel) {
               char msg[128];
               sprintf(msg, "AXICalibrator::startCalibration: idcode OK - clkdelay: %d - clkdiv: %d - clkfreq: %d - points: %d",
                  item.clkDelay, item.clkDiv, item.clkFreq, item.validPoints);
               printDebug(msg, 2);
            }

            setup->addItem(item);
         }

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
