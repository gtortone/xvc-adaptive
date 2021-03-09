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
   if(v <= MAX_CLOCK_DELAY) {
      clkdel = v;
      ptr->delay_offset = clkdel;
   } else std::cout << "E: clock delay out of range: " << v << std::endl;
};

void AXIDevice::setClockDiv(int v) { 
   if(v <= MAX_CLOCK_DIV) {
      clkdiv = v;
      ptr->tck_ratio_div2_min1_offset = clkdiv;
   } else std::cout << "E: clock divisor out of range: " << v << std::endl;
};

void AXIDevice::shift(int nbits, unsigned char *buffer, unsigned char *result) {
   
   int nbytes = (nbits + 7) / 8;

   int bytesLeft = nbytes;
   int bitsLeft = nbits;
   int byteIndex = 0;
   int *tdi, *tms, tdo; 
   int tmsVal, tdiVal;
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

         tms = reinterpret_cast<int*>(&buffer[byteIndex]);
         tdi = reinterpret_cast<int*>(&buffer[byteIndex + nbytes]);

      } else {

         len = bytesLeft;
         ptr->length_offset = bitsLeft;

         tmsVal = tdiVal = 0;
         memcpy(&tmsVal, &buffer[byteIndex], bytesLeft);
         memcpy(&tdiVal, &buffer[byteIndex + nbytes], bytesLeft);

         tms = &tmsVal;
         tdi = &tdiVal;
      }

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

      if(debugLevel) {
         char msg[128];
         sprintf(msg, "LEN:%d bits:%d TMS:0x%08x TDI:0x%08x TDO:0x%08x", len, (len==4)?32:bitsLeft, *tms, *tdi, tdo);
         printDebug(msg, 3);
      }

      bytesLeft -= 4;
      bitsLeft -= 32;
      byteIndex += 4;

   } // end while
}
