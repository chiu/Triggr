
void killWorkBuffer(WorkBuffer *w){
 Connection *c=w->c;
 //This function must be called with globalQueue mutex locked
 //Anyway, remove from connection queue
 if(w->c){
  if(w==w->c->tailWork) w->c->tailWork=w->prv;
  if(w==w->c->headWork) w->c->headWork=w->nxt;
  if(w->prv!=NULL) w->prv->nxt=w->nxt;
  if(w->nxt!=NULL) w->nxt->prv=w->prv;   
 }
 if(w->working){
  //Buffer is now processed; this way we can only make it a ghost that will exist
  //to inform trigger to throw out the worked out result. 
  Rprintf("Currently ongoing work was orphaned by client\n");
  w->orphaned=1;
  w->c=0;
 }else{
  //It is not orphaned, remove from globalQueue
  if(w==GlobalQueue.tailWork) GlobalQueue.tailWork=w->globalPrv;
  if(w==GlobalQueue.headWork) GlobalQueue.headWork=w->globalNxt;
  if(w->globalPrv!=NULL) w->globalPrv->globalNxt=w->globalNxt;
  if(w->globalNxt!=NULL) w->globalNxt->globalPrv=w->globalPrv; 
  if(w->buffer) free(w->buffer);
  free(w);
 }
}
