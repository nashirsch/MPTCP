#include "mpsend.h"

int senddata(conn* cn, struct packet* pack, int len){

  int sent = mp_send(cn->sd, pack, len, 0);
  return(sent);

}

int retransmit(pathHolder* ph, int ind, int seqNum, int size, char* data){

  int resendPath = ((int) pow(ind*5, 3)) % ((int) ph->numConns);
  if(resendPath == ind){
    if(ph->numConns != 1){
      if(resendPath > 0){
        resendPath--;
      }
      else{
        resendPath++;
      }
    }
  }

  struct packet* comms = (struct packet*) malloc(sizeof(struct packet));
  comms->data = (char*) malloc(sizeof(char) * BUFSIZE);
  comms->header = (struct mptcp_header*) malloc(sizeof(struct mptcp_header));
  comms->header->dest_addr = *(ph->conns[ind].servaddr);
  comms->header->src_addr = *(ph->conns[ind].clientaddr);
  comms->header->seq_num = seqNum;
  comms->header->ack_num = 1;
  comms->header->total_bytes = strlen(data);

  strncpy(comms->data, &data[seqNum - 1], size);
  int sent = senddata(&ph->conns[resendPath], comms, size);
  printf("retransed %d\n", seqNum);

  free(comms->header);
  free(comms);

  return(sent);

}

void* connThread(void* TI){


  struct threadInfo* tInfo = TI;

  pathHolder* ph = tInfo->ph;
  conn* cn = tInfo->cn;
  char* data = tInfo->data;
  struct sockaddr_in* clientaddr = tInfo->clientaddr;
  struct sockaddr_in* servaddr = tInfo->servaddr;

  int size = 0;
  qnode* iterator;

  struct packet* comms = (struct packet*) malloc(sizeof(struct packet));
  comms->data = (char*) malloc(sizeof(char) * BUFSIZE);
  comms->header = (struct mptcp_header*) malloc(sizeof(struct mptcp_header));
  comms->header->dest_addr = *(servaddr);
  comms->header->src_addr = *(clientaddr);
  comms->header->seq_num = 1;
  comms->header->ack_num = 1;
  comms->header->total_bytes = strlen(data);


  while(true){

    if(ph->success == 1){
      pthread_exit(NULL);
    }

    pthread_mutex_lock(&ph->lock);
    if(ph->unacked < RWIN){
      if(cn->packsOut < cn->cwnd){
        printf("MYPID %d     Data Index %d\n", (int) pthread_self(), ph->dataIndex);

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

        senddata(cn, comms, size);

        qnode* newAck = (qnode*) malloc(sizeof(qnode));
        newAck->size = size;
        newAck->seqNum = comms->header->seq_num;
        newAck->next = NULL;

        iterator = cn->packets->root;
        while(iterator->next != NULL)
          iterator = iterator->next;

        iterator->next = newAck;

      }
    }
    pthread_mutex_unlock(&ph->lock);

    if(mp_recv(cn->sd, comms, MSS, MSG_DONTWAIT) == -1){
      continue;
    }
    else{ //if a message was recieved:
      if(comms->header->ack_num == -1){
        ph->success = 1;
        pthread_exit(NULL); //file transfer complete
      }

      printf("MYPID %d    Recieved ack %d\n", (int) pthread_self(), comms->header->ack_num);

      printf("%d\t current list: \n", (int) pthread_self());
      qnode*  qn = cn->packets->root->next;
      while(qn != NULL){
        printf("%d\t\t %d \n", (int) pthread_self(), qn->seqNum);
        qn = qn->next;
      }

      iterator = cn->packets->root->next; //iterator into the sent queue

      //if the ack does not match the data after the queue's root (oldest packet)
      if((iterator->seqNum + iterator->size) > comms->header->ack_num){

        //reset the window and ssthresh
        cn->ssthresh = max(1, cn->cwnd/2);
        cn->cwnd = 1;
        cn->currentAcks = 0;

        //if dup ack is root, came from this subflow. retransmit
        if(iterator->seqNum == comms->header->ack_num){
          retransmit(ph, cn->index, comms->header->ack_num, BUFSIZE, data);
          printf("%d RETRANSMITTING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %d, %d\n",(int) pthread_self(), comms->header->ack_num, iterator->seqNum);
        }

        //if dup ack isnt root, sent on other subflow. this subflow's packet recieved, so free

          cn->packsOut--;
          ph->unacked -= (iterator->size + sizeof(struct mptcp_header));
          cn->packets->root->next = iterator->next;
          free(iterator);


      }
      else{
        iterator = cn->packets->root->next;
        ph->unacked -= (iterator->size + sizeof(struct mptcp_header));
        cn->packsOut--;
        cn->currentAcks++;
        cn->packets->root->next = iterator->next;
        free(iterator);
      }

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

    if(cn->cwnd >= cn->ssthresh){
      cn->congestionMode = additive;
    }

  }
  pthread_exit(NULL);
}


bool sendFile(pathHolder* ph, char* data){


  pthread_t PIDs[ph->numConns];
  threadInfo* args[ph->numConns];
  for(int i = 0; i < ph->numConns; i++){
    args[i] = (threadInfo*) malloc(sizeof(threadInfo));
    args[i]->ph = ph;
    args[i]->cn = &ph->conns[i];
    args[i]->data = data;

    args[i]->servaddr = ph->conns[i].servaddr;
    args[i]->clientaddr = ph->conns[i].clientaddr;
  }

  for(int i = 0; i < ph->numConns; i++){
    if(pthread_create(&PIDs[i], NULL, connThread, args[i])){
      printf("Error: creating pthread\n");
      exit(0);
    }
  }

  for(int j = 0; j < ph->numConns; j++){
    pthread_join(PIDs[j], NULL);
  }

  if(ph->success == 1){
    return(true);
  }
  else{
    return(false);
  }

}
