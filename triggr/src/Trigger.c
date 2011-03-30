

static void cbIdleAgain(struct ev_loop *loop,ev_async *this,int revent){
 pthread_mutex_lock(&gqM);
 //Put the output on the write queue of the connection
 if(!lastOrphaned) makeOutputBuffer(lastResult,lastDoneConnection);//This also starts writer watchers
 
 free(lastResult);//Malloc'ed in code.c
 
 pthread_mutex_unlock(&gqM);
 
 //Allow new jobs to be processed
 pthread_mutex_lock(&outSchedM);
 pthread_cond_signal(&outSchedC);
 pthread_mutex_unlock(&outSchedM);
}

static void onTim(struct ev_loop *lp,ev_timer *this,int revents){
 Rprintf("TriggR is alive. %d jobs form %d clients processed, %d in processing now, %d clients connected.\n",processedJobs,clients,working,curClients);
}

void* trigger(void *arg){
 lp=ev_loop_new(EVFLAG_AUTO);
 ev_async_init(&idleAgain,cbIdleAgain);
 ev_async_start(lp,&idleAgain);
 
 //Wait for the R part to go into idleLock
 pthread_mutex_lock(&idleM);
 pthread_mutex_unlock(&idleM); 
 
 //Installing server-is-alive timer
 struct ev_timer timer;
 ev_timer_init(&timer,onTim,20.,20.);
 ev_timer_start(lp,&timer);

 //Installing accept watcher
 ev_io acceptWatcher;
 ev_io_init(&acceptWatcher,cbAccept,acceptFd,EV_READ);
 ev_io_start(lp,&acceptWatcher);

 //Run loop
 ev_run(lp,0);

 //Clean up
 ev_loop_destroy(lp);
 return(NULL);
}
