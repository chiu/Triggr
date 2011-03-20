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
 GlobalQueue.tailWork=
 GlobalQueue.headWork=NULL;
 GlobalQueue.curCon=0;
 pthread_mutex_unlock(&gqM);
}

//Function running the trigger
SEXP startTrigger(SEXP port){
 R_CStackLimit=(uintptr_t)-1;
 active=1; count=0;
 port=INTEGER(port)[0];
 makeGlobalQueue();
 Rprintf("Initiating...\n"); 
 pthread_t thread;
 int rc;
 int processedJobs;
 Rprintf("Locking the idleLock for init...\n");
 pthread_mutex_lock(&idleM); //Hold the server from true staring 
 rc=pthread_create(&thread,NULL,trigger,NULL);
 Rprintf("Connecting socket...\n");
 //Initiate network interface
// int acceptFd;
 if((acceptFd=socket(AF_INET,SOCK_STREAM,0))<0) error("Cannot open listening socket!");
  
 struct sockaddr_in serverAddr;
 bzero((char*)&serverAddr,sizeof(serverAddr));
 serverAddr.sin_family=AF_INET;
 serverAddr.sin_addr.s_addr=INADDR_ANY; //Bind to localhost
 serverAddr.sin_port=htons(port);

 if(bind(acceptFd,(struct sockaddr*)&serverAddr,sizeof(serverAddr))<0) error("Cannot bind server!");

 //Starting listening for clients
 if(listen(acceptFd,MAX_CLIENTS)<0) error("Cannot listen with server!");
 Rprintf("Listening now on port %d...\n",port);
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
  //Dummy work 
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
