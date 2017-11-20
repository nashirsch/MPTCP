#include "mpconnect.h"


//Goes through each connection and closes it
void closePorts(pathHolder* ph){

  for(int i = 0; i < ph->numConns; i++)
    close(ph->conns[i].sd);

  return;

}


//Connects ports, puts them within a pathHolder struct for main to handle
pathHolder* connectPorts(int* ports, int num, in_addr_t servIP, in_addr_t myIP){

  //establish path manager struct
  pathHolder* allPaths = (pathHolder*)malloc(sizeof(pathHolder));
  allPaths->servIP = servIP;
  allPaths->myIP = myIP;
  allPaths->numConns = num;
  allPaths->conns = (conn*)malloc(sizeof(conn) * num);
  allPaths->unacked = 0;
  allPaths->dataIndex = 0;
  allPaths->success = 0;


  //for each returned port:
  int i = 0;
  for(conn* c = &allPaths->conns[0]; i < num; c = &allPaths->conns[i]) {

    //put server info in current connection struct
    c->servaddr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    c->servaddr->sin_family = AF_INET;
  	c->servaddr->sin_addr.s_addr = servIP;
  	c->servaddr->sin_port = htons(ports[i]);

    //create socket, set recieving timeout
    if((c->sd = mp_socket(AF_INET, SOCK_MPTCP, 0)) < 0){
      printf("Error establishing socket index %d\n", i);
  		exit(0);
    }
    struct timeval* timeout = (struct timeval*) malloc(sizeof(struct timeval));
    timeout->tv_sec = 2;
    timeout->tv_usec = 0;
    setsockopt(c->sd, SOL_SOCKET, SO_RCVTIMEO, timeout, sizeof(struct timeval));

    //connect this socket
    if(mp_connect(c->sd,
                  (struct sockaddr*) c->servaddr,
                  16) != 0){
  		printf("Error establishing connection on port index %d\n", i);
  		exit(0);
  	}
    //socket successfully connected!

    //getting this socket's information after connection
    c->clientaddr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    c->clientaddr->sin_family = AF_INET;
    socklen_t len = sizeof(struct sockaddr_in);
  	getsockname(c->sd, (struct sockaddr*) c->clientaddr, &len);

    //setting initial connection information
    c->ssthresh = INT_MAX;
    c->cwnd = 1;
    c->packsOut = 0;
    c->currentAcks = 0;
    c->congestionMode = exponential;
    c->index = i;

    //setup the queue of unacked packets for packet loss recognition
    c->packets = (queue*) malloc(sizeof(queue));
    c->packets->root = (qnode*) malloc(sizeof(qnode));
    c->packets->root->seqNum = -1;
    c->packets->root->size = -1;
    c->packets->root->next = NULL;

    i++;
  }

  return(allPaths);
}
