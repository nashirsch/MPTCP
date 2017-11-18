#include "mpsend.h"

int senddata(conn* cn, struct packet* pack, int len){

  int sent = mp_send(cn->sd, pack, len, 0);
  return(sent);

}

int connThread(pathHolder* ph, conn* cn, char* data){

  int size = 0;

  struct packet* comms = (struct packet*) malloc(sizeof(struct packet));
  comms->data = (char*) malloc(sizeof(char) * BUFSIZE);
  comms->header = (struct mptcp_header*) malloc(sizeof(struct mptcp_header));
  comms->header->dest_addr = *cn->servaddr;
  comms->header->src_addr = *cn->clientaddr;
  comms->header->seq_num = 1;
  comms->header->ack_num = 1;
  comms->header->total_bytes = strlen(data);
  printf("%d\n", comms->header->total_bytes);

  int i = 20;
  while(i > 0){
    i--;

    if(ph->unacked < RWIN){
      if(cn->packsOut < cn->cwnd){

        printf("SENT PACKET\n");
        printf("\tData Index %d\n", ph->dataIndex);

        size = min(min(BUFSIZE, RWIN - ph->unacked - sizeof(struct mptcp_header)), strlen(data) - ph->dataIndex);
        cn->packsOut++;
        ph->unacked += size + sizeof(struct mptcp_header);

        memset(comms->data, 0, BUFSIZE);
        strncpy(comms->data, &data[ph->dataIndex], size);

        comms->header->ack_num = 1;
        comms->header->seq_num = ph->dataIndex + 1;
        ph->dataIndex += size;

        printf("\tUnacked bytes %d\n", ph->unacked);
        printf("\tUnacked packets %d\n", cn->packsOut);
        printf("\tcwnd %d\n", cn->cwnd);
        printf("\tlast recieved ack %d\n", cn->lastAck);
        printf("\tData send %d\n", size);
        printf("\t\tsent seq number %d\n", comms->header->seq_num);
        comms->header->dest_addr = *cn->servaddr;
        comms->header->src_addr = *cn->clientaddr;
        comms->header->total_bytes = strlen(data);
        senddata(cn, comms, size);

      }
    }

    if(mp_recv(cn->sd, comms, MSS, MSG_DONTWAIT) == -1){
      continue;
    }
    else{
      if(comms->header->ack_num == -1){
        printf("FILE\nTRANSFER\nCOMPLETE\n");
        return -1; //file transfer complete
      }

      //CHECK AND HANDLE LOSS HERE
      ph->unacked -= (comms->header->ack_num - cn->lastAck); //not true in case of loss
      cn->lastAck = comms->header->ack_num;
      cn->packsOut--;
      cn->currentAcks++;

    }

    if(cn->currentAcks == cn->cwnd){
      if(cn->congestionMode == exponential){
        cn->cwnd = 2*cn->cwnd;
        cn->currentAcks = 0;
      }
      else{
        cn->cwnd++;
        cn->currentAcks = 0;
      }
    }
  }
  return 1;
}


bool sendFile(pathHolder* ph, char* data){

  int PIDs[ph->numConns];
  int pid;

  for(int i = 0; i < ph->numConns; i++) {
    if((pid = fork()) == 0){
      if(connThread(ph, &ph->conns[i], data) == -1){
        for(int j = 0; j < ph->numConns; j++){
          if(i != j){
            kill(PIDs[j], SIGKILL);
          }
        }
        exit(0);
        //close connections
      }
    }

    else
      PIDs[i] = pid;

  }

  while (waitpid(-1, NULL, 0)) {
    if (errno == ECHILD) {
      return(true);
    }
  }

  return(false);
}
