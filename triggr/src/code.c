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




int working=0;
int active=1;

int count=0;

#define WORK_DUMMY 1
#define WORK_TRUE 2
int workType;

struct ev_loop *lp;

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

static void cbTimer(struct ev_loop *loop,ev_timer *this,int revents){
 Rprintf("\t\tNew request\n");
 fireTask();
 if(count==20) ev_break(loop,EVBREAK_ALL);
}

void* trigger(void *arg){
 lp=ev_loop_new(EVFLAG_AUTO);

 //Wait for the R part to go into idleLock
 pthread_mutex_lock(&idleM);
 pthread_mutex_unlock(&idleM); 
 
 ev_timer timer;
 ev_timer_init(&timer,cbTimer,3.,1.);
 ev_timer_start(lp,&timer);
 
 //Run loop
 ev_run(lp,0);
 
 //Clean up
 ev_loop_destroy(lp);
 return(NULL);
}

SEXP runPth(SEXP time){
 R_CStackLimit=(uintptr_t)-1;
 active=1; count=0;
 Rprintf("Initiating...\n");
 pthread_t thread;
 int rc;
 int processedJobs;
 Rprintf("Locking the idleLock for init...\n");
 pthread_mutex_lock(&idleM); //Hold the server from true staring 
 rc=pthread_create(&thread,NULL,trigger,NULL);
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
  Rprintf("Idle status restored...\n");
  //Call the thread that the work is done
 }
 Rprintf("Detected that the server should not be active any more =(\n Done jobs: %d\n",processedJobs);


 Rprintf("Re-joining\n");
 pthread_join(thread,NULL);
 return(R_NilValue);
}
