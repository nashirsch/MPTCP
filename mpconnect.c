#include "mpconnect.h"


pathHolder* connectPorts(int* ports, int num, in_addr_t servIP, in_addr_t myIP){

  //establish path manager struct
  pathHolder* allPaths = (pathHolder*)malloc(sizeof(pathHolder));
  allPaths->servIP = servIP;
  allPaths->myIP = myIP;
  allPaths->numConns = num;
  allPaths->conns = (conn*)malloc(sizeof(conn) * num);

  //for each returned port:

  int i = 0;
  for(conn c = allPaths->conns[0]; i < num; c = allPaths->conns[i], i++) {

    //put server info in current connection struct
    c.servaddr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    c.servaddr->sin_family = AF_INET;
  	c.servaddr->sin_addr.s_addr = servIP;
  	c.servaddr->sin_port = htons(ports[i]);

    //create socket
    if((c.sd = mp_socket(AF_INET, SOCK_MPTCP, 0)) < 0){
      printf("Error establishing socket index %d\n", i);
  		exit(0);
    }

    //connect this socket
    if(mp_connect(c.sd,
                  (struct sockaddr*) c.servaddr,
                  16) != 0){
  		printf("Error estblishing connection on port index %d\n", i);
  		exit(0);
  	}
    //socket successfully connected!

    //getting this socket's information after connection
    c.clientaddr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    c.clientaddr->sin_family = AF_INET;
    socklen_t len = sizeof(struct sockaddr_in);
  	getsockname(c.sd, (struct sockaddr*) c.clientaddr, &len);

  }

  return(allPaths);
}
