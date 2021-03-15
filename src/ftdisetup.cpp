#include "ftdisetup.h"

FTDISetup::FTDISetup(void) {
}

void FTDISetup::addItem(FTDICalibItem &item) {
   calibList.push_back(item);
}

void FTDISetup::clear(void) {
   calibList.clear();
}

bool FTDISetup::loadFile(std::string filename) {
   
   char id[64], clkDiv[64], tdoSam[64], clkFreq[64];

   fp = fopen(filename.c_str(),"rt");
   if(fp) {

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
            item.id = atoi(id);
            item.clkDiv = atoi(clkDiv);
            item.tdoSam = atoi(tdoSam);
            item.clkFreq = atoi(clkFreq);
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
               it->id, it->clkDiv, it->tdoSam, it->clkFreq);

      fclose(fp);
      return true;
   }
   
   return false;
}

void FTDISetup::print(void) {
         
   std::vector<FTDICalibItem>::iterator it;

   for(it=calibList.begin(); it!=calibList.end(); it++)
      printf("id:%d DIV:%d SAM:%d CLK:%d\n", 
            it->id, it->clkDiv, it->tdoSam, it->clkFreq);
}

FTDICalibItem * FTDISetup::getItemById(int id) {

   bool found = false;
   std::vector<FTDICalibItem>::iterator it;

   for(it=calibList.begin(); it!=calibList.end(); it++) {
      if(it->id == id) {
         found = true;
         break;
      }
   }

   if(verbose) {
      if(found)
         printf("FTDISetup::getItemById id found: id:%d DIV:%d SAM:%d CLK:%d\n", 
               it->id, it->clkDiv, it->tdoSam, it->clkFreq);
      else {
         printf("FTDISetup::getItemById id not found: id:%d\n", id);
      }
   }

   return (found?(&*it):nullptr);
}

FTDICalibItem * FTDISetup::getItemByFrequency(int freq) {

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

   if(verbose) 
      printf("FTDISetup::getItemByFrequency id found req(%d) delta(%d): id:%d DIV:%d SAM:%d CLK:%d\n", 
            freq, delta, id, calibList[index].clkDiv, calibList[index].tdoSam, calibList[index].clkFreq);

   return &calibList[index];
}

FTDICalibItem * FTDISetup::getItemByMaxFrequency(void) {

   unsigned int i = 0;
   int id = 0;
   int maxfreq = 0;
   int index = 0;

   for(i=0; i<calibList.size(); i++) {

      if(calibList[i].clkFreq > maxfreq) {
         maxfreq = calibList[i].clkFreq; 
         id = calibList[i].id;
         index = i;
      }
   }

   if(verbose)
      printf("FTDISetup::getItemByMaxFrequency found: id:%d DIV:%d SAM:%d CLK:%d\n",
         id, calibList[index].clkDiv, calibList[index].tdoSam, calibList[index].clkFreq);

   return &calibList[index];
}
