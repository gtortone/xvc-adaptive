#include "axisetup.h"

AXISetup::AXISetup(void) {
}

void AXISetup::addItem(AXICalibItem &item) {
   calibList.push_back(item);
}

void AXISetup::clear(void) {
   calibList.clear();
}

bool AXISetup::loadFile(std::string filename) {
   
   char id[64], clkDiv[64], clkDelay[64], clkFreq[64], validPoints[64];

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
         if (sscanf(buffer,"%64s %64s %64s %64s %64s", 
                     id, clkDiv, clkDelay, clkFreq, validPoints) == 5) {

            AXICalibItem item;
            item.id = atoi(id);
            item.clkDiv = atoi(clkDiv);
            item.clkDelay = atoi(clkDelay);
            item.clkFreq = atoi(clkFreq);
            item.validPoints = atoi(validPoints);
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

      fprintf(fp, "%-10s%10s%10s%10s%10s\n", "#id", "divisor", "delay", "frequency", "valid");

      std::vector<AXICalibItem>::iterator it;
      for(it=calibList.begin(); it!=calibList.end(); it++)
         fprintf(fp, "%-10d%10d%10d%10d%10d\n",
               it->id, it->clkDiv, it->clkDelay, it->clkFreq, it->validPoints);

      fclose(fp);
      return true;
   }
   
   return false;
}

void AXISetup::print(void) {
         
   std::vector<AXICalibItem>::iterator it;

   for(it=calibList.begin(); it!=calibList.end(); it++)
      printf("id:%d DIV:%d DLY:%d CLK:%d VALID:%d\n", 
            it->id, it->clkDiv, it->clkDelay, it->clkFreq, it->validPoints);
}

AXICalibItem * AXISetup::getItemById(int id) {

   bool found = false;
   std::vector<AXICalibItem>::iterator it;

   for(it=calibList.begin(); it!=calibList.end(); it++) {
      if(it->id == id) {
         found = true;
         break;
      }
   }

   if(verbose) {
      if(found)
         printf("AXISetup::getItemById id found: id:%d DIV:%d DLY:%d CLK:%d\n", 
               it->id, it->clkDiv, it->clkDelay, it->clkFreq);
      else {
         printf("AXISetup::getItemById id not found: id:%d\n", id);
      }
   }

   return (found?(&*it):nullptr);
}

AXICalibItem * AXISetup::getItemByFrequency(int freq) {

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
      printf("AXISetup::getItemByFrequency id found req(%d) delta(%d): id:%d DIV:%d DLY:%d CLK:%d\n", 
            freq, delta, id, calibList[index].clkDiv, calibList[index].clkDelay, calibList[index].clkFreq);

   return &calibList[index];
}

