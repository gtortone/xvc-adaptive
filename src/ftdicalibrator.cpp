#include "ftdicalibrator.h"

FTDICalibrator::FTDICalibrator(FTDIDevice *d) {
   dev = d;
}

void FTDICalibrator::printDebug(std::string msg, int lvl) {
   if (debugLevel >= lvl)
      std::cout << msg << std::endl;
}

void FTDICalibrator::start(FTDISetup *setup, int minFreq, int maxFreq, int loop) {

   printDebug("FTDICalibrator::start start", 1);

   int id = 0;             // calibration id
   int cdiv = 0;           // clock divisor
   int tdoSampling = 0;    // TDO sampling edge (0: negative, 1: positive)
   int cfreq = 0;          // clock frequency
   int currIter = 0;
   int lastIter = 0;

   // reset previous calibration
   setup->clear();

   std::cout << "I: calibration started" << std::endl;

   int minVal = dev->getDivisorByFrequency(DIV5_OFF, maxFreq);
   int maxVal = dev->getDivisorByFrequency(DIV5_OFF, minFreq);
  
   for(cdiv=maxVal; cdiv>=minVal; cdiv--) {

      for(tdoSampling=0; tdoSampling<=1; tdoSampling++) {

         currIter = float(maxVal-cdiv) / (maxVal-minVal) * 100;
         if(currIter != lastIter) {    
            printf("\r %d%%", currIter);
            fflush(stdout);
            lastIter = currIter;
         }

         dev->setTDOPosSampling((bool)tdoSampling);
         dev->setClockDiv(false, cdiv);
         cfreq = dev->getFrequencyByDivisor(DIV5_OFF, cdiv);

         int match = 0;
         for(int i=0; i<loop; i++) {
            uint32_t idcode = dev->probeIdCode();
            if(idcode == dev->getIdCode())
               match++;
            else
               break;
         }

         if(match == loop) {

            FTDICalibItem item;
            item.setId(++id);
            item.setClockDivisor(cdiv);
            item.setTDOSampling(tdoSampling);
            item.setClockFrequency(cfreq);

            if(debugLevel) {
               char msg[128];
               sprintf(msg, "FTDICalibrator::startCalibration: idcode OK - tdoSampling: %d - clkdiv: %d - clkfreq: %d", tdoSampling, cdiv, cfreq);
               printDebug(msg, 2);
            }

            setup->addItem(item);

         } else {

            char msg[128];
            sprintf(msg, "FTDICalibrator::startCalibration: idcode FAIL - tdoSampling: %d - clkdiv: %d - clkfreq: %d", tdoSampling, cdiv, cfreq);
            printDebug(msg, 2);
         }
      }
   }

   setup->finalize();

   std::cout << std::endl;
   printDebug("FTDICalibrator::start end", 1);
   std::cout << "I: calibration finished" << std::endl;
}
