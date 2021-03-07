#include <iostream>
#include <iomanip>
#include <memory>
#include <unistd.h>

#include "argparse.h"
#include "ioserver.h"
#include "axidevice.h"
#include "devicedb.h"

int main(int argc, const char **argv) {

   std::unique_ptr<XVCDriver> dev;
   bool verbose = false;
   int debugLevel = 0;
   int port = 2542;
   const char *saveFilename = NULL;
   const char *loadFilename = NULL;
   int cdiv = -1;
   int cdel = -1;
   bool quickSetup = false;

   static const char *const usage[] = {
      "xvcserver [options]",
      NULL,
   };

   struct argparse_option options[] = {
      OPT_HELP(),
      OPT_GROUP("Basic options"),
      OPT_BOOLEAN('v', "verbose", &verbose, "enable verbose"),
      OPT_INTEGER('d', "debug", &debugLevel, "set debug level (default: 0)"),
      OPT_GROUP("Calibration options"),
      OPT_STRING('s', "savecalib", &saveFilename, "start calibration and save data to file", NULL, 0, 0),
      OPT_STRING('l', "loadcalib", &loadFilename, "load calibration data from file", NULL, 0, 0),
      OPT_GROUP("Setup options"),
      OPT_INTEGER(0, "cdiv", &cdiv, "set clock divisor [0:1023]", NULL, 0, 0),
      OPT_INTEGER(0, "cdel", &cdel, "set clock delay [0:255]", NULL, 0, 0),
      OPT_GROUP("Network options"),
      OPT_INTEGER('p', "port", &port, "set server port (default: 2542)"),
      OPT_END(),
   };

   struct argparse argparse;
   argparse_init(&argparse, options, usage, 0);
   argparse_describe(&argparse, "\nXilinx Virtual Cable (XVC) adaptive server", "");
   argparse_parse(&argparse, argc, argv);

   dev.reset(new AXIDevice());
   IOServer *srv = new IOServer(dev.get());

   // apply command line parameters
   dev.get()->setDebugLevel(debugLevel);
   dev.get()->setVerbose(verbose);
   srv->setVerbose(verbose);

   if(saveFilename) {
      std::cout << "I: start calibration task" << std::endl;
      // calibration does not start XVC server
      dev.get()->startCalibration();
      // save calibration data to file
      std::cout << "I: calibration data saved in " << saveFilename << std::endl; 
      exit(0);
   }
 
   if(cdiv != -1) {
      if(cdiv <= MAX_CLOCK_DIV) {
         quickSetup = true;
         std::cout << "I: apply clock divisor " << cdiv << std::endl;
         dev.get()->setClockDiv(cdiv);
      } else {
         std::cout << "E: clock divisor out of range: " << cdiv << std::endl;
         exit(-1);
      }
   }

	if(cdel != -1) {
      if(cdel <= MAX_CLOCK_DELAY) {
         quickSetup = true;
         std::cout << "I: apply clock delay " << cdel << std::endl;
         dev.get()->setDelay(cdel);
      } else {
         std::cout << "E: clock delay out of range: " << cdel << std::endl;
         exit(-1);
      }
   }

	if(quickSetup) {
		std::cout << "I: manual setup used - skip other setup options" << std::endl;
		goto startServer; 
	}

   if(loadFilename) {
      //
   }

startServer:
   std::cout << "I: using TCP port " << port << std::endl;
   srv->setPort(port);

   std::cout << "I: XVC server started" << std::endl;
   srv->start();

   return 0;
}
