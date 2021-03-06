#include <iostream>
#include <memory>
#include <unistd.h>
#include "ioserver.h"
#include "axidevice.h"

int main(int argc, char **argv) {

   std::unique_ptr<XVCDriver> dev;
   bool verbose = false;
   int c;

   dev.reset(new AXIDevice());
	IOServer *srv = new IOServer(dev.get());

	std::cout << "Xilinx Virtual Cable (XVC) adaptive server" << std::endl;

   while( (c = getopt(argc, argv, "v")) != -1 ) {

      switch(c) { 

         case 'v':
            std::cout << "I: verbose enabled" << std::endl;
            verbose = true;
            break;
      }
   }

   dev.get()->setDebug(false);
   dev.get()->setVerbose(verbose);
	srv->setVerbose(verbose);
	srv->setVectorLength(32768);

   dev.get()->startCalibration();
   dev.get()->printCalibrationList();
   dev.get()->setCalibrationParams(49);
   dev.get()->setCalibrationParams(490);

   printf("Listening....\n");

	srv->start();

	return 0;
}
