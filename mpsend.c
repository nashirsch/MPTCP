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

  int i = 20;
  while(i > 0){
    i--;

    if(ph->unacked < RWIN){
      if(cn->packsOut < cn->cwnd){

        printf("\tData Index %d\n", ph->dataIndex);
        printf("\t\tMYPID %d\n", getpid());

        //find the maximum size we can send given the three contraints:
        //    1) cant send more than 84 bytes in a segment
        //    2) cant send more than RWIN - unacknowledged bytes, with room for header
        //    3) cant send more than the amount of data left to transfer
        size = min(min(BUFSIZE, RWIN - ph->unacked - sizeof(struct mptcp_header)), strlen(data) - ph->dataIndex);

        //clear and copy data into buffer
        memset(comms->data, 0, BUFSIZE);
        strncpy(comms->data, &data[ph->dataIndex], size);

        //return ack to 1, and the seq to the current data index + 1
        comms->header->ack_num = 1;
        comms->header->seq_num = ph->dataIndex + 1;

        ph->unacked += size + sizeof(struct mptcp_header); //update unacked bytes on all paths
        ph->dataIndex += size; //update current data position
        cn->packsOut++; //update number of unacked packets on this path

        //swap address information on I/O packet, and reset the amount of data
        comms->header->dest_addr = *cn->servaddr;
        comms->header->src_addr = *cn->clientaddr;
        comms->header->total_bytes = strlen(data);

        //unlock mutex here
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
