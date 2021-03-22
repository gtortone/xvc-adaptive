#include "ftdidevice.h"

FTDIDevice::FTDIDevice(int vid, int pid, enum ftdi_interface interface, const char *serial, bool v, int dl) {
   
   int res = 0;

   verbose = v;
   debugLevel = dl;
   setName("FTDI");

   if ((ftdi = ftdi_new()) == 0) {
      std::string errmsg(ftdi_get_error_string(ftdi));
      throw std::runtime_error("E: FTDIDevice failed allocation (" + errmsg + ")");
   }

   if(ftdi_set_interface(ftdi, interface) < 0) {
      std::string errmsg(ftdi_get_error_string(ftdi));
      throw std::runtime_error("E: FTDIDevice can't set interface (" + errmsg + ")");
   }

   if (ftdi_usb_open_desc(ftdi, vid, pid, NULL, serial) < 0) {
      std::string errmsg(ftdi_get_error_string(ftdi));
      throw std::runtime_error("E: FTDIDevice can't open device (" + errmsg + ")");
   }

   ftdi_usb_reset(ftdi);
   ftdi_set_latency_timer(ftdi, 1);

   unsigned char buf[] = {
      DIS_DIV_5,
      SET_BITS_LOW, 0x00, 0x0B,     // set TMS high, TCK/TDI/TMS as outputs
      TCK_DIVISOR, 0x2B, 0x01,      // 100 kHz  
      0x82, 0x00, 0x00,
      SEND_IMMEDIATE };

   ftdi_set_bitmode(ftdi, 0x0B, BITMODE_MPSSE);

   ftdi_usb_purge_rx_buffer(ftdi);
   ftdi_usb_purge_tx_buffer(ftdi);

   if ((res = ftdi_write_data(ftdi, buf, sizeof(buf))) != sizeof(buf)) {
      std::string errmsg(ftdi_get_error_string(ftdi));
      throw std::runtime_error("E: FTDIDevice error initializing MPSSE (" + errmsg + ")");
   }

   // set TDO sampling on clock negative edge
   setTDOPosSampling(false);

   // try to detect device
   if(!detect())
      std::cout << "WARNING: FTDIDevice: failed to detect JTAG target" << std::endl;
}

FTDIDevice::~FTDIDevice() {
   ftdi_usb_reset(ftdi);
   ftdi_usb_close(ftdi);
   ftdi_deinit(ftdi);
}

int FTDIDevice::getDivisorByFrequency(bool div5, int freq) {
   if(div5)
      return (12000000/(2 * freq)) - 1;
   else
      return (60000000/(2 * freq)) - 1;
}

int FTDIDevice::getFrequencyByDivisor(bool div5, int div) {
   if(div5)
      return 12000000/((1+div)*2);
   else
      return 60000000/((1+div)*2);
}

void FTDIDevice::setClockDiv(bool div5, int value) {

   unsigned char div5word = div5?EN_DIV_5:DIS_DIV_5;
   unsigned char valueh = value>>8 & 0xFF;
   unsigned char valuel = value & 0xFF;
  
   unsigned char buf[] = {
      div5word,
      TCK_DIVISOR, valuel, valueh,
      SEND_IMMEDIATE
   };

   ftdi_write_data(ftdi, buf, sizeof(buf)); 
}

void FTDIDevice::setClockFrequency(int freq) {

   int valDiv5Off = 0;
   int valDiv5On = 0;
   int freqDiv5Off, freqDiv5On;
   int divisorBy5 = false;
   int value;

   if (freq <= MAX_CFREQ_DIV5_ON && freq >= MIN_CFREQ_DIV5_ON)
      valDiv5On = getDivisorByFrequency(true, freq);
      
   if (freq <= MAX_CFREQ_DIV5_OFF && freq >= MIN_CFREQ_DIV5_OFF)
      valDiv5Off = getDivisorByFrequency(false, freq);

   freqDiv5On  = getFrequencyByDivisor(true, valDiv5On);
   freqDiv5Off = getFrequencyByDivisor(false, valDiv5Off);

   if(abs(freq-freqDiv5On) <= abs(freq-freqDiv5Off)) {
      divisorBy5 = true;
      value = valDiv5On;
   } else {
      divisorBy5 = false;
      value = valDiv5Off;
   }

   setClockDiv(divisorBy5, value);

   if(verbose)
      std::cout << "FTDIDevice::setClockFrequency divisorby5: " << divisorBy5 << 
         " value: " << value << " frequency req: " << freq << std::endl;
}

void FTDIDevice::setTDOPosSampling(bool value) {
   samplingEdge = value?POS_EDGE:NEG_EDGE;
}

bool FTDIDevice::detect(void) {

   printDebug("FTDIDevice::detect start", 1);

   DeviceDB devDB(0);
   uint32_t tempId;
   const char *tempDesc;
   bool found = false;

   // read id code at _very_ low clock frequency (91,553 Hz)
   setClockDiv(DIV5_ON, 0xFFFF);

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

   if(detected && verbose)
      printf("FTDIDevice::detect device detected: idcode:0x%X irlen:%d idcmd:0x%X desc:%s\n",
         idcode, irlen, idcmd, desc.c_str());

   printDebug("FTDIDevice::detect end", 1);

   return found;
}

void FTDIDevice::readBytes(unsigned int len, unsigned char *buf) {
   
   int read, to_read, last_read;
   to_read = len;
   read = 0;
   
   last_read = ftdi_read_data(ftdi, buf, to_read);
   if (last_read > 0)
      read += last_read;

   while (read < to_read) {
      last_read = ftdi_read_data(ftdi, buf + read, to_read - read);
      if (last_read > 0)
         read += last_read;
   } 
}

void FTDIDevice::shift(int nbits, unsigned char *buffer, unsigned char *result) {

   int i;
   int nr_bytes;
   int cur_byte_pos = 0;
   int bit_pos = 0;
   int left;
   // len is rounds up to the nearest byte
   // buffer length is (nr_bytes * 2)
   nr_bytes = (nbits + 7) / 8;
   left = nbits;
   unsigned char ftdi_cmd[MAX_DATA];
   unsigned char ftdi_res[MAX_DATA];
   data_desc ftdi_desc[MAX_DATA];

   // prepare the result buffer (TDO)
   memset(result, 0, nr_bytes);

   // loop until there are data to handle
   while (left) {

      int rd_len = 0;
      int wr_ptr = 0;
      int desc_pos = 0;
      int in_cmd_building = 0;
      int last_len = 0;
      int cur_len = 0;

      // loop until we may add a command to the current set
      while ((desc_pos < (MAX_DATA - 8)) && // we may generate up to 9 descriptors in a single iteration
         (wr_ptr < (MAX_DATA - 25)) &&  // we may generate up to 24 command bytes in a single iteration
         (left > 0)) {

         // buffer => TMS[] TDI[]
         // no TMS and at least 1 byte to transmit
         // TMS[] = 0 means no TMS
         if ((left > 7) && (buffer[cur_byte_pos] == 0)) {

            if (in_cmd_building == 0) {

               in_cmd_building = 1;
               ftdi_cmd[wr_ptr++] = MPSSE_DO_WRITE | MPSSE_DO_READ | MPSSE_LSB | MPSSE_WRITE_NEG | samplingEdge;
               // 0x10 + 0x20 + 0x08 + 0x01        = 0x39       without MPSSE_READ_NEG
               // 0x10 + 0x20 + 0x08 + 0x01 + 0x04 = 0x3D       with MPSSE_READ_NEG

               last_len = wr_ptr; // save pointer position, to write the length
               wr_ptr += 2;       // reserve space for length (byte mode with 2 len)
               cur_len = -1;      // will be increased when the byte is added
            }

            ftdi_cmd[wr_ptr++] = buffer[cur_byte_pos + nr_bytes];     // current element of TDI[]
            rd_len += 1;      // it generates one byte for reading
            cur_len += 1;     // update len of current command
            cur_byte_pos++;   // go to next element of TDI[]
            left -= 8;        // 8 bits (1 byte) done

         } else {

            // it is no standard (TDI) shift
            // if we created a standard shift command before we must complete it
            if (in_cmd_building) { // complete the last command

               ftdi_cmd[last_len] = cur_len & 0xff;
               ftdi_cmd[last_len + 1] = (cur_len >> 8) & 0xff;
               in_cmd_building = 0;
               // add the descriptor to the read descriptors
               ftdi_desc[desc_pos].oper = 0;         // byte shift
               ftdi_desc[desc_pos++].len = cur_len;
            }

            if ((buffer[cur_byte_pos] == 0) && (left <= 7)) { // no TMS, bit shift of last bits

               ftdi_cmd[wr_ptr++] = MPSSE_DO_WRITE | MPSSE_DO_READ | MPSSE_LSB | MPSSE_BITMODE | MPSSE_WRITE_NEG | samplingEdge;
               // 0x10 + 0x20 + 0x08 + 0x02 + 0x01           = 0x3B       without MPSSE_READ_NEG
               // 0x10 + 0x20 + 0x08 + 0x01 + 0x02 + 0x04    = 0x3F       with MPSSE_READ_NEG
               ftdi_cmd[wr_ptr++] = left - 1;
               ftdi_cmd[wr_ptr++] = buffer[cur_byte_pos + nr_bytes];     // current element of TDI[] 
               rd_len += 1;
               cur_byte_pos++;
               // add the descriptor to the read descriptors
               ftdi_desc[desc_pos].oper = 1;         // bit shift
               ftdi_desc[desc_pos++].len = left - 1;
               left = 0;

            } else if (buffer[cur_byte_pos] != 0) { // TMS shift, convert it into a set of TMS shifts

               int i;
               for (i = 0; i < 8; i++) {

                  if (left == 0)
                     break; // this could be the last byte!

                  ftdi_cmd[wr_ptr++] = MPSSE_WRITE_TMS | MPSSE_DO_READ | MPSSE_LSB | MPSSE_BITMODE | MPSSE_WRITE_NEG | samplingEdge;
                  // 0x40 + 0x20 + 0x08 + 0x02 + 0x01         = 0x6B
                  // 0x40 + 0x20 + 0x08 + 0x02 + 0x01 + 0x04  = 0x6F
                  ftdi_cmd[wr_ptr++] = 0; // one bit length
                  // buffer[cur_byte_pos] => current element of TMS[]
                  ftdi_cmd[wr_ptr++] =
                     ((buffer[cur_byte_pos] & (1 << i)) ? 0x01 : 0x00) |             // check TMS[] bit
                     ((buffer[cur_byte_pos + nr_bytes] & (1 << i)) ? 0x80 : 0x00);   // check correspondent TDI[] bit
                  left--;
                  rd_len += 1;
                  // add the descriptor to the read descriptors
                  ftdi_desc[desc_pos].oper = 2;    // TMS bit shift
                  ftdi_desc[desc_pos++].len = 0;   // 1 bit length
               } // end for

               cur_byte_pos++;
            }

         } // else

      }   // end while commands

      // we must complete the last command if it has not been completed yet
      // the code below should be the same, as in the loop!
      if (in_cmd_building) { // complete the last command

         ftdi_cmd[last_len] = cur_len & 0xff;
         ftdi_cmd[last_len + 1] = (cur_len >> 8) & 0xff;
         in_cmd_building = 0;
         // add the descriptor to the read descriptors
         ftdi_desc[desc_pos].oper = 0;         // byte shift
         ftdi_desc[desc_pos++].len = cur_len;
      }

      // send the created command list
      ftdi_write_data(ftdi, ftdi_cmd, wr_ptr);

      // read the response
      readBytes(rd_len, ftdi_res);

      // unpack the response basing on the read descriptors
      // please note, that the responses do not always come as full bits!
      int rd_byte_pos = 0;

      for (i = 0; i < desc_pos; i++) {

         int j;

         // reading depends on the type of command
         switch (ftdi_desc[i].oper) {

            case 0: // standard shift
               for (j = 0; j <= ftdi_desc[i].len; j++) {
                  int bnr;
                  for (bnr = 0; bnr < 8; bnr++) {
                     result[bit_pos / 8] |= (ftdi_res[rd_byte_pos] & (1 << bnr)) ? (1 << (bit_pos & 7)) : 0;
                     bit_pos++;
                  }
                  rd_byte_pos++;
               }
            break;
            case 1: // bit shift
               int bnr; // received bits are shifted from the MSB!
               for (bnr = 7 - ftdi_desc[i].len; bnr < 8; bnr++) {
                  result[bit_pos / 8] |= (ftdi_res[rd_byte_pos] & (1 << bnr)) ? (1 << (bit_pos & 7)) : 0;
                  bit_pos++;
               }
               rd_byte_pos++;
            break;
            case 2: // TMS shift
               result[bit_pos / 8] |= (ftdi_res[rd_byte_pos] & 0x80) ? (1 << (bit_pos & 7)) : 0;
               bit_pos++;
               rd_byte_pos++;
            break;

         } // end switch

      } // end for

   } // end while
} 
