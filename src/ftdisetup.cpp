#include "ftdisetup.h"

FTDISetup::FTDISetup(void) {
}

void FTDISetup::addItem(FTDICalibItem &item) {
   calibList.push_back(item);
}

void FTDISetup::clear(void) {
   calibList.clear();
}

void FTDISetup::finalize(void) {

   std::vector<FTDICalibItem>::iterator it;
   int id = calibList.size() - 1;

   for(it=calibList.begin(); it!=calibList.end(); it++)
      it->setId(id--);
}

bool FTDISetup::loadFile(std::string filename) {
   
   char id[64], clkDiv[64], tdoSam[64], clkFreq[64];

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
         if (sscanf(buffer,"%64s %64s %64s %64s", 
                     id, clkDiv, tdoSam, clkFreq) == 4) {

            FTDICalibItem item;
            item.setId(atoi(id));
            item.setClockDivisor(atoi(clkDiv));
            item.setTDOSampling(atoi(tdoSam));
            item.setClockFrequency(atoi(clkFreq));
            addItem(item);
         }
      }

      fclose(fp);
      return true;
   } 

   return false;
}

bool FTDISetup::saveFile(std::string filename) {

   fp = fopen(filename.c_str(),"wt");
   if(fp) {

      fprintf(fp, "%-10s%10s%10s%10s\n", "#id", "divisor", "pedge", "frequency");

      std::vector<FTDICalibItem>::iterator it;
      for(it=calibList.begin(); it!=calibList.end(); it++)
         fprintf(fp, "%-10d%10d%10d%10d\n",
               it->getId(), it->getClockDivisor(), it->getTDOSampling(), it->getClockFrequency());

      fclose(fp);
      return true;
   }
   
   return false;
}

void FTDISetup::print(void) {
         
   std::vector<FTDICalibItem>::iterator it;

   for(it=calibList.begin(); it!=calibList.end(); it++)
      it->print();
}

FTDICalibItem * FTDISetup::getItemById(int id) {

   bool found = false;
   std::vector<FTDICalibItem>::iterator it;

   for(it=calibList.begin(); it!=calibList.end(); it++) {
      if(id == it->getId()) {
         found = true;
         break;
      }
   }

   if(verbose) {
      if(found) {
         printf("FTDISetup::getItemById id found\n"); 
         it->print();
      } else {
         printf("FTDISetup::getItemById id not found: id:%d\n", id);
      }
   }

   return (found?(&*it):nullptr);
}

FTDICalibItem * FTDISetup::getItemByFrequency(int freq) {

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
      printf("FTDISetup::getItemByFrequency id found req(%d) delta(%d)\n", freq, delta);
      calibList[index].print();
   }

   return &calibList[index];
}

FTDICalibItem * FTDISetup::getItemByMaxFrequency(void) {

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
      printf("FTDISetup::getItemByMaxFrequency found\n");
      calibList[index].print();
   }

   return &calibList[index];
}
