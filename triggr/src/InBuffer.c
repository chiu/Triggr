#define SIZE_LIMIT 1000000

void makeIB(InBuffer *ans){
 ans->buffer=malloc(512);
 if(!ans->buffer){
  ans->state=2;
 }else{
  ans->actualSize=512;
  ans->truSize=0;
  ans->state=0;
 }
}

inline void tryResize(InBuffer *b){
 if(b->actualSize==b->truSize){
  //Seems we need resize
  char *tmp;
  if((b->actualSize+=512)>SIZE_LIMIT || !(tmp=realloc(b->buffer,b->actualSize))){
   free(b->buffer);
   b->buffer=NULL;
   b->state=2; 
  }else{
   //We have memory
   b->buffer=tmp;
  }
 } 
}

inline int isRNRN(InBuffer *b){
 return(
  b->buffer[b->truSize-1]=='\n' &&
  (
   (b->truSize>1 && b->buffer[b->truSize-2]=='\n') //\n\n
    ||
   (b->truSize>3 && 
     b->buffer[b->truSize-2]=='\r' &&
     b->buffer[b->truSize-3]=='\n' &&
     b->buffer[b->truSize-4]=='\r') //\r\n\r\n
  )
 );
}

//The main streaming procedure, which fills buffer with new chars till RNRN 
//terminator is reached. The IB stays in 0 state as long as more data is needed.
//Error changes the state to 2 and frees the data (whole unfinished message is lost).
//The read is finished precisely on the end of RNRN, no overread happens.
//At succ. completion, state is 1 and buffer contains NULL-terminated string (though it may
// have larger size, thus freeIB destructor is needed).
//State 3 indicates the EOF; it is basically an error (no message delivered), but it is 
// an expected condition, since the special value.
void tillRNRN(int fd,InBuffer *b){
 while(1){
  int readed;
  readed=read(fd,b->buffer+b->truSize,1);
  if(readed>0){
   b->truSize++;
   if(isRNRN(b)){
    b->state=1;
    //Finishing and conversion to C-string
    if(b->buffer[b->truSize-2]=='\n') b->truSize-=2; else b->truSize-=4;
    b->buffer[b->truSize]=0;
    return;
   }
   tryResize(b);
   //The memory is exhausted (probably)
   if(b->state==2) return;
  }
  if(readed<0){if(errno!=EWOULDBLOCK){
   //Error
   free(b->buffer);b->buffer=0;
   /**/printf("Error while reading: %s\n",strerror(errno));
   b->state=2;return;
  }else{
   /**/printf("BLOCK!\n");
   return;
  }}
  if(readed==0){
   //EOF
   free(b->buffer);b->buffer=0;
   printf("--fin!\n");
   b->state=3;return;
  }
 }
}

inline void freeIB(InBuffer *b){
 if(b->buffer) free(b->buffer);
}
