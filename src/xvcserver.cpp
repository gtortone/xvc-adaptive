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
 
   if(port != 2542)
      std::cout << "I: using TCP port " << port << std::endl;
   srv->setPort(port);

   dev.get()->startCalibration();
   //dev.get()->printCalibrationList();
   //dev.get()->setCalibrationById(0);
   //dev.get()->setCalibrationByClock(8500000);      // 8.5 MHz
   //

   printf("Listening....\n");
   srv->start();

   return 0;
}
