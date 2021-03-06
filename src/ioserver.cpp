#include "ioserver.h"

IOServer::IOServer(XVCDriver *driver) {

	drv = driver;
	setVectorLength(vectorLength);

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if(sock < 0) {
		std::cout << "IOServer: socket error" << std::endl;
		exit(1);
	}

	int value = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &value, sizeof value);

	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	address.sin_family = AF_INET;

	if(bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
		std::cout << "IOServer: bind error" << std::endl;
		exit(1);
	}

	if(listen(sock, 1) < 0) {
		std::cout << "IOServer: listen error" << std::endl;
		exit(1);
	}

	FD_ZERO(&conn);
	FD_SET(sock, &conn);
	maxfd = sock;
}

void IOServer::setVectorLength(int v) {

	vectorLength = v;

   free(buffer);
   free(result);

	buffer = (unsigned char *) malloc(sizeof(char) * vectorLength);
	result = (unsigned char *) malloc(sizeof(char) * vectorLength/2);

	xvcInfo.clear();
	xvcInfo = "xvcServer_v1.0:";
	xvcInfo.append(std::to_string(vectorLength));
	xvcInfo.append("\n");
}

int IOServer::sread(int fd, void *target, int len) {

   unsigned char *t = (unsigned char *) target;

   while (len) {

      int r = read(fd, t, len);

      if (r <= 0)
         return r;

      t += r;
      len -= r;
   }

   return 1;
}

void IOServer::start(void) {

	while(true) {

		fd_set read = conn, except = conn;
		int fd;

		if (select(maxfd + 1, &read, 0, &except, 0) < 0) {
			std::cout << "IOServer: select error" << std::endl;
			exit(1);
		}

		for (fd = 0; fd <= maxfd; fd++) {

			if (FD_ISSET(fd, &read)) {

				if (fd == sock) {

					int newfd;
					socklen_t nsize = sizeof(address);

					newfd = accept(sock, (struct sockaddr *)&address, &nsize);

					if (verbose)
						std::cout << "IOServer: connection accepted - fd " << newfd << std::endl;

					if (newfd < 0) {

						std::cout << "IOServer: accept error" << std::endl;
					
					} else {

						int flag = 1;
						int optResult = setsockopt(newfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

						if (optResult < 0)
							std::cout << "IOServer: TCP_NODELAY error" << std::endl;

						if (newfd > maxfd) 
							maxfd = newfd;

						FD_SET(newfd, &conn);
					}
				
				} else if (handleData(fd)) {

					if (verbose)
						std::cout << "IOServer: connection closed - fd " << fd << std::endl;

					close(fd);

					FD_CLR(fd, &conn);
				}
			
			} else if (FD_ISSET(fd, &except)) {

				if (verbose)
					std::cout << "IOServer: connection aborted - fd " << fd << std::endl;

				close(fd);
				FD_CLR(fd, &conn);

				if (fd == sock)
					break;
			}
		} // end for
	} // end while
}

bool IOServer::handleData(int fd) {

	char cmd[16];

	while(true) {

		memset(cmd, 0, 16);

		if (sread(fd, cmd, 2) != 1)
         return 1;

		if (memcmp(cmd, "ge", 2) == 0) {

         if (sread(fd, cmd, 6) != 1)
            return 1;
			
			memcpy(result, xvcInfo.c_str(), xvcInfo.length());

         if (write(fd, result, xvcInfo.length()) != (ssize_t)(xvcInfo.length())) {
            std::cout << "IOServer: reply error" << std::endl;
            return 1;
         }

         if (verbose) {
            std::cout << "IOServer: received command: 'getinfo' " << (int)time(NULL) << std::endl;
				std::cout << "IOServer: replied with " << xvcInfo << std::endl;
         }

         break;
		
		} else if (memcmp(cmd, "se", 2) == 0) {

         if (sread(fd, cmd, 9) != 1)
            return 1;

         memcpy(result, cmd + 5, 4);

         if (write(fd, result, 4) != 4) {
            std::cout << "IOServer: reply error" << std::endl;
            return 1;
         }

         if (verbose)
         {
            std::cout << "IOServer: received command: 'settck' " << (int)time(NULL) << std::endl;
            std::cout << "IOServer: replied with " << result << std::endl;
         }

         break;
      
		} else if (memcmp(cmd, "sh", 2) == 0) {

         if (sread(fd, cmd, 4) != 1)
            return 1;

         if (verbose)
				std::cout << "IOServer: received command: 'shift' " << (int)time(NULL) << std::endl;

			// we will shift payload after...
      
		} else {

			std::cout << "IOServer: invalid command " << cmd << std::endl;
			return 1;
		}

		// shift payload

		int nbits;
		if (sread(fd, &nbits, 4) != 1) {

         std::cout << "IOServer: reading length failed " << std::endl;
         return 1;
      }

      int nbytes = (nbits + 7) / 8;
      if (verbose)
         std::cout << "IOServer: nbytes = " << nbytes << std::endl;

      if (nbytes * 2 > vectorLength) {
         std::cout << "IOServer: buffer size exceeded - requested: " << nbytes * 2 << " max: " << vectorLength << std::endl;
         return 1;
      }

      if (sread(fd, buffer, nbytes * 2) != 1) {
         std::cout << "IOServer: reading data failed " << std::endl;
         return 1;
      }

		if (verbose) {
			std::cout << "IOServer: number of bits " << nbits << std::endl;
			std::cout << "IOServer: number of bytes " << nbytes << std::endl;
      }

		drv->shift(nbits, buffer, result);
		
		if (write(fd, result, nbytes) != nbytes) 
			std::cout << "IOServer: failed to write data to client" << std::endl;

	} // end while

	return 0;
}
