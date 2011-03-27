struct _WorkBuffer{
 char* buffer;
 int working;
 Connection* c;
 int orphaned;
 WorkBuffer* nxt;
 WorkBuffer* prv;
 WorkBuffer* globalNxt;
 WorkBuffer* globalPrv;
};

void killWorkBuffer(WorkBuffer *w){
 puts("ZZ-A");
 printf("ZZ-AA %d\n",w);
 //This function must be called with globalQueue mutex locked
 //Anyway, remove from connection queue
 if(w->c){
  puts("ZZ-i");
  if(w==w->c->tailWork) w->c->tailWork=w->prv;
  puts("ZZ-j");
  if(w==w->c->headWork) w->c->headWork=w->nxt;
  puts("ZZ-k");
  if(w->prv!=NULL) w->prv->nxt=w->nxt;
  puts("ZZ-l");
  if(w->nxt!=NULL) w->nxt->prv=w->prv;   
  puts("ZZ-m");
 }
 
 if(working){
  //Buffer is now processed; this way we can only make it a ghost that will exist
  //to inform trigger to throw out the worked out result. 
  puts("ZZ-A2");
  w->orphaned=1;
 }else{
  //It is not orphaned, remove from globalQueue
  puts("ZZ-b");
  if(w==GlobalQueue.tailWork) GlobalQueue.tailWork=w->globalPrv;
  puts("ZZ-c");
  if(w==GlobalQueue.headWork) GlobalQueue.headWork=w->globalNxt;
  puts("ZZ-d");
  if(w->globalPrv!=NULL) w->globalPrv->globalNxt=w->globalNxt;
  puts("ZZ-e");
  if(w->globalNxt!=NULL) w->globalNxt->globalPrv=w->globalPrv;
  puts("ZZ-f");   
  if(w->buffer) free(w->buffer);
  puts("ZZ-g");
  free(w);
   puts("ZZ-h");
 }

}
