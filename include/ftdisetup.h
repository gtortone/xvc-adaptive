#ifndef FTDISETUP_H
#define FTDISETUP_H

#include <vector>
#include <string>
#include <string.h>

typedef struct {
   int id;
   int clkDiv;
   int clkFreq;
} FTDICalibItem;

class FTDISetup {

public:

   FTDISetup();
   ~FTDISetup();

   void setVerbose(bool v) { verbose = v; };
   void addItem(FTDICalibItem &item);
   void clear(void);

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
