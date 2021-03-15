#include "axidevice.h"

AXIDevice::AXIDevice(bool v, int dl) {
    
   setName("AXI");
   verbose = v;
   debugLevel = dl;

   fd = open("/dev/uio0", O_RDWR);

   if (fd < 1)
      throw std::runtime_error("E: AXIDevice: failed to open UIO device");

   ptr = (volatile jtag_t *) mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

   // try to detect device
   if(!detect()) 
      printf("WARNING: AXIDevice: failed to detect JTAG target (idcode: 0x%08X)\n", idcode);
}

bool AXIDevice::detect(void) {

   printDebug("AXIDevice::detect start", 1);

   DeviceDB devDB(0);
   uint32_t tempId;
   const char *tempDesc;
   bool found = false;

   // read id code at low clock frequency with delay sweep
   setClockDiv(MAX_CLOCK_DIV);

	for(int cdel=0; cdel<MAX_CLOCK_DELAY; cdel++) {

      setClockDelay(cdel);
      tempId = scanChain();
      tempDesc = devDB.idToDescription(tempId);

      if (tempDesc) {
         found = true;
         idcode = tempId;
         irlen = devDB.idToIRLength(idcode);
         idcmd = devDB.idToIDCmd(idcode);
         desc = tempDesc;

         if(verbose)
            printf("AXIDevice::detect device detected: idcode:0x%X irlen:%d idcmd:0x%X desc:%s\n", 
                  idcode, irlen, idcmd, desc.c_str());
      }
   }

   printDebug("AXIDevice::detect end", 1);

   return found;
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
   unsigned int *tdi, *tms, tdo; 
   unsigned int tmsVal, tdiVal;
   unsigned int last_tdi, last_tms, last_length;

   last_tms = ptr->tms_offset = 0;
   last_tdi = ptr->tdi_offset = 0;
   last_length = ptr->length_offset = 0;


   while (bytesLeft > 0) {

      int len = 0;

      if (bitsLeft >= 32) {

         len = 4;
         if (32 != last_length)
         {
            ptr->length_offset = 32;
            last_length = 32;
         }

         tms = reinterpret_cast<unsigned int*>(&buffer[byteIndex]);
         tdi = reinterpret_cast<unsigned int*>(&buffer[byteIndex + nbytes]);

      } else {

         len = bytesLeft;
         ptr->length_offset = bitsLeft;
         last_length = bitsLeft;
   
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
      
      // aligns captured TDO vector to lsb, compensates lack of hardware shifts  
      if (bitsLeft < 32) {
        tdo = tdo >> (32 - bitsLeft);
      }

      memcpy(&result[byteIndex], &tdo, len);

      if(debugLevel) {
         char msg[128];
         sprintf(msg, "Bytes:%d Bits:%d TMS:0x%08x TDI:0x%08x TDO:0x%08x", len, (bitsLeft>32)?32:bitsLeft, *tms, *tdi, tdo);
         printDebug(msg, 3);
      }

      bytesLeft -= 4;
      bitsLeft -= 32;
      byteIndex += 4;

   } // end while
}
