#include <iostream>
#include <unistd.h>
#include "ioserver.h"
#include "axidevice.h"

int main(int argc, char **argv) {

	AXIDevice *dev = new AXIDevice();
	IOServer *srv = new IOServer(dev);
   bool verbose = false;
   int c;

	std::cout << "Xilinx Virtual Cable (XVC) adaptive server" << std::endl;

   while( (c = getopt(argc, argv, "v")) != -1 ) {

      switch(c) { 

         case 'v':
            std::cout << "I: verbose enabled" << std::endl;
            verbose = true;
            break;
      }
   }

   dev->setVerbose(verbose);
	srv->setVerbose(verbose);
	srv->setVectorLength(32768);

   dev->setDelay(0);
   dev->setClockDiv(255);

   int refIdcode = dev->getIdCode();
   printf("reference idcode = 0x%X\n", refIdcode);

   for(int c=0; c<256; c++) {

      dev->setClockDiv(c);

      float clocksel = 100000000.0 / ((c + 1) * 2);

      int minDelay = -1;
      int maxDelay = -1;

      for(int d=0; d<128; d++) {

         dev->setDelay(d);

         int idcode = dev->getIdCode();

         if(idcode != refIdcode) {
          
            printf("error !\n");
         
         } else {

            if(minDelay == -1)
               minDelay = d;

            maxDelay = d;
         }
      }

      if(minDelay != -1)
         printf("DIV:%d DLY:%d CLK:%.0f\n", c, (maxDelay - minDelay)/2, clocksel);
   }
  
   printf("Listening....\n");

	srv->start();

	return 0;
}
