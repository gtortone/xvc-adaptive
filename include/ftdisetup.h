#ifndef FTDISETUP_H
#define FTDISETUP_H

#include <iostream>
#include <vector>
#include <string>
#include <string.h>

class FTDICalibItem {

public:
	int getId(void) { return id; };
   int getClockDivisor(void) { return clkDiv; };
	bool getTDOSampling(void) { return tdoSam; };
   int getClockFrequency(void) { return clkFreq; };

   void setId(int v) { id = v; };
   void setClockDivisor(int v) { clkDiv = v; };
	void setTDOSampling(bool v) { tdoSam = v; };
   void setClockFrequency(int v) { clkFreq = v; };

   void print(void) {
      std::cout << "ID: " << id << " DIV:" << clkDiv << " TDOSAM:" << tdoSam <<
         " FREQ:" << clkFreq << std::endl;
   }


private:
   int id;
   int clkDiv;
	bool tdoSam;
   int clkFreq;
};

class FTDISetup {

public:

   FTDISetup();
   ~FTDISetup();

   void setVerbose(bool v) { verbose = v; };
   void addItem(FTDICalibItem &item);
   void clear(void);
   void finalize(void);

   bool loadFile(std::string filename);
   bool saveFile(std::string filename);
   bool closeFile(void);

   void print(void);
   FTDICalibItem * getItemById(int id);
   FTDICalibItem * getItemByFrequency(int freq);
   FTDICalibItem * getItemByMaxFrequency(void);

private:
   bool verbose = false;
   FILE *fp;
   bool fileLoaded = false;
   std::vector<FTDICalibItem> calibList;
};

#endif
