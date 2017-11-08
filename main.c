#include "mpconnect.h"

int main(int argc, char *argv[]){

	if(argc != 5){
		printf("\nError: invalid number of arguments\n");
		printf("Expected: [#Paths] [Hostname] [Port] [File]\n\n");
		exit(0);
	}

	struct sockaddr_in* servaddr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	servaddr->sin_family = AF_INET;
	servaddr->sin_addr.s_addr = inet_addr(argv[2]);
	servaddr->sin_port = htons(atoi(argv[3]));

	//creating port-yield socket
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

	struct sockaddr_in clientaddr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	socklen_t socklen = sizeof(clientaddr);
	getsockname(reqsd, (struct sockaddr*) &clientaddr, &socklen);

	//preparing request string
	char* request = (char*)malloc(sizeof(char) * 256);
	request = strcat(request, "MPREQ ");
	request = strcat(request, argv[1]);

	//preparing header of request
	struct mptcp_header* reqhead = (struct mptcp_header*) malloc(sizeof(struct mptcp_header));
	reqhead->dest_addr = *servaddr;
	reqhead->src_addr = clientaddr;
	reqhead->seq_num = 1;
	reqhead->ack_num = 0;
	reqhead->total_bytes = strlen(request);

	struct packet* req = (struct packet*) malloc(sizeof(struct packet));
	req->data = request;
	req->header = reqhead;

	//preparing header of return
	struct packet* ret = (struct packet*) malloc(sizeof(struct packet));
	ret->data = (char*)malloc(sizeof(char)*128);

	printf("%u\n", inet_addr(argv[2]));

	mp_send(reqsd, req, sizeof(struct mptcp_header) + strlen(request), 0);
	while(mp_recv(reqsd, ret, 128, 0) != 0){
	}

	return 0;

}