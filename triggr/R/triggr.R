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
    if(class(y)=="try-error") return(9L);
    if(!is.character(y)) return(9L);
    y<-paste(paste(y,collapse=""),"\r\n\r\n",sep="");
    return(y);
   },new.env());
}
