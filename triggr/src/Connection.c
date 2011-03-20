

inline void rolloutOutBuffers(Connection* c){
 while(c->tailOut!=NULL) killOutputBuffer(c->tailOut);
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
    ev_io_stop(lp,&c->qWatch);
    ev_io_stop(lp,&c->aWatch);
    close(c->qWatch.fd);
    freeIB(&c->IB);
    free(c);
   }else{
    printf("Read still possible\n");
   } //else we can still get some message, so nothing is needed
  }else{
   printf("--b\n");
   //In theory we have something to send; but can we?
   if(!c->canWrite){
    rolloutOutBuffers(c);
    printf("Some-writes-lost exit of connection #%d",c->ID);
    ev_io_stop(lp,&c->qWatch);
    ev_io_stop(lp,&c->aWatch);
    close(c->qWatch.fd);
    freeIB(&c->IB);
    free(c);
   } //else we still can write, so nothing is needed
   printf("--c\n");
  }
 }else{
  printf("Still work to do... Can't stop without it\n");
 }
}

//This function copies its Null-terminated string what argument, so the
// original should be somehow removed manually.
void scheduleWork(const char *what,Connection* c){
 printf("Scheduling new work for %s\n",what);
 size_t size=strlen(what);
 WorkBuffer* WB=malloc(sizeof(WorkBuffer));
 if(WB){
  WB->c=c;
  WB->buffer=malloc(size+10);
  if(WB->buffer){
   WB->size=size;
   strcpy(WB->buffer,what);
   //Place on connection
   //Place on globalQueue
   pthread_mutex_lock(&gqM);
   //TODO: Continue HERE
   //In general implement all the work queue stack and problems with its connection object problems
   pthread_mutex_unlock(&gqM);
   //Fire if idling
   pthread_mutex_lock(&idleM);
   if(!working){
    //Signal about the work
    count++; if(count==20) active=0;
    Rprintf("\t\tFired new work!\n");
    pthread_cond_signal(&idleC);
   }
   pthread_mutex_unlock(&idleM); 
  }//Else scheduling just failed because of OOM
 }//Else ditto
}

static void cbRead(struct ev_loop *lp,struct ev_io *this,int revents){
 Connection *c;
 c=this->data;
 tillRNRN(this->fd,&(c->IB));
 if(c->IB.state==1){
  printf("Read: %s\n",c->IB.buffer);
  scheduleWorkBuffer(c->IB.buffer,c);
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
  tryResolveConnection(c);
  return;
 }
}

static void cbWrite(struct ev_loop *lp,ev_io *this,int revents){
 Connection* c;
 c=this->data;
 printf("W\n");
 if(c->headOut==NULL){
  //Queue is emty -- stop this watcher
  ev_io_stop(lp,this);
 }else{
  //Continue streaming outs
  OutBuffer* o;
  o=c->headOut;
  o->streamming=1;
  int written=write(this->fd,o->buffer+o->alrSent,o->size-o->alrSent);
  if(written<0){
   if(errno!=EAGAIN){
    printf("Error while reading: %s\n",strerror(errno));
    c->canWrite=0;
    printf("Do resolveWE\n");
    tryResolveConnection(c);
   }
  }else{
   o->alrSent+=written;
   if(o->alrSent==o->size){
    killOutputBuffer(o);
    printf("Do resolveWW\n");
    tryResolveConnection(c);
   }
  }
 } 
}

static void cbAccept(struct ev_loop *lp,ev_io *this,int revents){
 Rprintf("_B1\n");
 struct sockaddr_in clientAddr;
 socklen_t cliLen=sizeof(clientAddr);
 int conFd;
 conFd=accept(this->fd,(struct sockaddr*)&(clientAddr),&cliLen);
 if(conFd<0) return;
 fcntl(conFd,F_SETFL,fcntl(conFd,F_GETFL,0)|O_NONBLOCK);
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
 pthread_mutex_unlock(&gqM);
 
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
} 

