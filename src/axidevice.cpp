#include "axidevice.h"

AXIDevice::AXIDevice() {
    
   setName("AXI");

   fd = open("/dev/uio0", O_RDWR);

   if (fd < 1) {
      std::cout << "E: AXIDevice: failed to open UIO device" << std::endl;
      exit(-1);
   }

   ptr = (volatile jtag_t *) mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

AXIDevice::~AXIDevice() {
   munmap((void *)ptr, MAP_SIZE);
}

void AXIDevice::setClockDelay(int v) { 
   clkdel = v;
   ptr->delay_offset = clkdel;
};

void AXIDevice::setClockDiv(int v) { 
   clkdiv = v;
   ptr->tck_ratio_div2_min1_offset = clkdiv;
};

void AXIDevice::shift(int nbits, unsigned char *buffer, unsigned char *result) {
   
   int nbytes = (nbits + 7) / 8;

   int bytesLeft = nbytes;
   int bitsLeft = nbits;
   int byteIndex = 0;
   int *tdi, *tms, tdo; 
   int last_tdi, last_tms, last_length;

   last_tms = ptr->tms_offset = 0;
   last_tdi = ptr->tdi_offset = 0;
   last_length = ptr->length_offset = 0;

   while (bytesLeft > 0) {

      int len = 0;

      if (bytesLeft >= 4) {

         len = 4;
         if (32 != last_length)
         {
            ptr->length_offset = 32;
            last_length = 32;
         }

      } else {

         len = bytesLeft;
         ptr->length_offset = bitsLeft;
      }

      tms = reinterpret_cast<int*>(&buffer[byteIndex]);
      tdi = reinterpret_cast<int*>(&buffer[byteIndex + nbytes]);

      if (*tms != last_tms)
      {
         ptr->tms_offset = *tms;
         last_tms = *tms;
      }

      if (*tdi != last_tdi)
      {
         ptr->tdi_offset = *tdi;
         last_tdi = *tdi;
      }

      ptr->ctrl_offset = 0x01;

      while (ptr->ctrl_offset) { }

      tdo = ptr->tdo_offset;
      memcpy(&result[byteIndex], &tdo, len);

      bytesLeft -= 4;
      bitsLeft -= 32;
      byteIndex += 4;

      if(debugLevel) {
         char msg[128];
         sprintf(msg, "LEN:%d TMS:0x%08x TDI:0x%08x TDO:0x%08x", len, *tms, *tdi, tdo);
         printDebug(msg, 3);
      }

   } // end while
}
