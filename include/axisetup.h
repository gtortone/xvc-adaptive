#ifndef AXISETUP_H
#define AXISETUP_H

#include <vector>
#include <string>
#include <string.h>

typedef struct {
   int id;
   int clkDiv;
   int clkDelay;
   int clkFreq;
   int validPoints;
} AXICalibItem;

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

private:
   bool verbose = false;
   FILE *fp;
   bool fileLoaded = false;
   std::vector<AXICalibItem> calibList;
};

#endif
