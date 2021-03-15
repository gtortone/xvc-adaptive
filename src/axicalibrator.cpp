#include "axicalibrator.h"

AXICalibrator::AXICalibrator(AXIDevice *d) {
   dev = d;
}

void AXICalibrator::printDebug(std::string msg, int lvl) {
   if (debugLevel >= lvl)
      std::cout << msg << std::endl;
}

void AXICalibrator::start(AXISetup *setup) {

   printDebug("AXICalibrator::start start", 1);

   // reset previous calibration
   setup->clear();

   std::cout << "I: calibration started" << std::endl;

   int id = 0;       // calibration id
   int cdiv = 0;     // clock divisor
   int cdel = 0;     // clock delay
   int cfreq;        // clock frequency
   int validPoints;  // consecutive valid measurements

   int minDelay, maxDelay;
   int currIter = 0;
   int interval = 5;
   int h = 0;
   bool record = false;

   for(cdiv=MAX_CLOCK_DIV; cdiv>=0; cdiv--) {

      currIter = float(MAX_CLOCK_DIV - cdiv) / MAX_CLOCK_DIV * 100;
      if(currIter >= interval) {
         printf("\r %d%%", interval);
         fflush(stdout);
         interval += 5;
      }

      dev->setClockDiv(cdiv);

      minDelay = -1;
      maxDelay = -1;
      validPoints = 0;
      record = false;
      cfreq = 100000000 / ((cdiv + 1) * 2);
      h = 0;

      for(cdel=cdiv/2; cdel<MAX_CLOCK_DELAY; cdel++) {

         dev->setClockDelay(cdel);

         uint32_t idcode = dev->probeIdCode();

         if(idcode == dev->getIdCode()) {

            if(minDelay == -1) {

               if(++h >= hyst) {
                  if(debugLevel) {
                     char msg[128];
                     sprintf(msg, "AXICalibrator::startCalibration: idcode OK - clkdelay: %d - clkdiv: %d - clkfreq: %d", cdel, cdiv, cfreq);
                     printDebug(msg, 2);
                  }
                  validPoints = hyst;  //
                  minDelay = cdel-hyst;
                  record = true;
               }
            }

            if(record) {
               maxDelay = cdel;
               validPoints++;
            }

         } else {    // idcode != dev->getIdCode()

            h = 0;
            
            char msg[128];
            sprintf(msg, "AXICalibrator::startCalibration: idcode FAIL - clkdelay: %d - clkdiv: %d - clkfreq: %d", cdel, cdiv, cfreq);
            printDebug(msg, 2);
            
            if(record)  // idcode not valid after a set of valid results...
               break;
         }

      }  // end for loop (clock delay)

      if(record) {    // we found a valid clkdelay/clkdiv combination...

         // check if valid is greater then divisor/2
         if (validPoints >= cdiv/2) {

            AXICalibItem item;
            item.id = id++;
            item.clkDiv = cdiv;
            item.clkDelay = (maxDelay + minDelay) / 2;
            item.clkFreq = cfreq;
            item.validPoints = validPoints;
            item.eyeWidth = (validPoints / float((cdiv + 1) * 2)) * 100;

            if(debugLevel) {
               char msg[256];
               sprintf(msg, "AXICalibrator::startCalibration: idcode OK - clkdelay: %d - clkdiv: %d - clkfreq: %d - points: %d - eyewidth: %d",
                  item.clkDelay, item.clkDiv, item.clkFreq, item.validPoints, item.eyeWidth);
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
