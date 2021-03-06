#ifndef IOSERVER_H
#define IOSERVER_H

#include <string>          
#include <iostream>
#include <sstream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>

#include "xvcdriver.h"

/*
   IOServer opens TCP connection for XVC server and use XVCDriver to shift in/out buffers
*/

class IOServer {

private:
   bool verbose = false;
   int sock, maxfd;
   struct sockaddr_in address;
   fd_set conn;
   int port = 2542;
   int vectorLength = 32768;

   XVCDriver *drv;

   bool handleData(int fd);
   std::string xvcInfo;

   unsigned char *buffer, *result;

   int sread(int fd, void *target, int len);
   
public:
   IOServer(XVCDriver *driver);
   ~IOServer();

   void start(void);
   void setPort(int p) { port = p; }
   void setVerbose(bool v) { verbose = v; }
   void setVectorLength(int v);
};

#endif
