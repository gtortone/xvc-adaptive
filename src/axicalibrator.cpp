#include "axicalibrator.h"
#include <ctime>

AXICalibrator::AXICalibrator(AXIDevice *d) {
   dev = d;
}

void AXICalibrator::printDebug(std::string msg, int lvl) {
   if (debugLevel >= lvl)
      std::cout << msg << std::endl;
}

void AXICalibrator::start(AXISetup *setup, unsigned int calibSize) {

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
   unsigned int numEntry = 0;

   std::srand(std::time(NULL));

   // phase 1 : detect working frequency writing/reading a random value in bypass mode

   for(cdiv=0; cdiv<=MAX_CLOCK_DIV; cdiv++) {

      currIter = float(cdiv) / MAX_CLOCK_DIV * 100;
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

         uint32_t probeValue = std::rand();

         if(dev->probeBypass(probeValue) == probeValue) {

            if(minDelay == -1) {

               if(++h >= hyst) {
                  if(debugLevel) {
                     char msg[128];
                     sprintf(msg, "AXICalibrator::startCalibration: idcode OK - clkdelay: %d - clkdiv: %d - clkfreq: %d", cdel, cdiv, cfreq);
                     printDebug(msg, 2);
                  }
                  validPoints = hyst;
                  minDelay = cdel-hyst;
                  record = true;
               }
            }

            if(record) {
               maxDelay = cdel;
               validPoints++;
            }

         } else {    // dev->probeBypass(probeValue) != probeValue

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
            item.setId(id++);
            item.setClockDivisor(cdiv);
            item.setClockDelay((maxDelay + minDelay) / 2);
            item.setClockFrequency(cfreq);
            item.setValidPoints(validPoints);
            item.setEyeWidth((validPoints / float((cdiv + 1) * 2)) * 100);

            if(debugLevel) {
               char msg[256];
               sprintf(msg, "AXICalibrator::startCalibration: idcode OK - clkdelay: %d - clkdiv: %d - clkfreq: %d - points: %d - eyewidth: %d",
                  item.getClockDelay(), item.getClockDivisor(), item.getClockFrequency(), item.getValidPoints(), item.getEyeWidth());
               printDebug(msg, 2);
            }

            setup->addItem(item);

            if(calibSize && ++numEntry == calibSize) {
               printf("\nI: quick mode enabled - calibration stopped after %d valid measurements\n", calibSize);
               goto phase2; 
            }
               
         }

      } 

   } // end for loop (clock divisor)

   // phase 2 : hardening test on high frequency with some bits patterns

phase2:

   int nsample = 32;
   int npass = 100;
   const uint32_t ones = 0x80000000;
   std::vector<uint32_t> rdbuf;
   std::vector<int> badList;
   bool goodItem;

   // build test data buffers
   std::vector<uint32_t> wzbuf, gzbuf, wobuf;
   for(int i=0; i<nsample; i++) {
      wzbuf.push_back(~(ones >> i));
      gzbuf.push_back(~ones >> i);
      wobuf.push_back(ones >> i);
   }

   badList.clear();
   goodItem = false;

   for(int index=0; index<setup->getListSize() && !goodItem; index++) {

      AXICalibItem *item = setup->getItemByIndex(index);
      
      goodItem = true;

      // apply config to FPGA
      dev->setClockDiv(item->getClockDivisor());       
      dev->setClockDelay(item->getClockDelay());       

      //printf("start walking zero test\n");
      for(int i=0,pass=0; (goodItem && pass<npass); i++, pass++) {
         rdbuf = dev->probeBypass(wzbuf);
         if (rdbuf != wzbuf) {
            /*
            printf("pass: %d , freq = %d cdiv/cdel = %d/%d walking zero test failure \n", \
               pass, item->getClockFrequency(), item->getClockDivisor(), item->getClockDelay()); 
            */
            badList.push_back(item->getId());
            goodItem = false;
         }
      } 

      //printf("start growing zero test\n");
      for(int i=0,pass=0; (goodItem && pass<npass); i++, pass++) {
         rdbuf = dev->probeBypass(gzbuf);
         if (rdbuf != gzbuf) {
            /*
            printf("pass: %d , freq = %d cdiv/cdel = %d/%d growing zero test failure \n", \
               pass, item->getClockFrequency(), item->getClockDivisor(), item->getClockDelay()); 
            */
            badList.push_back(item->getId());
            goodItem = false;
         }
      } 

      //printf("start walking one test\n");
      for(int i=0,pass=0; (goodItem && pass<npass); i++, pass++) {
         rdbuf = dev->probeBypass(wobuf);
         if (rdbuf != wobuf) {
            /*
            printf("pass: %d , freq = %d cdiv/cdel = %d/%d walking one test failure \n", \
               pass, item->getClockFrequency(), item->getClockDivisor(), item->getClockDelay()); 
            */
            badList.push_back(item->getId());
            goodItem = false;
         }
      } 

   }

   for(unsigned int i=0; i<badList.size(); i++)
      setup->delItemById(badList[i]);

   setup->finalize();

   printDebug("AXICalibrator::start end", 1);
   std::cout << "\nI: calibration finished" << std::endl;
}
