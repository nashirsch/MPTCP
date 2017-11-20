#ifndef MPSEND_H
#define MPSEND_H

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
#define BUFSIZE (MSS - sizeof(struct mptcp_header))

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
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

//Node for the queue struct
typedef struct queueNode{

  struct queueNode* next;

  int seqNum;
  int size;

} qnode;

//Queue struct to contain unacked packets for a given connection
typedef struct queue{

  struct queueNode* root;

} queue;

//Struct to keep track of a single connection
typedef struct connection{

  int index; //index of this connection in the pathHolder
  int sd; //socket descriptor

  int ssthresh; //guess about the throughput knee, for congestion control
  int cwnd; //current congestion window
  int packsOut; //current unacked packets
  int currentAcks; //successful acks for current congestion window

  queue* packets; //queue of currently unacked packets

  struct sockaddr_in* clientaddr;
  struct sockaddr_in* servaddr;

  /*
   * Congestion Control Mode: How cwnd changes as packets are acked
   * exponential: when cwnd packets are successfully acked, the cwnd doubles
   * additive: when cwnd packets are successfully acked, cwnd++
   */
  enum {exponential, additive} congestionMode;

} conn;

//Struct to manage all connections
typedef struct pathHolder{

  int numConns; //number of connections for this pathholder
  conn* conns; //array of connections
  in_addr_t myIP; //this machine's IP
  in_addr_t servIP; //the server's IP

  int unacked; //total unacked BYTES
  int dataIndex; //current index at which connections are sending
  int success; //changes to 1 when file transfer complete, to notify all threads

  pthread_mutex_t lock; //used to ensure multiple threads arent sending simultaneously

} pathHolder;

//Used to pass arguments to connections when pthread_create is called
typedef struct threadInfo{

  pathHolder* ph;
  conn* cn;
  char* data;

  struct sockaddr_in* clientaddr;
  struct sockaddr_in* servaddr;

} threadInfo;

//Sends the given packet on the given connection
//Packet data is of length len
int senddata(conn* cn, struct packet* pack, int len);

//Retransmit function for a dropped packet
//Arguments:
//  ph - pathholder of all connections
//  ind - index of path to avoid (path on which packet was originally sent)
//  seqNum - sequence number into the data where the resent data begins
//  size - length of data to send in packet
//  data - pointer to string to transmit
//Return:
//  # of bytes sent
int retransmit(pathHolder* ph, int ind, int seqNum, int size, char* data);

//Central function that controls sending and recieving for a given connection
void* connThread(void* TI);

//Creates threads for each conn and calls connThread
bool sendFile(pathHolder* ph, char* data);

#endif
