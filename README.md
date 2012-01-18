Triggr is an [R](http://r-project.org) package that transforms the regular session into a *worker mode*, so that it could realise job requests coming through a socket from other apps and stay opened the whole time (something like fastCGI, but with own, lightweight and low level protocol -- or one can just think of it as a yet another light RPC). Opposite to [Rserve](http://rosuda.org/Rserve/), it exchanges and processes arbitrary text messages instead of simply executing all the incoming data and sending back the results.

Triggr controls only one R process, delegating seeding/load-balancing/misbehaving-instance-collection to the master application. The control server (later called *trigger*) itself should be responsive all the time, regardless if the worker is busy or not -- this is going to be achieved by forking it into a pthread thread and and making it a non-blocking solution.

Triggr is built around [libev](http://software.schmorp.de/pkg/libev.html), a light and fast event loop. 

General idea, use-cased:
-----

7. Client connects, server accepts.
7. Client sends `\r\n\r\n`-terminated textual request, server stores it on queue.
  - If R is idle and there is something on queue, R callback is executed on the oldest request.
  - If R finishes a job, the textual result is sent back as `\r\n\r\n`-terminated string. Callback can also request termination of connection (for instance when error occurs).
  - If client terminates the connection, all queued jobs get removed from queue. Running job is not going to be cancelled because I suppose there is no good way of cleanly breaking it.
  - Client may send arbitrary large number of requests at random times; the server will not block them because of R activity or not-finished read.
  - Error in callback will result in breaking connection.
7. Once started, Trigger will break on callback returning terminate request or on SIGHUP (SIGINT should also work, but its handling may badly conflict with R).

Installation
-----

7. Install `libev` (usual package names: `libev`, `libev-devel`, `libev-dev`; [source](http://software.schmorp.de/pkg/libev.html);  version 3.0+, better 4.x) and `autoconfig`.
7. Run updpak.sh. 
7. `cleanPkg/` will now contain clean version of the package. `cd` there.
7. Run `R CMD INSTALL triggr` to install triggr (or `sudo R CMD...` to install for all users)
7. Run R, use `library(triggr)` to load the package and start playing with `serve`, the function that initiates the trigger mode.
7. General docs are under `?triggr`, examples are under `?serve`.


Examples
-----

* Echo server

        #Echo server on :2222
        #You may use `telnet localhost 2222` to test it;
        #remember to hit ENTER twice to generate RNRN and submit.
        #'Q' input will terminate triggr server
        serve(function(x){
         if(identical(x,"Q")) return(stopServer())
         return(x);
        },2222) 
        
* Simple HTTP server

        #Simple HTTP server
        #Point your browser at localhost:8080/N
        #to get N random numbers. 
        serve(function(x){
         #Getting URL
         textConnection(x)->tc;
         w<-readLines(tc);
         close(tc);
         sub('/','',strsplit(w,' ')[[1]][2])->url;
         N<-as.numeric(url);
         if(any(is.na(N))) 
          return(endConnection("HTTP/1.0 404 Not Found\r\n\r\n404!"));
         return(endConnection(
          sprintf("HTTP/1.0 200 OK\r\n\r\n%d random numbers={%s}",
          N,paste(runif(N),collapse=" "))))
        },8080)


This software is quite usable, but is still in development. For plans and progress, see [Issues](https://github.com/mbq/Triggr/issues).

