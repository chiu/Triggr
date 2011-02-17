#include <R.h>
#include <Rdefines.h>
#include <Rinternals.h>
#include <unistd.h>
#include <pthread.h>
#define CSTACK_DEFNS 1
#include <Rinterface.h>
#include <stdint.h>



int working=0;
int active=1;

#define WORK_DUMMY 1
#define WORK_TRUE 2
int workType;

pthread_cond_t idleC=PTHREAD_COND_INITIALIZER;
pthread_mutex_t idleM=PTHREAD_MUTEX_INITIALIZER;

void* f(void *arg){
 //Wait for initialisation of R
 Rprintf("L: Lanista started; waiting for R to condWait...\n");
 pthread_mutex_lock(&idleM);
 pthread_mutex_unlock(&idleM);
 Rprintf("L: R is ready, I'm starting to listen!\n");

 for(int e=0;active;e++){

  if(e==20){
   //Stop request!
   active=0;
   //If it is doing work, than it will stop afterwards
   //Else, sending dummy job
   pthread_mutex_lock(&idleM);
   pthread_cond_signal(&idleC);
   pthread_mutex_unlock(&idleM);
  }

  Rprintf("L: New request!\n");
  //Process systemic requests
  if(working){
   Rprintf("L: Error, already working!\n");
  }else{
   Rprintf("L: Starting the job.\n");
   //Copying the input data
   Rprintf("L: Releasing idle lock.\n");
   pthread_mutex_lock(&idleM);
   pthread_cond_signal(&idleC);
   pthread_mutex_unlock(&idleM);
  }
  usleep(95000); //Simulate waiting for request
 }
 pthread_exit(NULL);
 return(NULL);
}

SEXP runPth(SEXP time){
 R_CStackLimit=(uintptr_t)-1;
 active=1;
 Rprintf("AAA\n");
 pthread_t thread;
 int rc;
 int processedJobs;
 Rprintf("AAA\n");
 pthread_mutex_lock(&idleM); //Hold the server from true staring 
 rc=pthread_create(&thread,NULL,f,NULL);
 usleep(2000000);//Simulate initialisation
 for(processedJobs=0;active;processedJobs++){
  Rprintf("Starting wait\n");
  //We musn't lock the mutex if it was already locked in init
  if(processedJobs>0) pthread_mutex_lock(&idleM);
  pthread_cond_wait(&idleC,&idleM);
  Rprintf("Lock de-locked\n");
  working=1;
  pthread_mutex_unlock(&idleM);

  Rprintf("Doing work...\n");
  usleep(500000);
  Rprintf("Done...\n");
  working=0;
  //Call the thread that the work is done
 }
 Rprintf("Detected that the server should not be active any more =(\n Done jobs: %d\n",processedJobs);


 Rprintf("Re-joining\n");
 pthread_join(thread,NULL);
 return(R_NilValue);
}
