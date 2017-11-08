#include "mpconnect.h"




//Parse MPOK return string into array of port ints
void parsePorts(int num, int* portArray, char* ret){
	ret = &ret[5];
	portArray[0] = atoi(strtok(ret, ":"));
	for(int i = 1; i < num; i++){
		portArray[i] = atoi(strtok(NULL, ":"));
	}
}




int main(int argc, char *argv[]){

	/*****************************************************************************
	 * CHECKING ARGUMENT SPEC
	 */
	if(argc != 5){
		printf("\nError: invalid number of arguments\n");
		printf("Expected: [#Paths] [Hostname] [Port] [File]\n\n");
		exit(0);
	}
	if((atoi(argv[1]) > 16) || (atoi(argv[1]) < 1)){
		printf("Error: max paths allowed is 16\n");
		exit(0);
	}


	/*****************************************************************************
	 *SETTING UP CONNECTION TO SERVER
	 */
	//getting server info
	struct sockaddr_in* servaddr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	servaddr->sin_family = AF_INET;
	servaddr->sin_addr.s_addr = inet_addr(argv[2]);
	servaddr->sin_port = htons(atoi(argv[3]));
	//creating socket
	int reqsd = mp_socket(AF_INET, SOCK_MPTCP, 0);
	if(reqsd < 0){
		perror("internal error\n");
		exit(0);
	}
	//requesting connection to address in servinfo
	if(mp_connect(reqsd, (struct sockaddr*) servaddr, 16) != 0){
		perror("internal error\n");
		exit(0);
	}
	//getting my IP and port
	socklen_t socklen = sizeof(struct sockaddr_in);
	struct sockaddr_in* clientaddr = (struct sockaddr_in*)malloc(socklen);
	getsockname(reqsd, (struct sockaddr*) clientaddr, &socklen);


	/*****************************************************************************
	 *REQUESTING INFO FROM SERVER
	 */
	//preparing request string
	char* request = (char*)malloc(sizeof(char) * 256);
	request = strcat(request, "MPREQ ");
	request = strcat(request, argv[1]);
	//preparing header of request
	struct mptcp_header* reqhead = (struct mptcp_header*) malloc(sizeof(struct mptcp_header));
	reqhead->dest_addr = *servaddr;
	reqhead->src_addr = *clientaddr;
	reqhead->seq_num = 1;
	reqhead->ack_num = 0;
	reqhead->total_bytes = strlen(request);
	//creating packet to send
	struct packet* req = (struct packet*) malloc(sizeof(struct packet));
	req->data = request;
	req->header = reqhead;


	/*****************************************************************************
	 *SENDING AND RECEIVING PORT INFO, CLOSING CONNECTION
	 */
	mp_send(reqsd, req, sizeof(struct mptcp_header) + strlen(request), 0);
	mp_recv(reqsd, req, 128, 0);
	close(reqsd);


	/*****************************************************************************
	 * PARSING PORTS, ESTAB. CONNECTIONS, READ FILE
	 */
	//getting int array of ports
	int ports[atoi(argv[1])];
	parsePorts(atoi(argv[1]), ports, req->data);
	//giving ports to connect function
	pathHolder* ph = connectPorts(ports,
		               							atoi(argv[1]),
							     							servaddr->sin_addr.s_addr,
							     							clientaddr->sin_addr.s_addr);

  //reading file to send
	FILE* fp;
	char* buf = (char*)malloc(sizeof(char) * (1 << 19));
	char* temp = (char*)malloc(sizeof(char) * 1024);
	if( !(fp = fopen(argv[4], "r")) ){
		printf("Error opening file\n");
		exit(0);
	}
	while(fgets(temp, 1024, fp)){
		strcat(buf, temp);
	}


	/*****************************************************************************
	 * INITIATING DATA TRANSFER
	 */
	sendFile(ph, buf);



	return(0);
}
