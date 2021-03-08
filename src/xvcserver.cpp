#include <iostream>
#include <iomanip>
#include <memory>
#include <unistd.h>

#include "argparse.h"
#include "ioserver.h"
#include "axidevice.h"
#include "ftdidevice.h"
#include "axicalibrator.h"

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
   const char *driverName = "AXI";

   static const char *const usage[] = {
      "xvcserver [options]",
      NULL,
   };

   struct argparse_option options[] = {
      OPT_HELP(),
      OPT_GROUP("Basic options"),
      OPT_BOOLEAN('v', "verbose", &verbose, "enable verbose"),
      OPT_INTEGER('d', "debug", &debugLevel, "set debug level (default: 0)"),
      OPT_GROUP("Driver options"),
      OPT_STRING(0, "driver", &driverName, "set driver name [AXI,FTDI] (default: AXI)", NULL, 0, 0),
      OPT_GROUP("Calibration options"),
      OPT_STRING('s', "savecalib", &saveFilename, "start calibration and save data to file [AXI driver]", NULL, 0, 0),
      OPT_STRING('l', "loadcalib", &loadFilename, "load calibration data from file [AXI driver]", NULL, 0, 0),
      OPT_GROUP("Setup options"),
      OPT_INTEGER(0, "cdiv", &cdiv, "set clock divisor (0:1023) [AXI driver]", NULL, 0, 0),
      OPT_INTEGER(0, "cdel", &cdel, "set clock delay (0:255) [AXI driver]", NULL, 0, 0),
      OPT_GROUP("Network options"),
      OPT_INTEGER('p', "port", &port, "set server port (default: 2542)"),
      OPT_END(),
   };

   struct argparse argparse;
   argparse_init(&argparse, options, usage, 0);
   argparse_describe(&argparse, "\nXilinx Virtual Cable (XVC) adaptive server", "");
   argparse_parse(&argparse, argc, argv);

   if(std::string(driverName) == "AXI") {
      dev.reset(new AXIDevice());
   } else if(std::string(driverName) == "FTDI") {
      dev.reset(new FTDIDevice());
      std::cout << "E: driver FTDI not supported yet" << std::endl;
      exit(-1);
   } else {
      std::cout << "E: driver " << driverName << " not found" << std::endl;
      exit(-1);
   }
   
   std::cout << "I: using driver " << driverName << std::endl;
   IOServer *srv = new IOServer(dev.get());

   // apply command line parameters
   dev.get()->setDebugLevel(debugLevel);
   dev.get()->setVerbose(verbose);
   srv->setVerbose(verbose);

   if(saveFilename) {
      if(dev.get()->getName() == "AXI") {
         std::cout << "I: start calibration task" << std::endl;
         // calibration does not start XVC server
         AXICalibrator *calib = new AXICalibrator((AXIDevice *) dev.get());
         calib->setDebugLevel(debugLevel);
         calib->setVerbose(verbose);
         calib->start();
         // save calibration data to file
         std::cout << "I: calibration data saved in " << saveFilename << std::endl; 
         exit(0);
      } else std::cout << "E: calibration not supported by driver " << driverName << std::endl;
   }
 
   if(cdiv != -1) {
      if(dev.get()->getName() == "AXI") {
         if(cdiv <= MAX_CLOCK_DIV) {
            quickSetup = true;
            std::cout << "I: apply clock divisor " << cdiv << std::endl;
            AXIDevice *adev = (AXIDevice *) dev.get();
            adev->setClockDiv(cdiv);
         } else {
            std::cout << "E: clock divisor out of range: " << cdiv << std::endl;
            exit(-1);
         }
      } else std::cout << "E: clock divisor parameter not supported by driver " << driverName << std::endl;
   }

	if(cdel != -1) {
      if(dev.get()->getName() == "AXI") {
         if(cdel <= MAX_CLOCK_DELAY) {
            quickSetup = true;
            std::cout << "I: apply clock delay " << cdel << std::endl;
            AXIDevice *adev = (AXIDevice *) dev.get();
            adev->setClockDelay(cdel);
         } else {
            std::cout << "E: clock delay out of range: " << cdel << std::endl;
            exit(-1);
         }
      } else std::cout << "E: clock delay parameter not supported by driver " << driverName << std::endl;
   }

	if(quickSetup && dev.get()->getName() == "AXI") {
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
