\name{serve}
\alias{serve}
\alias{callback}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{Starting the triggr server}
\description{
\code{serve} function converts the current session into a triggr server, binding it with a callback function that will handle the incoming requests.
}
\usage{
serve(callback,port=7777L,
      maxMessageLength=1048576L,aliveMessage=c(5,35))
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{callback}{Callback function. It should accept one parameter, a request as a 1-element character vector, and return a response to it either as a character vector or on output of \code{stopServer} or \code{endConnection} functions.}
  \item{port}{Port on which triggr server should listen. Will be coerced to integer.} 
  \item{maxMessageLength}{Maximal length of a single incoming message, in characters. Trigger will allocate at most this number of bytes, R can take extra up to this value, depending on internal hash state. Defaults to 1MB, should not exceed 1/5 of total RAM.} 
  \item{aliveMessage}{If set to two numbers, triggr will dump short status message \code{aliveMessage[0]} seconds after start and then every \code{aliveMessage[1]} seconds. If \code{NULL}, alive message will be disabled.}
}

\details{After this function is executed, the server is initiated and the console input is blocked.

Incoming requests are queued and executed sequentially in an order of their receive time (this means that requests from two simultaneously served clients may interchange -- yet the control over the origin of a request can be achieved by using connection IDs, see \code{\link{getConID}} function). The request is executed by passing it to the callback function defined by callback parameter. 

The function's environment is different for each request, but its parent environment is still the same (namely it is a child environment of the one in which \code{serve} was called)  -- this is essentially the same situation as if the callback function was mapped over all requests in a \code{lapply} loop.

If the result of the callback is a character vector (many-element character vectors are combined to 1-element with \code{\\r\\n}s ), it is terminated with \code{\\r\\n\\r\\n} and send as a response to the appropriate client.

Callback may also return a special value, obtained with either by a call to the \code{stopServer} or \code{endConnection} functions, to respectively stop the triggr server and return to interactive R session or terminate the connection with current client. Consult those functions' manual for more details.

If the function throws error or returns with a different value the server will break the current connection and continue working. 

Triggr is non-blocking, and thus it may process one request, receive and queue many requests and send many responses simultaneously.
}

\note{
In the current version, triggr does not validate whether a response is correctly formatted (i.e. does not contain \code{\\r\\n\\r\\n}s inside) -- this is intentional, because one may than consider using this feature to adapt triggr to other protocols or simply transfer raw data without any sanitising encoding (like base64) using socket close as a communication terminator.  
However, ensuring correct formatting is encouraged when dealing with triggr clients, which will be obviously confused by misformatted input and most probably treat this fact as a communication error.

If the message exceeds \code{maxMessageLength}, connection is closed and all not-send data is lost (this may randomly mean none or all of produced data because of the triggr's event-loop design -- thus treat exceeding this restriction like a connection fail, not a single message fail).
Possible running callback will continue, yet its output will be ignored.
That minimal \code{maxMessageLength} is 5, i.e. 1 character + 4, the size of \code{\\r\\n\\r\\n}.
Note that this value has nothing to do with the size of messages generated and sent by triggr, which is arbitrary not counting memory constrain. 

If the RAM is exhausted from some reason, trigger will try to survive by closing connections which are receiving at that time and not accepting new. 
However, note that the R thread is likely to kill current computation with allocation error in such condition, and the whole process can be killed by OOM killer.

The result of R crash during callback function are undefined; probably the server will also crash, but in very rare cases the server will continue to accept and queue requests till the memory will be exhausted. 

One can shut down the server by sending SIGHUP to the R process; if there is no message in processing at that time, R will return to REPL state immediately, otherwise it will wait for it to be finished before returning. 
SIGINT (also generated via ctrl-C) will work in the same manner, yet, due to R interpreter construction, the results of receiving this signal during message processing are undefined -- most likely the whole process will terminate, though it may also stuck in some esoteric state. 
Therefore the use of this signal is strongly discouraged. 

There are no time-outs defined. This allows a client to COMET-like wait for even a very long computation but obviously creates a risk of orphaned connections.
}


\author{
Miron B. Kursa <M.Kursa@icm.edu.pl>
}

\examples{
\dontrun{
#Run echo server on :2222
#You may use `telnet localhost 2222` to test it;
#remember to hit ENTER twice to generate RNRN and submit.
#'Q' input will terminate triggr server
serve(function(x){
 if(identical(x,"Q")) return(stopServer())
 return(x);
},2222) 
}

\dontrun{
#Simple HTTP server
#Point your browser to localhost:8080/N
#to get N random numbers. Browse to
#localhost:8080/Q to quit
serve(function(x){
 #Getting URL
 textConnection(x)->tc;
 w<-readLines(tc);
 close(tc);
 sub('/','',strsplit(w,' ')[[1]][2])->url;
 if(identical(url,"Q")) return(stopServer())
 N<-as.numeric(url);
 if(any(is.na(N))) 
  return(endConnection("HTTP/1.0 404 Not Found\r\n\r\n404!"));
 return(endConnection(
  sprintf("HTTP/1.0 200 OK\r\n\r\n\%d random numbers={\%s}",
  N,paste(runif(N),collapse=" "))))
},8080)
}

   

\dontrun{
#Simple telnet interface to R
#One should never use such server because it is a security hole.
#Issue telnet localhost 9999 and just start using R; 
#remember to hit ENTER twice to generate RNRN and submit.
working<-new.env();
serve(function(x){
 if(identical(x,"Q")) return(stopServer());
 try(eval(parse(text=strsplit(x,split="\r\n")[[1]]),working))->b;
 if("try-error"\%in\%class(b)) return("Error!");
 #Getting print output
 textConnection(NULL,'w')->output;
 sink(output);print(b);sink();
 textConnectionValue(output)->ans;
 close(output);
 #Send it back
 return(ans);
},9999) 
}
}
