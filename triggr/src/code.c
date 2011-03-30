#include <R.h>
#include <Rdefines.h>
#include <Rinternals.h>
#include <unistd.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h> 
#include <linux/in.h>
#include <sys/stat.h> 
#include <fcntl.h>  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h> 
 
#define EV_STANDALONE 7
#include "ev.c"

#define HAVE_UINTPTR_T 7
#define CSTACK_DEFNS 7  
#include <Rinterface.h>
  
#define MAX_CLIENTS 3
#define MAX_QSIZE 1024 

#include "defs.h"

//Request object
#include "InBuffer.c"
//Response object 
#include "OutBuffer.c"
//In-queue incarnation of request
#include "WorkBuffer.c"
//Connection object
#include "Connection.c"
//Trigger thread
#include "Trigger.c"



void makeGlobalQueue(){
 pthread_mutex_lock(&gqM);
 GlobalQueue.tailWork=NULL;
 GlobalQueue.headWork=NULL;
 GlobalQueue.curCon=0;
 pthread_mutex_unlock(&gqM);
}

void sigpipeHandler(int sig){
 //Do very nothing
}
 
//Function running the trigger
SEXP startTrigger(SEXP port){
 R_CStackLimit=(uintptr_t)-1;
 active=1; count=0;
 port=INTEGER(port)[0];
 makeGlobalQueue(); 
 Rprintf("Initiating TriggR...\n"); 
 pthread_t thread;
 int rc;
 
 //Ignore SIGPIPE
 void *oldSigpipe=signal(SIGPIPE,sigpipeHandler);
 //Initiate network interface
 if((acceptFd=socket(AF_INET,SOCK_STREAM,0))<0) error("Cannot open listening socket!");
  
 struct sockaddr_in serverAddr;
 bzero((char*)&serverAddr,sizeof(serverAddr));
 serverAddr.sin_family=AF_INET;
 serverAddr.sin_addr.s_addr=INADDR_ANY; //Bind to localhost
 serverAddr.sin_port=htons(port);

 if(bind(acceptFd,(struct sockaddr*)&serverAddr,sizeof(serverAddr))<0) error("Cannot bind server!");

 //Starting listening for clients
 if(listen(acceptFd,MAX_CLIENTS)<0) error("Cannot listen with server!");
 //Libev will be binded to this interface in the trigger thread
 
 pthread_mutex_lock(&idleM); //Hold the server from true staring 
 rc=pthread_create(&thread,NULL,trigger,NULL);
 Rprintf("Connecting socket...\n");
 
 
 Rprintf("Init simulation START\n");
 usleep(200000);//Simulate initialisation
 Rprintf("Init simulation STOP\n");
 
 Rprintf("Listening on port %d.\n",port);
 
 //Starting process loop
 for(processedJobs=0;active;){
  //We musn't lock the mutex if it was already locked in init 
  if(processedJobs>0) pthread_mutex_lock(&idleM);
  pthread_cond_wait(&idleC,&idleM);
  pthread_mutex_unlock(&idleM);  
       
  pthread_mutex_lock(&gqM); 
  while(GlobalQueue.headWork!=NULL){
   working=1;
   WorkBuffer *WB=GlobalQueue.headWork;
   WB->working=1;
   Connection *c=WB->c;
   pthread_mutex_unlock(&gqM);
  
   //TODO: Execute processing code on the GlobalQueue.headWork's contents
   //Dummy work  
   usleep(10);   
   
   //WORK DONE
   processedJobs++;
   //Locking gqM to update the global state 
   pthread_mutex_lock(&gqM);
   lastDoneConnection=c;
   char tmpResponse[]="CusCus\r\n\r\n"; 
   lastResult=malloc(strlen(&tmpResponse)+1);
   strcpy(lastResult,&tmpResponse);
   working=0;
   WB->working=0;
   lastOrphaned=WB->orphaned;
   killWorkBuffer(WB);
   pthread_mutex_unlock(&gqM);
   
   //Notifying Triggr to initiate the output sending
   pthread_mutex_lock(&outSchedM);
   ev_async_send(lp,&idleAgain);
   pthread_cond_wait(&outSchedC,&outSchedM);
   pthread_mutex_unlock(&outSchedM);
   
   pthread_mutex_lock(&gqM);
  }
  pthread_mutex_unlock(&gqM);
  
 }
 //Wait for trigger thread
 pthread_join(thread,NULL);
 //Restore sigpipe
 signal(SIGPIPE,oldSigpipe);
 
 Rprintf("Clean exit of TriggR. There was %d executed jobs.\n",processedJobs);
 return(R_NilValue);
}


//Some debug
/*void DumpWorkBuffs(Connection *c){
 WorkBuffer *iwb;
 Rprintf("Work buffs: ");
 if(c->headWork) for(iwb=c->headWork;iwb!=NULL;iwb=iwb->nxt) Rprintf("|%d|",iwb);
 Rprintf("\n");
}*/

/*void DumpOutBuffs(Connection *c,int j){
 OutBuffer *iob;
 Rprintf("Out buffs %d:",j);
 if(c->headOut) for(iob=c->headOut;iob!=NULL;iob=iob->nxt) Rprintf(":%d:",iob);
 Rprintf("\n");
}*/
