# XVC adaptive

Xilinx Virtual Cable (XVC) daemon with calibration features to compensate TDO propagation delay on long cables.

## Usage

```
Xilinx Virtual Cable (XVC) adaptive server

    -h, --help                show this help message and exit

Basic options
    -v, --verbose             enable verbose
    -d, --debug=<int>         set debug level (default: 0)

Network options
    -p, --port=<int>          set server port (default: 2542)

Driver options
    --driver=<str>            set driver name [AXI,FTDI] (default: AXI)

AXI Calibration options
    -s, --savecalib=<str>     start calibration and save data to file [AXI driver]
    -l, --loadcalib=<str>     load calibration data from file [AXI driver]
    --id=<int>                load calibration entry from file by id [AXI driver]
    --freq=<int>              load calibration entry from file by clock frequency [AXI driver]

AXI Setup options
    --cdiv=<int>              set clock divisor (0:1023) [AXI driver]
    --cdel=<int>              set clock delay (0:255) [AXI driver]
```

## AXI driver
AXI driver is based on Xilinx XAPP1251 that use an open IP core (AXI-JTAG). This IP core is modified in order to support configurable TCK frequency and delay to compensate TDO propagation on long cables.

Access from userspace to AXI IP core is provided by UIO (Userspace I/O).

Device database and handling methods are based on xc3sprog (http://xc3sprog.sourceforge.net) source code.

A calibration routine is available to identify valid frequency/delay couple that makes reliable JTAG communication between AXI IP core and FPGA target.
These measurements can be loaded/saved to file to have distinct setups.

## FTDI driver
FTDI driver is based on libftdi (https://www.intra2net.com/en/developer/libftdi/) and work of @wzab (https://github.com/wzab/xvcd-ff2232h) that use MPSSE instructions with XVC server.
