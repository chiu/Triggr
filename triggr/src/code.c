#include <R.h>
#include <Rdefines.h>
#include <Rinternals.h>
#include <unistd.h>
#include <pthread.h>

#define EV_STANDALONE 7
#include "ev.c"

#define HAVE_UINTPTR_T 7
#define CSTACK_DEFNS 7
#include <Rinterface.h>

#define MAX_CLIENTS 3
#define MAX_QSIZE 1024

int working=0;
int active=1;

int count=0;

int acceptFd;

#define WORK_DUMMY 1
#define WORK_TRUE 2

int workType;

struct ev_loop *lp;
struct ev_async idleAgain;

enum connectionStatus={QS_FRESH,QS_QUERY_READ,QS_ANS_WRITE,QS_BROKEN};

struct Connection{
 char *query;
 struct ev_io qWatch;
 struct ev_io aWatch;
 char *ans;
 size_t qLen;
 size_t aLen;
 int conType;
 enum connectionStatus status;
}; 

pthread_cond_t idleC=PTHREAD_COND_INITIALIZER;
pthread_mutex_t idleM=PTHREAD_MUTEX_INITIALIZER;

void fireTask(){
 pthread_mutex_lock(&idleM);
 if(working){
  //Set an error
  Rprintf("\t\tError; working already!\n");
 }else{
  //Signal about the work
  count++; if(count==20) active=0;
  Rprintf("\t\tFired new work!\n");
  pthread_cond_signal(&idleC);
 }
 pthread_mutex_unlock(&idleM); 
}

static void cbIdleAgain(struct ev_loop *loop,ev_async *this,int revent){
 Rprintf("\t\tInformed that job is done\n");
}

static void cbTimer(struct ev_loop *loop,ev_timer *this,int revents){
 Rprintf("\t\tNew request\n");
 fireTask();
 if(count==20) ev_break(loop,EVBREAK_ALL);
}

int readNum(int fd){
 int ans=0;
 char c;
 c='0';
 int cnt;
 while(c!=' '){
  ans*=10;
  ans+=c-'0';
  cnt=read(fd,&c,1);
  if(cnt!=1 || c<'0' || c>'9') return(-1);
 }
 return(ans);
}

//Connection destructor
void freec(struct connection* c){
 if(c->query!=NULL) free(c->query);
 if(c->ans!=NULL) free(c->ans);
 free(c);
}

static void cbRead(struct ev_loop *lp,ev_io *this,int revents){
 struct Connection *connection;
 connection=(struct connection*)this->data;
 
 if(connection->status==QS_FRESH){
  char hand[8];
  char vhand[]="triggr ";//plus one char for query

  int Q;
  Q=read(this->fd,&hand,8);         //hand[7] is one of A-E; Acquire status, Begin job, Cancel job, get Details, Exit worker
  if(Q!=8 || strncomp(hand,vhand,7) || 65>hand[7] || 69<hand[7]){
   //Bad connection
   ev_io_stop(lp,this);
   freec(connection);
   close(this->fd);
  }
  //We have a valid query; now time to identify it and perform action
  switch(hand[7]){
   case 'A':
    connection->aLen=1;
    connection->ans=malloc(1);
    if(working) ans='W'; else ans='I';
    ev_io_stop(lp,this);
    connection->conType=QS_ANS_WRITE;
    break;
   case 'B':
    connection->qLen=readNum(this->fd);
    if(connection->qLen<1 || connection->qLen>MAX_QSIZE){
     connection->conType=QS_BROKEN;
     break;  
    } 
    connection->conType=QS_QUERY_READ;    
    break;
   case 'C':
    //TODO: Write real handler
    connection->conType=QS_BROKEN;
    break;
   case 'D':
    connection->conType=QS_BROKEN;
    break;
   case 'E':
    exit(0);
  }
  if(connection->conType==QS_ANS_WRITE){
   //Start write callback
  }
  if(connection->conType==QS_BROKEN){
   ev_io_stop(lp,this);
   close(this->fd);
   freec(connection);
  }
  //Else, we just go and continue with reading
 }
 
 if(connection->status==QS_QUERY_READ){
  //TODO: Implement
 }
 
 
}

static void cbWrite(struct ev_loop *lp,ev_io *this,int revents){

}

static void cbAccept(struct ev_loop *lp,ev_io *this,int revents){
 struct sockaddr_in clientAddr;
 socklen_t cliLen=sizeof(clientAddr);
 int conFd;
 conFd=accept(w->fd,(struct sockaddr*)&(query->clientAddr),&cliLen);
 if(conFd<0) return;
 fcntl(conFd,F_SETFL,fcntl(conFd,F_GETFL,0)|O_NONBLOCK);
 
 struct Connection *connection; connection=malloc(sizeof(connection));
 //Otherwise there will be memory problems
 connection->ans=NULL;
 connection->query=NULL;
 connction->status=QS_FRESH;
 //So we have client accepted; let's hear what it wants to tell us
 ev_io_init(&connection->qWatcher,cbRead,conFd,EV_READ);
 connection->qWatcher.data=(void*)connection;
 ev_io_start(lp,&connection->qWatcher); 
} 


void Err(char *x){

}

void* trigger(void *arg){
 lp=ev_loop_new(EVFLAG_AUTO);
 ev_async_init(&idleAgain,cbIdleAgain);
 ev_async_start(lp,&idleAgain);
 
 //Wait for the R part to go into idleLock
 pthread_mutex_lock(&idleM);
 pthread_mutex_unlock(&idleM); 
 
 
 //Installing accept watcher
 ev_io acceptWatcher;
 ev_io_init(&acceptWatcher,cbAccept,acceptFd,EV_READ);
 ev_io_start(lp,&acceptWatcher);
 
 
 //ev_timer timer;
 //ev_timer_init(&timer,cbTimer,3.,1.);
 //ev_timer_start(lp,&timer);
 
 //Run loop
 ev_run(lp,0);
 
 //Clean up
 ev_loop_destroy(lp);
 return(NULL);
}

SEXP runPth(SEXP port){
 R_CStackLimit=(uintptr_t)-1;
 active=1; count=0;
 Rprintf("Initiating...\n");
 pthread_t thread;
 int rc;
 int processedJobs;
 Rprintf("Locking the idleLock for init...\n");
 pthread_mutex_lock(&idleM); //Hold the server from true staring 
 rc=pthread_create(&thread,NULL,trigger,NULL);
 Rprintf("Connection socket\n");
 //Initiate network interface
 int acceptFd;
 if((acceptFd=socket(AF_INET,SOCK_STREAM,0))<0) error("Cannot open listening socket!");
 
 struct sockaddr_in serverAddr;
 bzero((char*)&serverAddr,sizeof(serverAddr));
 serverAddr.sin_family=AF_INET;
 serverAddr.sin_addr.s_addr=INADDR_ANY; //Bind to localhost
 serverAddr.sin_port=htons(INTEGER(port)[0]);

 if(bind(acceptFd,(struct sockaddr*)&serverAddr,sizeof(serverAddr))<0) error("Cannot bind server!");

 //Starting listening for clients
 if(listen(acceptFd,MAX_CLIENTS)<0) error("Cannot listen with server!");
 Rprintf("Listening now...\n");
 //Libev will be binded to this interface in the trigger thread
 
 Rprintf("Init simulation START\n");
 
 usleep(2000000);//Simulate initialisation
 Rprintf("Init simulation STOP\n");
 for(processedJobs=0;active;processedJobs++){
  Rprintf("Starting wait\n");
  //We musn't lock the mutex if it was already locked in init
  if(processedJobs>0) pthread_mutex_lock(&idleM);
  pthread_cond_wait(&idleC,&idleM);
  Rprintf("Lock de-locked\n");
  working=1;
  pthread_mutex_unlock(&idleM);

  Rprintf("Doing work...\n");
  usleep(1500000);
  Rprintf("Done...\n");
  pthread_mutex_lock(&idleM);
  working=0;
  pthread_mutex_unlock(&idleM);
  ev_async_send(lp,&idleAgain);
  Rprintf("Idle status restored...\n");
  //Call the thread that the work is done
 }
 Rprintf("Detected that the server should not be active any more =(\n Done jobs: %d\n",processedJobs);


 Rprintf("Re-joining\n");
 pthread_join(thread,NULL);
 return(R_NilValue);
}
