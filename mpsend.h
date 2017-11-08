#ifndef MPSEND_H
#define MPSEND_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "mptcp.h"

//struct to keep track of a single connection
typedef struct connection{

  int sd;
  int lastAck;
  int lastSeq;

  int ssthresh;
  int cwnd;

  struct sockaddr_in* clientaddr;
  struct sockaddr_in* servaddr;

} conn;

//struct to manage all connections
typedef struct pathHolder{

  int numConns;
  conn* conns;
  in_addr_t myIP;
  in_addr_t servIP;

} pathHolder;

void sendFile(pathHolder* ph, char* data);

#endif
