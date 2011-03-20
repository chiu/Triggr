

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
 Rprintf("\t\t\tInformed that job is done\n");
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
