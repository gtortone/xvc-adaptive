# XVC adaptive

Xilinx Virtual Cable (XVC) daemon with calibration features to compensate TDO propagation delay on long cables.

## Usage

```
Xilinx Virtual Cable (XVC) adaptive server

    -h, --help                show this help message and exit

Basic options
    -v, --verbose             enable verbose
    -d, --debug=<int>         set debug level (default: 0)
    --driver=<str>            set driver name [AXI,FTDI] (default: AXI)
    --scan                    scan for connected device and exit

Network options
    -p, --port=<int>          set server port (default: 2542)

Calibration options
    -s, --savecalib=<str>     start calibration and save data to file
    -l, --loadcalib=<str>     load calibration data from file
    --id=<int>                load calibration entry from file by id
    --freq=<int>              load calibration entry from file by clock frequency

AXI Calibration options
    --hyst=<int>              set hysteresis value (default: 0)

AXI Quick Setup options
    --cdiv=<int>              set clock divisor (0:1023)
    --cdel=<int>              set capture delay (0:255)

FTDI options
    --vid=<int>               set FTDI device Vendor ID (default: 0x0403)
    --pid=<int>               set FTDI device Product ID (default: 0x6010)
    --interface=<int>         set FTDI device JTAG interface (default: 1)
    --serial=<str>            set serial number (default: none)

FTDI Calibration options
    --minfreq=<int>           set min clock frequency for calibration (default: 100000 - 100 kHz)
    --maxfreq=<int>           set max clock frequency for calibration (default: 30000000 - 30 MHz)
    --loop=<int>              set number of loop to use for calibration (default: 10)

FTDI Quick Setup options
    --cfreq=<int>             set FTDI clock frequency
    --pedge                   set FTDI TDO positive sampling edge (default: 0 - negative)
```

## Build
FTDI code depends from libusb and libftdi
```
apt-get install libusb-1.0-0 libusb-1.0-0-dev libftdi1 libftdi1-dev
```

## AXI driver
AXI driver is based on Xilinx XAPP1251 that use an open IP core (AXI-JTAG). This IP core is modified in order to support configurable TCK frequency and delay to compensate TDO propagation on long cables.

Access from userspace to AXI IP core is provided by UIO (Userspace I/O).

Device database and handling methods are based on xc3sprog (http://xc3sprog.sourceforge.net) source code.

A calibration routine is available to identify valid frequency/delay couple that makes reliable JTAG communication between AXI IP core and FPGA target.
These measurements can be loaded/saved to file to have distinct setups.

## FTDI driver
FTDI driver is based on libftdi (https://www.intra2net.com/en/developer/libftdi/) and work of @wzab (https://github.com/wzab/xvcd-ff2232h) that use MPSSE instructions with XVC server.
