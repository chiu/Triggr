

inline void rolloutOutBuffers(Connection* c){
 while(c->headOut!=NULL) killOutputBuffer(c->headOut);
}

inline void rolloutWorkBuffers(Connection* c){
 printf("Rollout Work Buffers\n");
 while(c->headWork!=NULL){
   printf("Killing now=%d\n",c->headWork);
   killWorkBuffer(c->headWork);
  }
}


inline void killConnection(Connection *c){
 printf("Killing connection #%d",c->ID);
 //Killing connection removes all its outBuffers (even uncompleted, but those have no place to be transfered now)
 puts("Q-a");
 rolloutOutBuffers(c);
 //...and WorkBuffers; if one of them is the work currently done, it stays in memory with connection set to NULL yet removed from connection's work queue
 puts("Q-b");
 rolloutWorkBuffers(c);
 
 puts("Q-c");
 ev_io_stop(lp,&c->qWatch);
 puts("Q-d");
 ev_io_stop(lp,&c->aWatch);
 puts("Q-e");
 close(c->qWatch.fd);
 puts("Q-f");
 if(c==GlobalQueue.tailCon) GlobalQueue.tailCon=c->prv;
 if(c==GlobalQueue.headCon) GlobalQueue.headCon=c->nxt;
 puts("Q-fA");
 printf("C->prv=%d\n",c->prv);
 if(c->prv) c->prv->nxt=c->nxt;
 puts("Q-fB");
 if(c->nxt) c->nxt->prv=c->prv;
   puts("Q-g");
 freeIB(&c->IB);
 puts("Q-h");
 free(c);
}

void tryResolveConnection(Connection* c){
 printf("Resolving!\n");
 //Try to close and destroy connection if it is not needed
 if(c->headWork==NULL){
  printf("--a\n");
  //Ok, nothing is here to compute 
  if(c->headOut==NULL){
   printf("--aa\n");
   if(!c->canRead){
    printf("Clean exit of connection #%d\n",c->ID);
    killConnection(c);
   }else{
    printf("Read still possible\n");
   } //else we can still get some message, so nothing is needed
  }else{
   printf("--b\n");
   //In theory we have something to send; but can we?
   if(!c->canWrite){
    printf("Some-writes-lost exit of connection #%d\n",c->ID);
    killConnection(c);
   } //else we still can write, so nothing is needed
   printf("--c\n");
  }
 }else{
  printf("Still work to do... Can't stop without it\n");
  if(!c->canWrite){
   ev_io_stop(lp,&c->aWatch);
  }
  if(!c->canWrite){
   ev_io_stop(lp,&c->qWatch);
  }
 }
}

//This function copies its Null-terminated string what argument, so the
// original should be somehow removed manually.
void scheduleWork(const char *what,Connection* c){
 printf("Scheduling new work for %s\n",what);
 size_t size=strlen(what);
 WorkBuffer* WB=malloc(sizeof(WorkBuffer));
  printf("ZZ-Z: %d\n",WB);
 if(WB){
  WB->c=c;
  WB->working=0;
  WB->orphaned=0;
  WB->buffer=malloc(size+10);
  if(WB->buffer){
   //WB->size=size; REDUND
   //WB->done=0; REDUND
   strcpy(WB->buffer,what);
   pthread_mutex_lock(&gqM);
   //Place on connection
   if(c->tailWork==NULL){
    c->tailWork=c->headWork=WB;
    WB->prv=WB->nxt=NULL;
   }else{
    WB->prv=c->tailWork;
    c->tailWork=WB;
    WB->prv->nxt=WB;
    WB->nxt=NULL;
   }
   //Place on globalQueue
   if(GlobalQueue.tailWork==NULL){
    //Currently this is the only work
    GlobalQueue.tailWork=GlobalQueue.headWork=WB;
    WB->globalPrv=WB->globalNxt=NULL;
   }else{
    WB->globalPrv=GlobalQueue.tailWork;
    GlobalQueue.tailWork=WB;
    WB->globalPrv->globalNxt=WB;
    WB->globalNxt=NULL;
   }   
   //Queues/buffers updated
   pthread_mutex_unlock(&gqM);
   
   
   //Fire if idling
   pthread_mutex_lock(&idleM);
   if(!working){
    //Signal about the work
    Rprintf("\t\tFired new work!\n");
    pthread_cond_signal(&idleC);
   }//Else, this work will be fired on finishing currently done work
   pthread_mutex_unlock(&idleM); 
  }//Else scheduling just failed because of OOM
 }//Else ditto
    //TODO: Two above should kill the connection.
}

static void cbRead(struct ev_loop *lp,struct ev_io *this,int revents){
 Connection *c;
 c=this->data;
 tillRNRN(this->fd,&(c->IB));
 if(c->IB.state==1){
  printf("Read: %s\n",c->IB.buffer);
  scheduleWork(c->IB.buffer,c);
  freeIB(&c->IB);
  makeIB(&c->IB);
  return;
 }
 if(c->IB.state==2){
  close(this->fd);
 }
 if(c->IB.state>1){
  c->canRead=0;
  ev_io_stop(lp,this);
  printf("Do resolveRR\n");
  pthread_mutex_lock(&gqM);
  tryResolveConnection(c);
  pthread_mutex_unlock(&gqM);
  return;
 }
}

static void cbWrite(struct ev_loop *lp,ev_io *this,int revents){
 Connection* c;
 c=this->data;
 printf("W-a\n");
 if(c->headOut==NULL){
  //Queue is emty -- stop this watcher
  printf("W-b\n");
  ev_io_stop(lp,this);
 }else{
  //Continue streaming outs
  OutBuffer* o;
  printf("W-c\n");
  o=c->headOut;
  printf("W-d\n");
  o->streamming=1;
  printf("W-e, %s[%d-%d]\n",o->buffer,o->alrSent,o->size);
  int written=write(this->fd,o->buffer+o->alrSent,o->size-o->alrSent);
  printf("W-f\n");
  if(written<0){
   if(errno!=EAGAIN){
    printf("Error while writing: %d=%s\n",errno,strerror(errno));
    c->canWrite=0;
    printf("Do resolveWE\n");
    pthread_mutex_lock(&gqM);
    tryResolveConnection(c);
    pthread_mutex_unlock(&gqM);
   }
  }else{
   printf("W-X\n");
   o->alrSent+=written;
   if(o->alrSent==o->size){
    killOutputBuffer(o);
    printf("Do resolveWW\n");
    pthread_mutex_lock(&gqM);
    tryResolveConnection(c);
    pthread_mutex_unlock(&gqM);
   }
  }
 } 
 printf("W-z\n");
}

static void cbAccept(struct ev_loop *lp,ev_io *this,int revents){
 Rprintf("_B1\n");
 struct sockaddr_in clientAddr;
 socklen_t cliLen=sizeof(clientAddr);
 int conFd;
 conFd=accept(this->fd,(struct sockaddr*)&(clientAddr),&cliLen);
 if(conFd<0) return;
 if(fcntl(conFd,F_SETFL,fcntl(conFd,F_GETFL,0)|O_NONBLOCK)==-1){
  //Something wrong with connection, just skipping request
  close(conFd);
  return;  
 }
 Connection *connection; 
 connection=malloc(sizeof(Connection));
 if(!connection){
  //No memory, just skipping this request
  close(conFd);
  return;
 }
 
 //Put on GlobalQueue
 pthread_mutex_lock(&gqM);
 if(GlobalQueue.tailCon==NULL){
  //Currently this is the only connection
  GlobalQueue.tailCon=GlobalQueue.headCon=connection;
  connection->prv=connection->nxt=NULL;
 }else{
  connection->prv=GlobalQueue.tailCon;
  GlobalQueue.tailCon=connection;
  connection->prv->nxt=connection;
  connection->nxt=NULL;
 }
 connection->ID=GlobalQueue.curCon++;
 
 //Clear local queues
 connection->headOut=connection->tailOut=connection->headWork=connection->tailWork=NULL;
 connection->canRead=1;
 connection->canWrite=1;
 //Make IB for messages
 makeIB(&(connection->IB));
 if(!connection->IB.buffer){
  //Same as above
  free(connection);
  close(conFd);
  return;
 }
 //So we have client accepted; let's hear what it wants to tell us
 ev_io_init(&connection->qWatch,cbRead,conFd,EV_READ);
 connection->qWatch.data=(void*)connection;
 
 ev_io_init(&connection->aWatch,cbWrite,conFd,EV_WRITE);
 connection->aWatch.data=(void*)connection;
 
 ev_io_start(lp,&connection->qWatch); 
 //Make global queue acessible again
 pthread_mutex_unlock(&gqM);
} 

