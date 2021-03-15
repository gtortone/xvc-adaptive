#ifndef AXISETUP_H
#define AXISETUP_H

#include <iostream>
#include <vector>
#include <string>
#include <string.h>

class AXICalibItem {

public:
   int getId(void) { return id; };
   int getClockDivisor(void) { return clkDiv; };
   int getClockDelay(void) { return clkDelay; };
   int getClockFrequency(void) { return clkFreq; };
   int getValidPoints(void) { return validPoints; };
   int getEyeWidth(void) { return eyeWidth; };

   void setId(int v) { id = v; };
   void setClockDivisor(int v) { clkDiv = v; };
   void setClockDelay(int v) { clkDelay = v; };
   void setClockFrequency(int v) { clkFreq = v; };
   void setValidPoints(int v) { validPoints = v; };
   void setEyeWidth(int v) { eyeWidth = v; };

   void print(void) {
      std::cout << "ID: " << id << " DIV:" << clkDiv << " DLY:" << clkDelay <<
         " FREQ:" << clkFreq << " VALID:" << validPoints << " EYEW:" << eyeWidth << std::endl;
   }


private:
   int id;
   int clkDiv;
   int clkDelay;
   int clkFreq;
   int validPoints;
   int eyeWidth;
};

class AXISetup {

public:

   AXISetup();
   ~AXISetup();

   void setVerbose(bool v) { verbose = v; };
   void addItem(AXICalibItem &item);
   void clear(void);

   bool loadFile(std::string filename);
   bool saveFile(std::string filename);
   bool closeFile(void);

   void print(void);
   AXICalibItem * getItemById(int id);
   AXICalibItem * getItemByFrequency(int freq);
   AXICalibItem * getItemByMaxFrequency(void);

private:
   bool verbose = false;
   FILE *fp;
   bool fileLoaded = false;
   std::vector<AXICalibItem> calibList;
};

#endif
