/* Trigger thread

   Copyright (c)2011,2012 Miron Bartosz Kursa
 
   This file is part of triggr R package.

 Triggr is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 Triggr is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 You should have received a copy of the GNU General Public License along with triggr. If not, see http://www.gnu.org/licenses/. */

static void cbIdleAgain(struct ev_loop *loop,ev_async *this,int revent){
 pthread_mutex_lock(&gqM);
 //Put the output on the write queue of the connection
 if(active && !lastOrphaned && !termCon) makeOutputBuffer(lastResult,lastDoneConnection,killConnectionAftrSend);//This also starts writer watchers
 if(!active) ev_break(lp,EVBREAK_ONE);
 if(termCon) killConnection(lastDoneConnection);
 if(lastResult) free(lastResult); //Malloc'ed in code.c
 
 pthread_mutex_unlock(&gqM);
 
 //Allow new jobs to be processed
 pthread_mutex_lock(&outSchedM);
 pthread_cond_signal(&outSchedC);
 pthread_mutex_unlock(&outSchedM);
}

static void onTim(struct ev_loop *lp,ev_timer *this,int revents){
 Rprintf("Triggr is alive; %d jobs form %d clients processed, %d in processing now, %d clients connected.\n",processedJobs,clients,working,curClients);
}

ev_signal signalWatcherINT;
ev_signal signalWatcherHUP;

static void onSig(struct ev_loop *lp, struct ev_signal *w, int revents){
 if(w->signum==SIGINT)
  Rprintf("Caught SIGINT, trying to exit...\n");
  else
  Rprintf("Caught SIGHUP, exitting...\n");
  
 ev_signal_stop(lp,&signalWatcherHUP);
 ev_signal_stop(lp,&signalWatcherINT);
 sigEnd=1;
 active=0;
 pthread_mutex_lock(&idleM);
 if(!working){
   //Make R stop waiting for work and exit serve
   pthread_cond_signal(&idleC);
 }//Else, everything is probably screwed already since R caught SIGINT;
  //if not, everything will exit nicely after orphaned job is done.
 pthread_mutex_unlock(&idleM); 
 //ev_unloop(lp,EVUNLOOP_ALL);
}

void* trigger(void *arg){
 lp=ev_loop_new(EVFLAG_AUTO);
 ev_async_init(&idleAgain,cbIdleAgain);
 ev_async_start(lp,&idleAgain);
 
 //Wait for the R part to go into idleLock
 pthread_mutex_lock(&idleM);
 pthread_mutex_unlock(&idleM); 
 
 //Installing server-is-alive timer
 ev_timer timer;
 ev_timer_init(&timer,onTim,aliveMessageStart,aliveMessageInter);
 ev_timer_start(lp,&timer);
 
 //Enable siginit; by default, it has the same mask as R thread, so -SIGPIPE -SIGINT -SIGHUP
 sigset_t triggerSet;
 sigemptyset(&triggerSet);
 sigaddset(&triggerSet,SIGINT);
 sigaddset(&triggerSet,SIGHUP);
 pthread_sigmask(SIG_UNBLOCK,&triggerSet,NULL);
 //Installing SIGINT watcher
 ev_signal_init(&signalWatcherINT,onSig,SIGINT);
 ev_signal_init(&signalWatcherHUP,onSig,SIGHUP);
 ev_signal_start(lp,&signalWatcherINT);
 ev_signal_start(lp,&signalWatcherHUP);


 //Installing accept watcher
 ev_io acceptWatcher;
 ev_io_init(&acceptWatcher,cbAccept,acceptFd,EV_READ);
 ev_io_start(lp,&acceptWatcher);

 //Run loop
 ev_run(lp,0);
 
 ev_signal_stop(lp,&signalWatcherHUP);
 ev_signal_stop(lp,&signalWatcherINT);
 //Clean up; we don't need any locks since main thread is waiting for a join now
 while(GlobalQueue.headCon!=NULL) killConnection(GlobalQueue.headCon);
 ev_loop_destroy(lp);
 return(NULL);
}
