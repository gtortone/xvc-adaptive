#include <iostream>
#include <iomanip>
#include <memory>
#include <unistd.h>

#include "argparse.h"
#include "ioserver.h"
#include "axidevice.h"
#include "ftdidevice.h"
#include "axicalibrator.h"
#include "axisetup.h"

int main(int argc, const char **argv) {

   std::unique_ptr<XVCDriver> dev;
   bool verbose = false;
   int debugLevel = 0;
   int port = 2542;
   bool scan = false;
   const char *saveFilename = NULL;
   const char *loadFilename = NULL;
   int cdiv = -1;
   int cdel = -1;
   bool quickSetup = false;
   const char *driverName = "AXI";
   AXISetup *setup = new AXISetup();
   int id = -1;
   int freq = -1;

   static const char *const usage[] = {
      "xvcserver [options]",
      NULL,
   };

   struct argparse_option options[] = {
      OPT_HELP(),
      OPT_GROUP("Basic options"),
      OPT_BOOLEAN('v', "verbose", &verbose, "enable verbose"),
      OPT_INTEGER('d', "debug", &debugLevel, "set debug level (default: 0)"),
      OPT_GROUP("Network options"),
      OPT_INTEGER('p', "port", &port, "set server port (default: 2542)"),
      OPT_GROUP("Driver options"),
      OPT_STRING(0, "driver", &driverName, "set driver name [AXI,FTDI] (default: AXI)", NULL, 0, 0),
      OPT_BOOLEAN(0, "scan", &scan, "scan for connected device and exit"),
      OPT_GROUP("AXI Calibration options"),
      OPT_STRING('s', "savecalib", &saveFilename, "start calibration and save data to file [AXI driver]", NULL, 0, 0),
      OPT_STRING('l', "loadcalib", &loadFilename, "load calibration data from file [AXI driver]", NULL, 0, 0),
      OPT_INTEGER(0, "id", &id, "load calibration entry from file by id [AXI driver]"),
      OPT_INTEGER(0, "freq", &freq, "load calibration entry from file by clock frequency [AXI driver]"),
      OPT_GROUP("AXI Setup options"),
      OPT_INTEGER(0, "cdiv", &cdiv, "set clock divisor (0:1023) [AXI driver]", NULL, 0, 0),
      OPT_INTEGER(0, "cdel", &cdel, "set clock delay (0:255) [AXI driver]", NULL, 0, 0),
      OPT_END(),
   };

   struct argparse argparse;
   argparse_init(&argparse, options, usage, 0);
   argparse_describe(&argparse, "\nXilinx Virtual Cable (XVC) adaptive server", "");
   argparse_parse(&argparse, argc, argv);

   if(std::string(driverName) == "AXI") {
      try {
         dev.reset(new AXIDevice());
      } catch (const std::exception& e) {
         std::cout << e.what() << std::endl;
         exit(-1);
      }
   } else if(std::string(driverName) == "FTDI") {
      //try {
      //   dev.reset(new FTDIDevice());
      //} catch (const std::exception& e) {
         std::cout << "E: driver FTDI not supported yet" << std::endl;
         exit(-1);
      //}
   } else {
      std::cout << "E: driver " << driverName << " not found" << std::endl;
      exit(-1);
   }
   
   // apply command line parameters
   dev.get()->setDebugLevel(debugLevel);
   dev.get()->setVerbose(verbose);
   setup->setVerbose(verbose);
   
   std::cout << "I: using driver " << driverName << std::endl;
   std::cout << "I: device detected: " << dev.get()->getDescription() << 
      " idcode: 0x" << std::hex << dev.get()->getIdCode() << std::dec <<
      " irlen: " << dev.get()->getIrLen() <<
      " idcmd: 0x" << std::hex << dev.get()->getIdCmd() << std::dec << std::endl;

   if(scan)
      exit(0);

   if(saveFilename) {
      if(dev.get()->getName() == "AXI") {
         std::cout << "I: start calibration task" << std::endl;
         AXICalibrator *calib = new AXICalibrator((AXIDevice *) dev.get());
         calib->setDebugLevel(debugLevel);
         calib->setVerbose(verbose);
         calib->start(setup);
         // save calibration data to file
         setup->saveFile(saveFilename);
         std::cout << "I: calibration data saved in " << saveFilename << std::endl; 
         // calibration does not start XVC server
         exit(0);
      } else std::cout << "E: calibration not supported by driver " << driverName << std::endl;
   }
 
   if(cdiv != -1) {
      if(dev.get()->getName() == "AXI") {
         quickSetup = true;
         std::cout << "I: apply clock divisor " << cdiv << std::endl;
         AXIDevice *adev = (AXIDevice *) dev.get();
         adev->setClockDiv(cdiv);
      } else std::cout << "E: clock divisor parameter not supported by driver " << driverName << std::endl;
   }

	if(cdel != -1) {
      if(dev.get()->getName() == "AXI") {
         quickSetup = true;
         std::cout << "I: apply clock delay " << cdel << std::endl;
         AXIDevice *adev = (AXIDevice *) dev.get();
         adev->setClockDelay(cdel);
      } else std::cout << "E: clock delay parameter not supported by driver " << driverName << std::endl;
   }

	if(quickSetup && dev.get()->getName() == "AXI") {
		std::cout << "I: manual setup used - skip other setup options" << std::endl;
		goto startServer; 
	}

   if(loadFilename) {

      if(dev.get()->getName() != "AXI") {
         std::cout << "E: setup options only available for AXI device" << std::endl;
         exit(-1);
      }

      setup->clear();
      bool ret = setup->loadFile(loadFilename);
      if(ret) {

         AXICalibItem *item;

         std::cout << "I: file " << loadFilename << " loaded successfully" << std::endl;
         
         // check for id command line options
         if(id != -1) {
         
            item = setup->getItemById(id);
            if(item != nullptr) {
               AXIDevice *adev = (AXIDevice *) dev.get();
               adev->setClockDelay(item->clkDelay);
               adev->setClockDiv(item->clkDiv);
               std::cout << "I: AXI setup with id " << id << " successfully (DIV:" << item->clkDiv << 
                  " DLY:" << item->clkDelay << ")" << std::endl;
            } else std::cout << "E: setup item with id " << id << " not found" << std::endl;
            goto startServer;
         }
         
         // check for freq command line options
         if(freq != -1)
            item = setup->getItemByFrequency(freq);
         else  // select max frequency
            item = setup->getItemByMaxFrequency();

         AXIDevice *adev = (AXIDevice *) dev.get();
         adev->setClockDelay(item->clkDelay);
         adev->setClockDiv(item->clkDiv);
         std::cout << "I: AXI setup with id " << item->id << " successfully (DIV:" << item->clkDiv << 
               " DLY:" << item->clkDelay << " FREQ:" << item->clkFreq << ")" << std::endl;

      } else { 
         std::cout << "E: file " << loadFilename << " loading error" << std::endl;
         exit(-1);
      }
   }

startServer:

   IOServer *srv = new IOServer(dev.get());
   srv->setVerbose(verbose);

   std::cout << "I: using TCP port " << port << std::endl;
   srv->setPort(port);

   try {
      std::cout << "I: starting XVC server..." << std::endl;
      srv->start();
   } catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
   }

   return 0;
}
