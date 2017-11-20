#include "mpsend.h"

//Sends the given packet on the given connection
//Packet data is of length len
int senddata(conn* cn, struct packet* pack, int len){

  int sent = mp_send(cn->sd, pack, len, 0);
  return(sent);

}

//Retransmit function for a dropped packet
//Arguments:
//  ph - pathholder of all connections
//  ind - index of path to avoid (path on which packet was originally sent)
//  seqNum - sequence number into the data where the resent data begins
//  size - length of data to send in packet
//  data - pointer to string to transmit
//Return:
//  # of bytes sent
int retransmit(pathHolder* ph, int ind, int seqNum, int size, char* data){

  //Calculates (semi-randomly) the path to retransmit the data on
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

  //Sets up a packet for the retransmitted data to be sent within
  struct packet* comms = (struct packet*) malloc(sizeof(struct packet));
  comms->data = (char*) malloc(sizeof(char) * BUFSIZE);
  comms->header = (struct mptcp_header*) malloc(sizeof(struct mptcp_header));
  comms->header->dest_addr = *(ph->conns[ind].servaddr);
  comms->header->src_addr = *(ph->conns[ind].clientaddr);
  comms->header->seq_num = seqNum;
  comms->header->ack_num = 1;
  comms->header->total_bytes = strlen(data);

  //Copy the data into the packet, and send it
  strncpy(comms->data, &data[seqNum - 1], size);
  int sent = senddata(&ph->conns[resendPath], comms, size);

  //Free the used packet
  free(comms->header);
  free(comms);

  //Returns the # of bytes sent
  return(sent);

}

//Central function that controls sending and recieving for a given connection
void* connThread(void* TI){

  /*****************************************************************************
	 * PARSING ARGUMENTS TO THE THREAD
	 */
  struct threadInfo* tInfo = TI;
  pathHolder* ph = tInfo->ph;
  conn* cn = tInfo->cn;
  char* data = tInfo->data;
  struct sockaddr_in* clientaddr = tInfo->clientaddr;
  struct sockaddr_in* servaddr = tInfo->servaddr;

  int size;  //Keeps track of how many bytes are going to be sent in current packet
  qnode* iterator; //Used to operate on unacked queue

  /*****************************************************************************
   * SETTING UP I/O PACKET
   */
  struct packet* comms = (struct packet*) malloc(sizeof(struct packet));
  comms->data = (char*) malloc(sizeof(char) * BUFSIZE);
  comms->header = (struct mptcp_header*) malloc(sizeof(struct mptcp_header));
  comms->header->dest_addr = *(servaddr);
  comms->header->src_addr = *(clientaddr);
  comms->header->seq_num = 1;
  comms->header->ack_num = 1;
  comms->header->total_bytes = strlen(data);

  /*****************************************************************************
	 * CONNECTION WHILE LOOP
	 */
  while(true){

    printf("Current transfer progress: %dB / %luB, %f%% \r", ph->dataIndex, strlen(data), 100 * (float)ph->dataIndex / (float)strlen(data));

    //If a thread recieved ack -1, file transfer complete
    if(ph->success == 1){
      pthread_exit(NULL);
    }

    /*****************************************************************************
  	 * SENDING PACKET (MUST LOCK OUT OTHER THREADS)
  	 */
    pthread_mutex_lock(&ph->lock);
    if(ph->unacked < RWIN){ //If the recieving window among all paths isnt full
      if(cn->packsOut < cn->cwnd){ //If this paths congestion window isnt full

        //find the maximum size we can send given the three contraints:
        //    1) cant send more than 84 bytes in a segment
        //    2) cant send more than RWIN - unacknowledged bytes, with room for header
        //    3) cant send more than the amount of data left to transfer
        size = min(min(BUFSIZE, RWIN - ph->unacked - sizeof(struct mptcp_header)), strlen(data) - ph->dataIndex);

        //clear and copy data into buffer
        memset(comms->data, 0, BUFSIZE);
        strncpy(comms->data, &data[ph->dataIndex], size);

        //set ack to 1, and the seq to the current data index + 1
        comms->header->ack_num = 1;
        comms->header->seq_num = ph->dataIndex + 1;

        ph->unacked += size + sizeof(struct mptcp_header); //update unacked bytes on all paths
        ph->dataIndex += size; //update current data position
        cn->packsOut++; //update number of unacked packets on this path

        //swap address information on I/O packet, and set the amount of data
        comms->header->dest_addr = *cn->servaddr;
        comms->header->src_addr = *cn->clientaddr;
        comms->header->total_bytes = strlen(data);

        //send the packet
        senddata(cn, comms, size);

        //create a new node in the unacked queue
        qnode* newAck = (qnode*) malloc(sizeof(qnode));
        newAck->size = size;
        newAck->seqNum = comms->header->seq_num;
        newAck->next = NULL;

        //iterate to end of list and insert new node
        iterator = cn->packets->root;
        while(iterator->next != NULL)
          iterator = iterator->next;
        iterator->next = newAck;

      }
    }
    pthread_mutex_unlock(&ph->lock);

    if(mp_recv(cn->sd, comms, MSS, MSG_DONTWAIT) == -1){ //if nothing to recieve:
      continue;
    }
    else{ //if a message was recieved:
      if(comms->header->ack_num == -1){ //ACK of -1 signifies transfer complete
        ph->success = 1;
        pthread_exit(NULL);
      }

      iterator = cn->packets->root->next; //iterator into the sent queue
      qnode* temp; //temporary node used for freeing nodes

      //free all nodes in unacked queue with seq# below the ack
      while((iterator != NULL) && (iterator->seqNum < comms->header->ack_num)){

        cn->currentAcks++;
        cn->packsOut--;
        ph->unacked -= (iterator->size + sizeof(struct mptcp_header));
        cn->packets->root->next = iterator->next;
        temp = iterator;
        free(temp);
        iterator = iterator->next;


      }

      /*
       * After getting rid of lower seq nums, if the ack matches the lowest seq
       * then it must be retransmitted. This prevents subflows that didnt drop
       * the packet from calling retransmit function.
       */
      if((iterator != NULL) && (iterator->seqNum == comms->header->ack_num)){

        //packet dropped, so we set ssthresh and cwnd
        cn->ssthresh = max(1, cn->cwnd/2);
        cn->cwnd = 1;
        cn->currentAcks = 0;
        cn->congestionMode = exponential;

        retransmit(ph, cn->index, comms->header->ack_num, BUFSIZE, data);

      }



    }

    /* If the current # of successful acks for this cwnd is the cwnd, then
     * we update cwnd depending on the current congestion mode
     */
    if(cn->currentAcks == cn->cwnd){
      if(cn->congestionMode == exponential){
        cn->cwnd = 2*cn->cwnd; //doubles if exponential
        cn->currentAcks = 0;
      }
      else{
        cn->cwnd++; //++ if additive
        cn->currentAcks = 0;
      }
    }

    //when the cwnd reaches ssthresh, we switch to additive
    if(cn->cwnd >= cn->ssthresh){
      cn->congestionMode = additive;
    }

  }

}

//Creates threads for each conn and calls connThread
bool sendFile(pathHolder* ph, char* data){

  //Setup thread holders, and setup arguments to connThread for each conn
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

  //For each conn, create a thread and pass arguments
  for(int i = 0; i < ph->numConns; i++){
    if(pthread_create(&PIDs[i], NULL, connThread, args[i])){
      printf("Error: creating pthread\n");
      exit(0);
    }
  }

  //After threads exit, process waits until they all join here
  for(int j = 0; j < ph->numConns; j++){
    pthread_join(PIDs[j], NULL);
  }

  //If ph->success set to 1 by a thread, transfer successful
  if(ph->success == 1){
    return(true);
  }
  //If not, transfer unsuccessful
  else{
    return(false);
  }

}
