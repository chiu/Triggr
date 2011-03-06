#include <unistd.h>

#define EV_STANDALONE 7
#include "ev.c"

#include <sys/socket.h> 
#include <linux/in.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CLIENTS 3
#define MAX_QSIZE 1024

#include <assert.h>

int acceptFd;

#define WORK_DUMMY 1
#define WORK_TRUE 2

int workType;

struct ev_loop *lp;

#define SIZE_LIMIT 1000000

//TODO: Add some kind of session management -- new connection, new session ID

/* InBuffer class */

typedef struct{
 char *buffer;
 size_t actualSize;
 size_t truSize;
 int state; //0-uncompleted, 1-finished, 2-error, 3-eof
} InBuffer;

//typedef struct _xyz InBuffer;

void makeIB(InBuffer *ans){
 ans->buffer=malloc(512);
 if(!ans->buffer){
  ans->state=2;
 }else{
  ans->actualSize=512;
  ans->truSize=0;
  ans->state=0;
 }
}

/*inline void debugIB(struct InBuffer* b){
 int e;
 printf("DEB: ");
 for(e=0;e<b->actualSize;e++){
  printf("%c",b->buffer[e]);
 }
 printf("\n");
}*/

inline void tryResize(InBuffer *b){
 if(b->actualSize==b->truSize){
  //Seems we need resize
  char *tmp;
  if((b->actualSize+=512)>SIZE_LIMIT || !(tmp=realloc(b->buffer,b->actualSize))){
   free(b->buffer);
   b->buffer=NULL;
   b->state=2; 
  }else{
   //We have memory
   b->buffer=tmp;
  }
 } 
}

inline int isRNRN(InBuffer *b){
 return(
  b->buffer[b->truSize-1]=='\n' &&
  (
   (b->truSize>1 && b->buffer[b->truSize-2]=='\n') //\n\n
    ||
   (b->truSize>3 && 
     b->buffer[b->truSize-2]=='\r' &&
     b->buffer[b->truSize-3]=='\n' &&
     b->buffer[b->truSize-4]=='\r') //\r\n\r\n
  )
 );
}

//The main streaming procedure, which fills buffer with new chars till RNRN 
//terminator is reached. The IB stays in 0 state as long as more data is needed.
//Error changes the state to 2 and frees the data (whole unfinished message is lost).
//The read is finished precisely on the end of RNRN, no overread happens.
//At succ. completion, state is 1 and buffer contains NULL-terminated string (though it may
// have larger size, thus freeIB destructor is needed).
//State 3 indicates the EOF; it is basically an error (no message delivered), but it is 
// an expected condition, since the special value.
void tillRNRN(int fd,InBuffer *b){
 while(1){
  int readed;
  readed=read(fd,b->buffer+b->truSize,1);
  if(readed>0){
   b->truSize++;
   if(isRNRN(b)){
    b->state=1;
    //Finishing and conversion to C-string
    if(b->buffer[b->truSize-2]=='\n') b->truSize-=2; else b->truSize-=4;
    b->buffer[b->truSize]=0;
    return;
   }
   tryResize(b);
   //The memory is exhausted (probably)
   if(b->state==2) return;
  }
  if(readed<0){if(errno!=EWOULDBLOCK){
   //Error
   free(b->buffer);b->buffer=0;
   /**/printf("Error while reading: %s\n",strerror(errno));
   b->state=2;return;
  }else{
   /**/printf("BLOCK!\n");
   return;
  }}
  if(readed==0){
   //EOF
   free(b->buffer);b->buffer=0;
   printf("--fin!\n");
   b->state=3;return;
  }
 }
}

inline void freeIB(InBuffer *b){
 if(b->buffer) free(b->buffer);
}
/******************************/

/* Queuee classes */
struct _Connection;
typedef struct _Connection Connection;

struct _OutBuffer;
typedef struct _OutBuffer OutBuffer;

struct _WorkBuffer;
typedef struct _WorkBuffer WorkBuffer;

struct _Connection{
 struct ev_io qWatch;
 struct ev_io aWatch;
 Connection* prv;
 Connection* nxt;
 OutBuffer* tailOut;
 OutBuffer* headOut;
 WorkBuffer* tailWork;
 WorkBuffer* headWork;
 int canRead;
 int canWrite;
 InBuffer IB;
 int ID;
}; 

struct _OutBuffer{
 char* buffer;
 size_t size;
 size_t alrSent;
 int streamming;
 OutBuffer* nxt;
 OutBuffer* prv;
 Connection* c;
};

struct _WorkBuffer{
 char* buffer;
 int working;
 Connection* c;
 WorkBuffer* globalNxt;
 WorkBuffer* globalPrv;
};

/* Global Queue object */

struct{
 WorkBuffer* tailWork;
 WorkBuffer* headWork;
 Connection* tailCon;
 Connection* headCon;
 int curCon;
} GlobalQueue;

void makeGlobalQueue(){
 GlobalQueue.tailWork=
 GlobalQueue.headWork=NULL;
 GlobalQueue.curCon=0;
}

//This function copies its Null-terminated string what argument, so the
// original should be somehow removed manually.
void scheduleOutBuffer(const char *what,Connection* c){
 printf("Scheded new buffer for %s\n",what);
 size_t size=strlen(what)+4;
 OutBuffer* OB=malloc(sizeof(OutBuffer));
 if(OB){
  OB->c=c;
  //TODO: Fix it to fit proper size (probably only +1)
  OB->buffer=malloc(size+10);
  if(OB->buffer){
   OB->size=size;
   strcpy(OB->buffer,what);
   OB->buffer[size]=0;
   OB->buffer[size-1]='\n';
   OB->buffer[size-2]='\r';
   OB->buffer[size-3]='\n';
   OB->buffer[size-4]='\r';
   OB->alrSent=0;
   OB->streamming=0;
   if(c->tailOut==NULL){
    //This is the only output in connection
    OB->prv=OB->nxt=NULL;
    c->tailOut=c->headOut=OB;
   }else{
    c->tailOut->nxt=OB;
    OB->prv=c->tailOut;
    c->tailOut=OB;
    OB->nxt=NULL;
   }
   ev_io_start(lp,&c->aWatch);
  }else{
   free(OB);
  }
 }
}

void killOutputBuffer(OutBuffer *o){
 printf("Output buffer killed\n");
 Connection *c=o->c;
 free(o->buffer);
 if(o==c->tailOut) c->tailOut=o->prv;
 if(o==c->headOut) c->headOut=o->nxt;
 if(o->prv!=NULL) o->prv->nxt=o->nxt;
 if(o->nxt!=NULL) o->nxt->prv=o->prv;
 free(o);
}


/******************************/




inline void rolloutOutBuffers(Connection* c){
 while(c->tailOut!=NULL) killOutputBuffer(c->tailOut);
}
 
//inline void rolloutWorkBuffers(Connection* c){
// while(c->tailOut!=NULL)
//}

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

static void cbRead(struct ev_loop *lp,struct ev_io *this,int revents){
 Connection *c;
 c=this->data;
 tillRNRN(this->fd,&(c->IB));
 if(c->IB.state==1){
  printf("Read: %s\n",c->IB.buffer);
  scheduleOutBuffer(c->IB.buffer,c);
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
 if(c->headOut==NULL){
  //Queue is emty -- stop this watcher
  ev_io_stop(lp,this);
 }else{
  //Continue streaming outs
  OutBuffer* o;
  o->streamming=1;
  o=c->headOut;
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
} 





int main(int argc,char** argv){

 int acceptFd;
 assert((acceptFd=socket(AF_INET,SOCK_STREAM,0))>=0);
 
 struct sockaddr_in serverAddr;
 bzero((char*)&serverAddr,sizeof(serverAddr));
 serverAddr.sin_family=AF_INET;
 serverAddr.sin_addr.s_addr=INADDR_ANY; //Bind to localhost
 serverAddr.sin_port=htons(9998);

 assert(bind(acceptFd,(struct sockaddr*)&serverAddr,sizeof(serverAddr))>=0);

 //Starting listening for clients
 assert(listen(acceptFd,MAX_CLIENTS)>=0);

 //Libev will be binded to this interface in the trigger thread
 lp=ev_loop_new(EVFLAG_AUTO);
 
 //Installing accept watcher
 ev_io acceptWatcher;
 ev_io_init(&acceptWatcher,cbAccept,acceptFd,EV_READ);
 ev_io_start(lp,&acceptWatcher);
 
 //Run loop
 ev_run(lp,0);
 
 //Clean up
 ev_loop_destroy(lp);
 
 return 0;
}
