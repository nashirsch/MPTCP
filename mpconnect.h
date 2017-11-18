#ifndef MPCONNECT_H
#define MPCONNECT_H

#include "mpsend.h"

void closePorts(pathHolder* ph);

//connects ports, puts them within a pathHolder struct for main to handle
pathHolder* connectPorts(int* ports, int num, in_addr_t servIP, in_addr_t myIP);




#endif
