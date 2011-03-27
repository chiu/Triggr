
void killOutputBuffer(OutBuffer *o){
 printf("Output buffer %d being killed\n",o);
 Connection *c=o->c;
 free(o->buffer);
 if(o==c->tailOut) c->tailOut=o->prv;
 if(o==c->headOut) c->headOut=o->nxt;
 if(o->prv!=NULL) o->prv->nxt=o->nxt;
 if(o->nxt!=NULL) o->nxt->prv=o->prv;
 free(o);
 printf("OB kill done.\n");
}


OutBuffer *makeOutputBuffer(const char *what,Connection *c){
 //gqM is locked outside this function
 if(!c->canWrite) tryResolveConnection(c);
 printf("Scheduling new output with %s\n",what);
 size_t size=strlen(what);
 OutBuffer* OB=malloc(sizeof(OutBuffer));
 if(OB){
  OB->c=c;
  OB->buffer=malloc(size+10);
  if(OB->buffer){
   strcpy(OB->buffer,what);
   //Place on connection
   if(c->tailOut==NULL){
    c->tailOut=c->headOut=OB;
    OB->prv=OB->nxt=NULL;
   }else{
    OB->prv=c->tailOut;
    c->tailWork=OB;
    OB->prv->nxt=OB;
    OB->nxt=NULL;
   }
   ev_io_start(lp,&c->aWatch);
   return(OB);
  }else{
   //OOM
   c->canWrite=0;
   tryResolveConnection(c);
   return(NULL);
  }
 }else{
  //OOM
  c->canWrite=0;
  tryResolveConnection(c);
  return(NULL);
 }
 

}

