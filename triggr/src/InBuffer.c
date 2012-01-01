/* Input buffer object implementation

   Copyright (c)2011,2012 Miron Bartosz Kursa
 
   This file is part of triggr R package.

 Triggr is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 Triggr is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 You should have received a copy of the GNU General Public License along with triggr. If not, see http://www.gnu.org/licenses/. */

void makeIB(InBuffer *ans){
 ans->buffer=malloc(inBufferInitSize);
 if(!ans->buffer){
  ans->state=2;
 }else{
  ans->actualSize=inBufferInitSize;
  ans->truSize=0;
  ans->state=0;
 }
}

inline void tryResize(InBuffer *b){
 if(b->actualSize==b->truSize){
  //Seems we need resize
  char *tmp;
  //This both enforced maxMessage length and handles OOM
  if((b->actualSize+=IN_BUFFER_GROW_SIZE)>maxMessageLength || !(tmp=realloc(b->buffer,b->actualSize))){
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
   /**/Rprintf("Error %d while reading: %s\n",errno,strerror(errno));
   b->state=2;return;
  }else{
   return;
  }}
  if(readed==0){
   //EOF
   free(b->buffer);b->buffer=0;
   b->state=3;return;
  }
 }
}

inline void freeIB(InBuffer *b){
 if(b->buffer) free(b->buffer);
}
