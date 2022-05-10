#include "axisetup.h"

AXISetup::AXISetup(void) {
}

void AXISetup::addItem(AXICalibItem &item) {
   calibList.push_back(item);
}

void AXISetup::clear(void) {
   calibList.clear();
}

void AXISetup::finalize(void) {

   std::vector<AXICalibItem>::iterator it;
   int id = calibList.size() - 1;

   for(it=calibList.begin(); it!=calibList.end(); it++)
      it->setId(id--);
}

bool AXISetup::loadFile(std::string filename) {
   
   char id[64], clkDiv[64], clkDelay[64], clkFreq[64], validPoints[64], eyeWidth[64];

   fp = fopen(filename.c_str(),"rt");
   if(fp) {

      clear();
      while(!feof(fp)) {

         int i;
         char buffer[256];
         fgets(buffer,256,fp);  // get next line from file
         i = strlen(buffer);         
         while (i > 0 && isspace(buffer[i-1]))
            i--;
         buffer[i] = 0;
         if(buffer[0] == '#')
            continue;
         if (sscanf(buffer,"%64s %64s %64s %64s %64s %64s", 
                     id, clkDiv, clkDelay, clkFreq, validPoints, eyeWidth) == 6) {

            AXICalibItem item;
            item.setId(atoi(id));
            item.setClockDivisor(atoi(clkDiv));
            item.setClockDelay(atoi(clkDelay));
            item.setClockFrequency(atoi(clkFreq));
            item.setValidPoints(atoi(validPoints));
            item.setEyeWidth(atoi(eyeWidth));
            addItem(item);
         }
      }

      fclose(fp);
      return true;
   } 

   return false;
}

bool AXISetup::saveFile(std::string filename) {

   fp = fopen(filename.c_str(),"wt");
   if(fp) {

      fprintf(fp, "%-10s%10s%10s%10s%10s%10s\n", "#id", "divisor", "delay", "frequency", "valid", "eyewidth");

      std::vector<AXICalibItem>::iterator it;
      for(it=calibList.begin(); it!=calibList.end(); it++)
         fprintf(fp, "%-10d%10d%10d%10d%10d%10d\n",
               it->getId(), it->getClockDivisor(), it->getClockDelay(), it->getClockFrequency(), 
               it->getValidPoints(), it->getEyeWidth());

      fclose(fp);
      return true;
   }
   
   return false;
}

void AXISetup::print(void) {
         
   std::vector<AXICalibItem>::iterator it;

   for(it=calibList.begin(); it!=calibList.end(); it++)
      it->print();
}

AXICalibItem * AXISetup::getItemById(int id) {

   bool found = false;
   std::vector<AXICalibItem>::iterator it;

   for(it=calibList.begin(); it!=calibList.end(); it++) {
      if(id == it->getId()) {
         found = true;
         break;
      }
   }

   if(verbose) {
      if(found) {
         printf("AXISetup::getItemById id found\n");
         it->print();
      } else {
         printf("AXISetup::getItemById id not found: id:%d\n", id);
      }
   }

   return (found?(&*it):nullptr);
}

AXICalibItem * AXISetup::getItemByFrequency(int freq) {

   unsigned int i = 0;
   int index = 0;
   int delta = 0;
   bool init = false;

   for(i=0; i<calibList.size(); i++) {
      
      if (!init) {
         init = true;
         delta = abs(freq - calibList[i].getClockFrequency());
         continue;
      }

      if(delta > abs(freq - calibList[i].getClockFrequency())) {
         delta = abs(freq - calibList[i].getClockFrequency());
         index = i;
      }
   }

   if(verbose) {
      printf("AXISetup::getItemByFrequency id found req(%d) delta(%d)\n", freq, delta);
      calibList[index].print();
   }

   return &calibList[index];
}

AXICalibItem * AXISetup::getItemByMaxFrequency(void) {

   unsigned int i = 0;
   int maxfreq = 0;
   int index = 0;

   for(i=0; i<calibList.size(); i++) {

      if(calibList[i].getClockFrequency() > maxfreq) {
         maxfreq = calibList[i].getClockFrequency();
         index = i;
      }
   }

   if(verbose) {
      printf("AXISetup::getItemByMaxFrequency found\n");
      calibList[index].print();
   }

   return &calibList[index];
}
