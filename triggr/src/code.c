/* Controller of the R session + additional C chunks

   Copyright (c)2011,2012 Miron Bartosz Kursa
 
   This file is part of triggr R package.

 Triggr is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 Triggr is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 You should have received a copy of the GNU General Public License along with triggr. If not, see http://www.gnu.org/licenses/. */

#include <R.h>
#include <Rdefines.h>
#include <Rinternals.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h> 
#include <fcntl.h>  
#include <stdlib.h>

#include "libev.h"

#include <errno.h>

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
 GlobalQueue.tailCon=
  GlobalQueue.headCon=NULL;
 GlobalQueue.curCon=0;
 pthread_mutex_unlock(&gqM);
}

void sigpipeHandler(int sig){
 //Do very nothing
}

SEXP getCID(){
 SEXP ans;
 if(!GlobalQueue.headWork) error("getConID() called outside callback!\n");
 PROTECT(ans=allocVector(INTSXP,1));
 INTEGER(ans)[0]=curConID;
 UNPROTECT(1);
 return(ans); 
}

//Function running the trigger
SEXP startTrigger(SEXP Node,SEXP Port,SEXP WrappedCall,SEXP Envir,SEXP MaxMessageLength,SEXP AliveTimes){
 R_SignalHandlers=0;
 R_CStackLimit=(uintptr_t)-1;
 active=1; count=0;
 const char *port,*node;
 //port=INTEGER(Port)[0];
 port=CHAR(STRING_ELT(Port,0));
 if(Node==R_NilValue)
	node=NULL; else node=CHAR(STRING_ELT(Node,0));
	
 makeGlobalQueue(); 
 pthread_t thread;
 int rc;
 
 if(length(AliveTimes)!=2){
	aliveMessageStart=0;
 }else{
	aliveMessageStart=REAL(AliveTimes)[0];
	aliveMessageInter=REAL(AliveTimes)[1];
 }
 
 
 maxMessageLength=INTEGER(MaxMessageLength)[0];
 inBufferInitSize=(maxMessageLength<MAX_IN_BUFFER_INIT_SIZE)?maxMessageLength:MAX_IN_BUFFER_INIT_SIZE;
 
 //Initiate network interface
 /*
 if((acceptFd=socket(PF_INET,SOCK_STREAM,0))<0) error("Cannot open listening socket!");

  
 struct sockaddr_in serverAddr;
 bzero((char*)&serverAddr,sizeof(serverAddr));
 serverAddr.sin_family=AF_INET;
 serverAddr.sin_addr.s_addr=INADDR_ANY; //Bind to all interfaces
 serverAddr.sin_port=htons(port);

 if(bind(acceptFd,(struct sockaddr*)&serverAddr,sizeof(serverAddr))<0) error("Cannot bind server!");
 */
 
 struct addrinfo hints,*addrInfo,*a;
 socklen_t sinSize;
 
 memset(&hints,0,sizeof(hints));
 hints.ai_family=AF_UNSPEC; //IPv4,v6,v7... don't care
 hints.ai_socktype=SOCK_STREAM;
 //TODO: Give a chance to specify interface
 if(1){
	hints.ai_flags=AI_PASSIVE;
 }
 if(getaddrinfo(node,port,&hints,&addrInfo)!=0)
	 error("Cannot obtain address to bind!");

 //Trying luck binding and listening
 int one=1;
 for(a=addrInfo;a!=NULL;a=a->ai_next){
	 if((acceptFd=socket(a->ai_family,a->ai_socktype,a->ai_protocol))<0){
		 //Let's try next option
		 continue;
	  }
	 if(setsockopt(acceptFd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one))<0){
		freeaddrinfo(addrInfo);
		error("Socket option set failed");
	}
	if(bind(acceptFd,a->ai_addr,a->ai_addrlen)<0){
		close(acceptFd);
		//Let's try next option
		continue;
	}
	//Ok, socket survived
	break;
 }
 if(a==NULL){
	 freeaddrinfo(addrInfo);
	 error("Socket binding failed!");
  }
 
 //Starting listening for clients
 if(listen(acceptFd,MAX_CLIENTS)<0) error("Cannot listen with server!");
 //Libev will be binded to this interface in the trigger thread
 
 //OK, network interface is up; now time to build thread structure
 
 active=1; 
 
 //Signal handling
 //Making R ignore SIGINT and SIGPIPE
 //in fact this works only when R is idle; 
 sigset_t newSet,oldSet;
 sigemptyset(&newSet); sigemptyset(&oldSet);
 sigaddset(&newSet,SIGINT);
 sigaddset(&newSet,SIGHUP);
 sigaddset(&newSet,SIGPIPE);
 pthread_sigmask(SIG_BLOCK,&newSet,&oldSet);
 sigEnd=0;
 
 pthread_mutex_lock(&idleM); //Hold the server from true staring 
 rc=pthread_create(&thread,NULL,trigger,NULL);
 
 Rprintf("Listening on port %s.\n",port);
 
 //Starting process loop
 for(processedJobs=0;active;){
  //We musn't lock the mutex if it was already locked in init 
  if(processedJobs>0) pthread_mutex_lock(&idleM);
  pthread_cond_wait(&idleC,&idleM);
  pthread_mutex_unlock(&idleM);  
    
  //There was a signal break and loop was idle
  if(sigEnd){
   //Locking gqM 
   pthread_mutex_lock(&gqM);
   lastResult=NULL;
   pthread_mutex_unlock(&gqM);
   //Notifying trigger to initiate self-destruct
   pthread_mutex_lock(&outSchedM);
   ev_async_send(lp,&idleAgain);
   //And wait till the idle callback ends
   pthread_cond_wait(&outSchedC,&outSchedM);
   pthread_mutex_unlock(&outSchedM);
   break;
  }
 
  pthread_mutex_lock(&gqM); 
  while(GlobalQueue.headWork!=NULL){
   working=1;
   WorkBuffer *WB=GlobalQueue.headWork;
   WB->working=1;
   termCon=0;
   Connection *c=WB->c;
   SEXP arg;
   PROTECT(arg=allocVector(STRSXP,1));
   SET_STRING_ELT(arg,0,mkChar(WB->buffer));
   curConID=c->ID;
   pthread_mutex_unlock(&gqM);
   
   //Execute processing code on the GlobalQueue.headWork's contents
   SEXP response;
   const char *responseC=NULL;
   SEXP call;
   PROTECT(call=lang2(WrappedCall,arg));
   PROTECT(response=eval(call,Envir));
   if(TYPEOF(response)!=STRSXP){
    if(TYPEOF(response)==INTSXP && LENGTH(response)==1){
     int respCode=INTEGER(response)[0];
     if(respCode==0) active=0; else termCon=1; 
     responseC=NULL;
    }else{
     error("PANIC: Bad callback wrapper result! Triggr will dirty-collapse now.\n");
    }
   }else{
    if(LENGTH(response)<0 || LENGTH(response)>2) error("PANIC: Bad callback wrapper result! Triggr will dirty-collapse now.\n");
    killConnectionAftrSend=(LENGTH(response)==2);
    responseC=CHAR(STRING_ELT(response,0));
   }
   UNPROTECT(3);
   
   //WORK DONE
   processedJobs++;
   //Locking gqM to update the global state 
   pthread_mutex_lock(&gqM);
   lastDoneConnection=c;
   if(active && !termCon){
    lastResult=malloc(strlen(responseC)+1);
    strcpy(lastResult,responseC);
   } else lastResult=NULL;
   working=0;
   WB->working=0;
   lastOrphaned=WB->orphaned;
   killWorkBuffer(WB);
   pthread_mutex_unlock(&gqM);
   //Notifying Triggr to initiate the output sending
   pthread_mutex_lock(&outSchedM);
   ev_async_send(lp,&idleAgain);
   //And wait till the idle callback ends
   pthread_cond_wait(&outSchedC,&outSchedM);
   pthread_mutex_unlock(&outSchedM);
   pthread_mutex_lock(&gqM);
  }
  pthread_mutex_unlock(&gqM);
  if(sigEnd) break;
 }
 //Wait for trigger thread
 pthread_join(thread,NULL);
 //Restore R's default signal handling
 pthread_sigmask(SIG_SETMASK,&oldSet,NULL);
 close(acceptFd);
 Rprintf("Clean exit of triggr. %d jobs were executed.\n",processedJobs);
 R_SignalHandlers=1;
 return(R_NilValue);
}
