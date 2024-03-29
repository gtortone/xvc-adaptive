#include "axidevice.h"

AXIDevice::AXIDevice(bool v, int dl) {
    
   setName("AXI");
   verbose = v;
   debugLevel = dl;
   const char *uioid = getenv("AXIJTAG_UIO_ID");
   std::string uiodev;   

   if(uioid != NULL)
      uiodev = "/dev/uio" + std::string(uioid);
   else
      uiodev = "/dev/uio1";

   if(debugLevel) {
      char msg[128];
      sprintf(msg, "AXIDevice: UIO device %s", uiodev.c_str());
      printDebug(msg, 1);
   }     

   fd = open(uiodev.c_str(), O_RDWR);

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

         detected = true;
      }
   }

   if(detected && verbose)
      printf("AXIDevice::detect device detected: idcode:0x%X irlen:%d idcmd:0x%X desc:%s\n",
         idcode, irlen, idcmd, desc.c_str());

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
   unsigned int *tdi, *tms, *tdo; 
   unsigned int tmsVal, tdiVal, tdoVal;
   unsigned int last_tdi, last_tms; 
   
   last_tms = ptr->tms_offset = 0;
   last_tdi = ptr->tdi_offset = 0;
   ptr->length_offset = 32;
   
   tms = reinterpret_cast<unsigned int*>(buffer);
   tdi = reinterpret_cast<unsigned int*>(buffer+nbytes);
   tdo = reinterpret_cast<unsigned int*>(result);

   while (bitsLeft > 0) {

      if (bitsLeft < 32) {
         
         ptr->length_offset = bitsLeft;
   
         tmsVal = tdiVal = 0;
         memcpy(&tmsVal, tms, bytesLeft);
         memcpy(&tdiVal, tdi, bytesLeft);

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

      tdoVal = ptr->tdo_offset;
           
      if (bitsLeft < 32) {
      
        // aligns captured TDO vector to lsb, compensates lack of hardware shifts 
        tdoVal = tdoVal >> (32 - bitsLeft);
        memcpy(tdo, &tdoVal, bytesLeft);

      } else {

        tdo[0] = tdoVal;
        
      }

      if(debugLevel) {
         char msg[128];
         sprintf(msg, "Bytes:%d Bits:%d TMS:0x%08x TDI:0x%08x TDO:0x%08x", (bytesLeft>4)?4:bytesLeft, (bitsLeft>32)?32:bitsLeft, *tms, *tdi, tdoVal);
         printDebug(msg, 3);
      }     

      bytesLeft -= 4;
      bitsLeft -= 32;
      tms++; // tms, tdi and tdo  pointers are not valid when bitsLeft < 32
      tdi++; // but then the are not used anymore
      tdo++;

   } // end while
}
