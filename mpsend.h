#ifndef MPSEND_H
#define MPSEND_H

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
#define BUFSIZE (MSS - sizeof(struct mptcp_header))

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h>
#include <pthread.h>
#include "mptcp.h"

//struct to keep track of a single connection
typedef struct connection{

  int sd;
  int lastAck; //last ack received
  int lastSeq; //number (not ind) of what to send next

  int ssthresh;
  int cwnd;
  int packsOut; //unacked packets
  int currentAcks; //successful acks for current window

  struct sockaddr_in* clientaddr;
  struct sockaddr_in* servaddr;

  enum {exponential, additive} congestionMode;

} conn;

//struct to manage all connections
typedef struct pathHolder{

  int numConns;
  conn* conns;
  in_addr_t myIP;
  in_addr_t servIP;

  int unacked;
  int dataIndex;
  int success;

  pthread_mutex_t lock;

} pathHolder;

typedef struct threadInfo{

  pathHolder* ph;
  conn* cn;
  char* data;

  struct sockaddr_in* clientaddr;
  struct sockaddr_in* servaddr;

} threadInfo;

int senddata(conn* cn, struct packet* pack, int len);

void* connThread(void* TI);

bool sendFile(pathHolder* ph, char* data);

#endif
