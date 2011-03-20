struct _WorkBuffer{
 char* buffer;
 int working;
 Connection* c;
 WorkBuffer* globalNxt;
 WorkBuffer* globalPrv;
};

void killWorkBuffer(WorkBuffer *w){
 //TODO: Do
 /*Connection *c=w->c;
 free(w->buffer);
 if(w==c->)*/
}
