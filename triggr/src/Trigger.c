

static void cbIdleAgain(struct ev_loop *loop,ev_async *this,int revent){
 Rprintf("\t\t\tInformed that job is done\n");

 char currentResponse[]="CusCus\r\n\r\n\0";
 
 
 pthread_mutex_lock(&gqM);
 //Remove the work current work buffer
 //->BAD Connection *c=GlobalQueue.headWork->c;
 
 //Put the output on the write queue of the connection
 puts("Making new OB...");
 OutBuffer *ob;
 //This also starts writer watchers
 ob=makeOutputBuffer(&currentResponse,lastDoneConnection);
 printf("New OB is %d\n",ob);
   //free(currentResponse);TODO: Resolve it
 pthread_mutex_unlock(&gqM);
 
 //Allow new jobs to be processed
 pthread_mutex_lock(&outSchedM);
 pthread_cond_signal(&outSchedC);
 pthread_mutex_unlock(&outSchedM);
 
 
 //Connection* doneConnection=GlobalQueue.headWork->c;
 //TODO: Make out buffer with lastResult
 
}

static void onTim(struct ev_loop *lp,ev_timer *this,int revents){
 Rprintf("\t\tXXXXXX\n");
}

void* trigger(void *arg){
 Rprintf("_A1\n");
 lp=ev_loop_new(EVFLAG_AUTO);
 ev_async_init(&idleAgain,cbIdleAgain);
 ev_async_start(lp,&idleAgain);
 Rprintf("_A2\n");
 //Wait for the R part to go into idleLock
 pthread_mutex_lock(&idleM);
 pthread_mutex_unlock(&idleM); 
 Rprintf("_A3\n");
 
 //Installing accept watcher
 Rprintf("_A4\n");
 
 struct ev_timer timer;
 ev_timer_init(&timer,onTim,20.,20.);
 ev_timer_start(lp,&timer);
 
 ev_io acceptWatcher;
 ev_io_init(&acceptWatcher,cbAccept,acceptFd,EV_READ);
 ev_io_start(lp,&acceptWatcher);
 Rprintf("_A5\n");
 //Run loop
 ev_run(lp,0);
 Rprintf("_A6\n");
 //Clean up
 Rprintf("_A7\n");
 ev_loop_destroy(lp);
 return(NULL);
}
