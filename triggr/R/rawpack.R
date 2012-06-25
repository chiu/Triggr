
packRaw<-function(xRaw)
 sprintf("%d:%s",as.integer(length(xRaw)),.Call('stringizeRaw',xRaw))


unpackRaw<-function(pRaw)
 .Call('rawizeString',pRaw[1])

encodeObject<-function(x){
 rc<-rawConnection(raw(0),'wb')
 serialize(x,rc,ascii=FALSE)
 packRaw(rawConnectionValue(rc))->ans
 close(rc)
 ans
}

decodeObject<-function(cx){
 rc<-rawConnection(unpackRaw(cx),'rb')
 ans<-unserialize(rc)
 close(rc)
 ans
}
