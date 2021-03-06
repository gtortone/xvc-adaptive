#include <iostream>
#include <iomanip>
#include <memory>
#include <unistd.h>
#include "ioserver.h"
#include "axidevice.h"

int main(int argc, char **argv) {

   std::unique_ptr<XVCDriver> dev;
   int width = 15;
   bool verbose = false;
   bool debug = false;
   int port = 2542;
   int c;

   dev.reset(new AXIDevice());
   IOServer *srv = new IOServer(dev.get());

   std::cout << "Xilinx Virtual Cable (XVC) adaptive server" << std::endl;

   while( (c = getopt(argc, argv, "vdhp:")) != -1 ) {

      switch(c) { 

         case 'v':
            std::cout << "I: verbose enabled" << std::endl;
            verbose = true;
            break;

         case 'd':
            std::cout << "I: debug enabled" << std::endl;
            debug = true;
            break;

         case 'h':
            std::cout << std::left << "Usage: " << basename(argv[0]) << " [OPTION]" << std::endl << std::endl;
            
            std::cout << std::left << "Network parameters" << std::endl;
            std::cout << std::left << std::setw(width) << "-p <port>" << "set server port (default: 2542)" << std::endl;
            std::cout << std::endl;

            std::cout << std::left << "General parameters" << std::endl;
            std::cout << std::left << std::setw(width) << "-v" << "enable verbose" << std::endl; 
            std::cout << std::left << std::setw(width) << "-d" << "enable debug" << std::endl; 
            std::cout << std::left << std::setw(width) << "-h" << "print usage" << std::endl; 
            std::cout << std::endl;
            exit(0);
            break;
         
         case 'p':
            if(atoi(optarg) != 0) {
               port = atoi(optarg);
               std::cout << "I: using TCP port " << port << std::endl;
            } else std::cout << "E: TCP port not valid - using default" << std::endl; 
            break;

         default:
            exit(-1);
      }
   }

   // apply command line parameters
   dev.get()->setDebug(debug);
   dev.get()->setVerbose(verbose);
   srv->setVerbose(verbose);
   srv->setPort(port);

   // test methods
   dev.get()->startCalibration();
   dev.get()->printCalibrationList();
   //dev.get()->setCalibrationById(0);
   //dev.get()->setCalibrationByClock(550000);
   //

   printf("Listening....\n");
   srv->start();

   return 0;
}
