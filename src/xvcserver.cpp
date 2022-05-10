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
   bool runCalib = false;
   const char *saveFilename = NULL;
   const char *loadFilename = NULL;
   const char *serial = NULL;
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
   int loop = 10;
   bool pedge = false;

   AXISetup *asetup = new AXISetup();
   FTDISetup *fsetup = new FTDISetup();

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
      OPT_BOOLEAN('r', "runcalib", &runCalib, "start calibration and run server (default: max freq)"),
      OPT_STRING('s', "savecalib", &saveFilename, "start calibration and save data to file", NULL, 0, 0),
      OPT_STRING('l', "loadcalib", &loadFilename, "load calibration data from file", NULL, 0, 0),
      OPT_INTEGER(0, "id", &id, "load calibration entry from file by id"),
      OPT_INTEGER(0, "freq", &freq, "load calibration entry from file by clock frequency"),
      OPT_GROUP("AXI Calibration options"),
      OPT_INTEGER(0, "hyst", &hyst, "set hysteresis value (default: 0)"),
      OPT_GROUP("AXI Quick Setup options"),
      OPT_INTEGER(0, "cdiv", &cdiv, "set clock divisor (0:1023)", NULL, 0, 0),
      OPT_INTEGER(0, "cdel", &cdel, "set capture delay (0:255)", NULL, 0, 0),
      OPT_GROUP("FTDI options"),
      OPT_INTEGER(0, "vid", &vid, "set FTDI device Vendor ID (default: 0x0403)", NULL, 0, 0),
      OPT_INTEGER(0, "pid", &pid, "set FTDI device Product ID (default: 0x6010)", NULL, 0, 0),
      OPT_INTEGER(0, "interface", &interface, "set FTDI device JTAG interface (default: 1)", NULL, 0, 0), 
      OPT_STRING(0, "serial", &serial, "set serial number (default: none)", NULL, 0, 0),
      OPT_GROUP("FTDI Calibration options"),
      OPT_INTEGER(0, "minfreq", &minfreq, "set min clock frequency for calibration (default: 100000 - 100 kHz)", NULL, 0, 0),
      OPT_INTEGER(0, "maxfreq", &maxfreq, "set max clock frequency for calibration (default: 30000000 - 30 MHz)", NULL, 0, 0),
      OPT_INTEGER(0, "loop", &loop, "set number of loop to use for calibration (default: 10)", NULL, 0, 0),
      OPT_GROUP("FTDI Quick Setup options"),
      OPT_INTEGER(0, "cfreq", &cfreq, "set FTDI clock frequency", NULL, 0, 0),
      OPT_BOOLEAN(0, "pedge", &pedge, "set FTDI TDO positive sampling edge (default: 0 - negative)"),
      OPT_END(),
   };

   struct argparse argparse;
   argparse_init(&argparse, options, usage, 0);
   argparse_describe(&argparse, "\nXilinx Virtual Cable (XVC) adaptive server", "\nDefine AXIJTAG_UIO_ID environment variable to specify UIO device file id (default: 1 => /dev/uio1)\n\n");
   argparse_parse(&argparse, argc, argv);

   std::cout << "I: using driver " << driverName << std::endl;

   if(std::string(driverName) == "AXI") {
      try {
         dev.reset(new AXIDevice(verbose, debugLevel));
      } catch (const std::exception& e) {
         std::cout << e.what() << std::endl;
         exit(-1);
      }
   } else if(std::string(driverName) == "FTDI") {
      try {
         dev.reset(new FTDIDevice(vid, pid, interface, serial, verbose, debugLevel));
      } catch (const std::exception& e) {
         std::cout << e.what() << std::endl;
         exit(-1);
      }
   } else {
      std::cout << "E: driver " << driverName << " not found" << std::endl;
      exit(-1);
   }

   if(dev.get()->isDetected())
      std::cout << "I: device detected: " << dev.get()->getDescription() << 
         " idcode: 0x" << std::hex << dev.get()->getIdCode() << std::dec <<
         " irlen: " << dev.get()->getIrLen() <<
         " idcmd: 0x" << std::hex << dev.get()->getIdCmd() << std::dec << std::endl;

   if(scan)
      exit(0);

   if(runCalib || saveFilename) {
      if(dev.get()->getName() == "AXI") {
         asetup->setVerbose(verbose);
         std::cout << "I: start calibration task" << std::endl;
         AXICalibrator *calib = new AXICalibrator((AXIDevice *) dev.get());
         calib->setDebugLevel(debugLevel);
         calib->setVerbose(verbose);
         if(hyst) {
            calib->setHysteresis(hyst);
            std::cout << "I: apply hysteresis value " << hyst << std::endl;
         }
         calib->start(asetup);
         if(saveFilename) {
            // save calibration data to file
            asetup->saveFile(saveFilename);
            std::cout << "I: calibration data saved in " << saveFilename << std::endl; 
         } 

         if(!runCalib)
            exit(0);

      } else if(dev.get()->getName() == "FTDI") {
         fsetup->setVerbose(verbose);
         std::cout << "I: start calibration task" << std::endl;
         FTDICalibrator *calib = new FTDICalibrator((FTDIDevice *) dev.get());
         calib->setDebugLevel(debugLevel);
         calib->setVerbose(verbose);
         if(minfreq > maxfreq) {
            std::cout << "E: calibration min clock frequency is greater then max clock frequency" << std::endl;
            exit(-1);
         }
         if(loop < 0) {
            std::cout << "E: calibration loop must be positive number" << std::endl;
            exit(-1);
         }
         calib->start(fsetup, minfreq, maxfreq, loop);
         if(saveFilename) {
            // save calibration data to file
            fsetup->saveFile(saveFilename);
            std::cout << "I: calibration data saved in " << saveFilename << std::endl;
         } 
         if (!runCalib) {
            exit(0);
         }

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

   if(pedge) {
      if(dev.get()->getName() == "FTDI") {
         quickSetup = true;
         std::cout << "I: apply TDO positive sampling edge" << std::endl;
         FTDIDevice *fdev = (FTDIDevice *) dev.get();
         fdev->setTDOPosSampling(true);
      } else std::cout << "E: TDO sampling parameter not supported by driver " << driverName << std::endl;
   }

   if(quickSetup) {
      std::cout << "I: manual setup used - skip other setup options" << std::endl;
      goto startServer; 
   }

   if(runCalib || loadFilename) {
      if(dev.get()->getName() == "AXI") {
         asetup->setVerbose(verbose);
         if(loadFilename) {
            if(asetup->loadFile(loadFilename) == 0) {
               std::cout << "E: file " << loadFilename << " loading error" << std::endl;
               exit(-1);
            }
            std::cout << "I: file " << loadFilename << " loaded successfully" << std::endl;
         }
            
         AXICalibItem *item = asetup->getItemByMaxFrequency();     // run at max freq by default
         if(item == nullptr) {
            std::cout << "E: no valid calibration setting found" << std::endl;
            exit(-1);
         }

         // check for id command line options
         if(id != -1) {
         
            item = asetup->getItemById(id);
            if(item != nullptr) {
               AXIDevice *adev = (AXIDevice *) dev.get();
               adev->setClockDelay(item->getClockDelay());
               adev->setClockDiv(item->getClockDivisor());
               std::cout << "I: AXI setup with id " << id << " successfully" << std::endl;
               item->print();
            } else std::cout << "E: setup item with id " << id << " not found" << std::endl;
            goto startServer;
         }
         
         // check for freq command line options
         if(freq != -1)
            item = asetup->getItemByFrequency(freq);

         AXIDevice *adev = (AXIDevice *) dev.get();
         adev->setClockDelay(item->getClockDelay());
         adev->setClockDiv(item->getClockDivisor());
         std::cout << "I: AXI setup with id " << item->getId() << " successfully" << std::endl;
         item->print();

      }  else if(dev.get()->getName() == "FTDI") {

         fsetup->setVerbose(verbose);
         if(loadFilename) {
            if(fsetup->loadFile(loadFilename) == 0) {
               std::cout << "E: file " << loadFilename << " loading error" << std::endl;
               exit(-1);
            }
            std::cout << "I: file " << loadFilename << " loaded successfully" << std::endl;
         }

         FTDICalibItem *item = fsetup->getItemByMaxFrequency();     // run at max freq by default
         if(item == nullptr) {
            std::cout << "E: no valid calibration setting found" << std::endl;
            exit(-1);
         }

         // check for id command line options
         if(id != -1) {
         
            item = fsetup->getItemById(id);
            if(item != nullptr) {
               FTDIDevice *fdev = (FTDIDevice *) dev.get();
               fdev->setClockDiv(DIV5_OFF, item->getClockDivisor());
               fdev->setTDOPosSampling((bool)item->getTDOSampling());
               std::cout << "I: FTDI setup with id " << id << " successfully" << std::endl;
               item->print();
            } else std::cout << "E: setup item with id " << id << " not found" << std::endl;
            goto startServer;
         }
         
         // check for freq command line options
         if(freq != -1)
            item = fsetup->getItemByFrequency(freq);

         FTDIDevice *fdev = (FTDIDevice *) dev.get();
         fdev->setClockDiv(DIV5_OFF, item->getClockDivisor());
         fdev->setTDOPosSampling((bool)item->getTDOSampling());
         std::cout << "I: FTDI setup with id " << item->getId() << " successfully" << std::endl;
         item->print();
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
