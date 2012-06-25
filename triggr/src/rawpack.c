SEXP stringizeRaw(SEXP Rvec){
 SEXP Ans;
 if(TYPEOF(Rvec)!=RAWSXP) error("You must provide a raw vector!\n");
 Rbyte *rvec=RAW(Rvec);
 uint32_t l=length(Rvec);
 uint32_t numBad=0;
 for(uint32_t e=0;e<l;e++)
  if(rvec[e]==0 || rvec[e]==10 || rvec[e]==13 || rvec[e]=='|') numBad++;
 
 uint32_t lOut=l+numBad;
 char *p=R_alloc(lOut,1);
 uint32_t ee=0;
 for(uint32_t e=0;e<l;e++){
   if(rvec[e]==0 || rvec[e]==10 || rvec[e]==13 || rvec[e]=='|'){
    p[ee]='|';ee++;
    switch(rvec[e]){
     case 0  :p[ee]='Z';break;
     case 10 :p[ee]='L';break;
     case 13 :p[ee]='F';break;
     case '|':p[ee]='P';break;
    }
   }else{
    p[ee]=rvec[e];
   }
   ee++;
 }
 PROTECT(Ans=NEW_CHARACTER(1));
 SET_STRING_ELT(Ans,0,mkCharLen(p,lOut));
 UNPROTECT(1);
 return(Ans);
}

SEXP rawizeString(SEXP Cvec){
 SEXP Ans;
 const char *cvec=CHAR(STRING_ELT(Cvec,0));
 uint32_t e=0;
 uint32_t len=0;
 uint32_t ee=0;
 while(cvec[e] && cvec[e]!=':'){
  len=len*10+(cvec[e]-'0');
  e++;
 }
 if(cvec[e]!=':') error("Bad header!");
 e++;
 
 PROTECT(Ans=allocVector(RAWSXP,len));
 Rbyte *rvec=RAW(Ans);
 while(cvec[e]){
  if(cvec[e]=='|'){
   e++;
   switch(cvec[e]){
    case 'Z':rvec[ee]=0;break;
    case 'L':rvec[ee]=10;break;
    case 'F':rvec[ee]=13;break;
    case 'P':rvec[ee]='|';break;
    default: error("Bad encoding!");break;
   }
  }else rvec[ee]=cvec[e];
  e++;
  ee++;
 }
 UNPROTECT(1);
 return(Ans);
}
