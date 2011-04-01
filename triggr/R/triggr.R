getConID<-function(){
 .Call("getConID");
}

serve<-function(callback,port=7777L){
 stopifnot(is.function(callback));
 stopifnot(length(port)==1);
 as.integer(port)->port; 
 .Call("startTrigger",port,
   function(x){
    #DEBUG:
    if(x=="Q") return(0L);
    #:DEBUG
    try(callback(x))->y;
    if(is.integer(y)) return(y);
    if(!is.character(y) | class(y)=="try-error") return(9L);
    if(class(y)!="end-connection")
     y<-paste(paste(y,collapse=""),"\r\n\r\n",sep="") 
    else
     y<-c(paste(paste(y,collapse=""),"\r\n\r\n",sep=""),'');
    return(y);
   },new.env());
}

stopServer<-function() 0L

endConnection<-function(x){
 if(missing(x)) return(9L) else{
  class(x)<-"end-connection";
  return(x);
 }
}
