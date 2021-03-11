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
#include "ftdicalibrator.h"
#include "ftdisetup.h"

int main(int argc, const char **argv) {

   std::unique_ptr<XVCDriver> dev;
   bool verbose = false;
   int debugLevel = 0;
   int port = 2542;
   bool scan = false;
   int hyst = 0;
   const char *saveFilename = NULL;
   const char *loadFilename = NULL;
   int cdiv = -1;
   int cdel = -1;
   bool quickSetup = false;
   const char *driverName = "AXI";
   int id = -1;
   int freq = -1;
   int vid = DEFAULT_VID;
   int pid = DEFAULT_PID;
   enum ftdi_interface interface = INTERFACE_A;
   int cfreq = -1;
   int minfreq = 100000;      // 100 kHz
   int maxfreq = 30000000;    // 30 MHz

   static const char *const usage[] = {
      "xvcserver [options]",
      NULL,
   };

   struct argparse_option options[] = {
      OPT_HELP(),
      OPT_GROUP("Basic options"),
      OPT_BOOLEAN('v', "verbose", &verbose, "enable verbose"),
      OPT_INTEGER('d', "debug", &debugLevel, "set debug level (default: 0)"),
      OPT_STRING(0, "driver", &driverName, "set driver name [AXI,FTDI] (default: AXI)", NULL, 0, 0),
      OPT_BOOLEAN(0, "scan", &scan, "scan for connected device and exit"),
      OPT_GROUP("Network options"),
      OPT_INTEGER('p', "port", &port, "set server port (default: 2542)"),
      OPT_GROUP("Calibration options"),
      OPT_STRING('s', "savecalib", &saveFilename, "start calibration and save data to file", NULL, 0, 0),
      OPT_STRING('l', "loadcalib", &loadFilename, "load calibration data from file", NULL, 0, 0),
      OPT_INTEGER(0, "id", &id, "load calibration entry from file by id"),
      OPT_INTEGER(0, "freq", &freq, "load calibration entry from file by clock frequency"),
      OPT_GROUP("AXI Calibration options"),
      OPT_INTEGER(0, "hyst", &hyst, "set hysteresis value (default: 0)"),
      OPT_GROUP("AXI Quick Setup options"),
      OPT_INTEGER(0, "cdiv", &cdiv, "set clock divisor (0:1023) [AXI driver]", NULL, 0, 0),
      OPT_INTEGER(0, "cdel", &cdel, "set capture delay (0:255) [AXI driver]", NULL, 0, 0),
      OPT_GROUP("FTDI options"),
      OPT_INTEGER(0, "vid", &vid, "set FTDI device Vendor ID (default: 0x0403)", NULL, 0, 0),
      OPT_INTEGER(0, "pid", &pid, "set FTDI device Product ID (default: 0x6010)", NULL, 0, 0),
      OPT_INTEGER(0, "interface", &interface, "set FTDI device JTAG interface (default: 1)", NULL, 0, 0), 
      OPT_GROUP("FTDI Calibration options"),
      OPT_INTEGER(0, "minfreq", &minfreq, "set min clock frequency for calibration (default: 100000 - 100 kHz)", NULL, 0, 0),
      OPT_INTEGER(0, "maxfreq", &maxfreq, "set max clock frequency for calibration (default: 30000000 - 30 MHz)", NULL, 0, 0),
      OPT_GROUP("FTDI Quick Setup options"),
      OPT_INTEGER(0, "cfreq", &cfreq, "set FTDI clock frequency", NULL, 0, 0),
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
      try {
         dev.reset(new FTDIDevice(vid, pid, interface));
      } catch (const std::exception& e) {
         std::cout << e.what() << std::endl;
         exit(-1);
      }
   } else {
      std::cout << "E: driver " << driverName << " not found" << std::endl;
      exit(-1);
   }

   // apply command line parameters
   dev.get()->setDebugLevel(debugLevel);
   dev.get()->setVerbose(verbose);
   
   std::cout << "I: using driver " << driverName << std::endl;
   std::cout << "I: device detected: " << dev.get()->getDescription() << 
      " idcode: 0x" << std::hex << dev.get()->getIdCode() << std::dec <<
      " irlen: " << dev.get()->getIrLen() <<
      " idcmd: 0x" << std::hex << dev.get()->getIdCmd() << std::dec << std::endl;

   if(scan)
      exit(0);

   if(saveFilename) {
      if(dev.get()->getName() == "AXI") {
         AXISetup *setup = new AXISetup();
         setup->setVerbose(verbose);
         std::cout << "I: start calibration task" << std::endl;
         AXICalibrator *calib = new AXICalibrator((AXIDevice *) dev.get());
         calib->setDebugLevel(debugLevel);
         calib->setVerbose(verbose);
         if(hyst) {
            calib->setHysteresis(hyst);
            std::cout << "I: apply hysteresis value " << hyst << std::endl;
         }
         calib->start(setup);
         // save calibration data to file
         setup->saveFile(saveFilename);
         std::cout << "I: calibration data saved in " << saveFilename << std::endl; 
         // calibration does not start XVC server
         exit(0);
      } else if(dev.get()->getName() == "FTDI") {
         FTDISetup *setup = new FTDISetup();
         setup->setVerbose(verbose);
         std::cout << "I: start calibration task" << std::endl;
         FTDICalibrator *calib = new FTDICalibrator((FTDIDevice *) dev.get());
         calib->setDebugLevel(debugLevel);
         calib->setVerbose(verbose);
         if(minfreq > maxfreq) {
            std::cout << "E: calibration min clock frequency is greater then max clock frequency" << std::endl;
            exit(-1);
         }
         calib->start(setup, minfreq, maxfreq);
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

   if(cfreq != -1) {
      if(dev.get()->getName() == "FTDI") {
         quickSetup = true;
         std::cout << "I: apply clock frequency " << cfreq << std::endl;
         FTDIDevice *fdev = (FTDIDevice *) dev.get();
         fdev->setClockFrequency(cfreq);
      } else std::cout << "E: clock frequency parameter not supported by driver " << driverName << std::endl;
   }

   if(quickSetup) {
      std::cout << "I: manual setup used - skip other setup options" << std::endl;
      goto startServer; 
   }

   if(loadFilename) {
      if(dev.get()->getName() == "AXI") {
         AXISetup *setup = new AXISetup();
         setup->setVerbose(verbose);
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

      }  else if(dev.get()->getName() == "FTDI") {

         FTDISetup *setup = new FTDISetup();
         setup->setVerbose(verbose);
         bool ret = setup->loadFile(loadFilename);
         if(ret) {

            FTDICalibItem *item;

            std::cout << "I: file " << loadFilename << " loaded successfully" << std::endl;
            
            // check for id command line options
            if(id != -1) {
            
               item = setup->getItemById(id);
               if(item != nullptr) {
                  FTDIDevice *fdev = (FTDIDevice *) dev.get();
                  fdev->setClockDiv(DIV5_OFF, item->clkDiv);
                  std::cout << "I: FTDI setup with id " << id << " successfully (DIV:" << item->clkDiv << ")" << std::endl;
               } else std::cout << "E: setup item with id " << id << " not found" << std::endl;
               goto startServer;
            }
            
            // check for freq command line options
            if(freq != -1)
               item = setup->getItemByFrequency(freq);
            else  // select max frequency
               item = setup->getItemByMaxFrequency();

            FTDIDevice *fdev = (FTDIDevice *) dev.get();
            fdev->setClockDiv(DIV5_OFF, item->clkDiv);
            std::cout << "I: FTDI setup with id " << item->id << " successfully (DIV:" << item->clkDiv << 
                  " FREQ:" << item->clkFreq << ")" << std::endl;

         } else { 
            std::cout << "E: file " << loadFilename << " loading error" << std::endl;
            exit(-1);
         }
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
