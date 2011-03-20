
void killOutputBuffer(OutBuffer *o){
 printf("Output buffer killed\n");
 Connection *c=o->c;
 free(o->buffer);
 if(o==c->tailOut) c->tailOut=o->prv;
 if(o==c->headOut) c->headOut=o->nxt;
 if(o->prv!=NULL) o->prv->nxt=o->nxt;
 if(o->nxt!=NULL) o->nxt->prv=o->prv;
 free(o);
}


