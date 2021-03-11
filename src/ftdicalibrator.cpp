#include "ftdicalibrator.h"

FTDICalibrator::FTDICalibrator(FTDIDevice *d) {
   dev = d;
}

void FTDICalibrator::printDebug(std::string msg, int lvl) {
   if (debugLevel >= lvl)
      std::cout << msg << std::endl;
}

void FTDICalibrator::start(FTDISetup *setup, int minFreq, int maxFreq) {

   printDebug("FTDICalibrator::start start", 1);

   int id = 0;       // calibration id
   int cdiv = 0;     // clock divisor
   int cfreq = 0;    // clock frequency
   int currIter = 0;
   int lastIter = 0;

   // reset previous calibration
   setup->clear();

   std::cout << "I: calibration started" << std::endl;

   int minVal = dev->getDivisorByFrequency(DIV5_OFF, maxFreq);
   int maxVal = dev->getDivisorByFrequency(DIV5_OFF, minFreq);
  
   for(cdiv=maxVal; cdiv>=minVal; cdiv--) {

      currIter = float(maxVal-cdiv) / (maxVal-minVal) * 100;
      if(currIter != lastIter) {    
         printf("\r %d%%", currIter);
         fflush(stdout);
         lastIter = currIter;
      }

      dev->setClockDiv(false, cdiv);
      cfreq = dev->getFrequencyByDivisor(DIV5_OFF, cdiv);

      uint32_t idcode = dev->probeIdCode();

      if(idcode == dev->getIdCode()) {

         FTDICalibItem item;
         item.id = ++id;
         item.clkDiv = cdiv;
         item.clkFreq = cfreq;

         if(debugLevel) {
            char msg[128];
            sprintf(msg, "FTDICalibrator::startCalibration: idcode OK - clkdiv: %d - clkfreq: %d", cdiv, cfreq);
            printDebug(msg, 2);
         }

         setup->addItem(item);

      } else {

         char msg[128];
         sprintf(msg, "FTDICalibrator::startCalibration: idcode FAIL - clkdiv: %d - clkfreq: %d", cdiv, cfreq);
         printDebug(msg, 2);
      }
   }

   std::cout << std::endl;
   printDebug("FTDICalibrator::start end", 1);
   std::cout << "I: calibration finished" << std::endl;
}
