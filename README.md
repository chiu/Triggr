Triggr is an [R](http://r-project.org) package that transforms the regular session into a *worker mode*, so that it could realise job requests coming through a socket from other apps and stay opened the whole time (something like fastCGI, but with own, lightweight and low level protocol -- or one can just think of it as a yet another light RPC). Opposite to [Rserve](http://rosuda.org/Rserve/), it exchanges and processes arbitrary text messages instead of simply executing all the incoming data and sending back the results.

Triggr controls only one R process, delegating seeding/load-balancing/misbehaving-instance-collection to the master application. The control server (later called *trigger*) itself should be responsive all the time, regardless if the worker is busy or not -- this is going to be achieved by forking it into a pthread thread and and making it a non-blocking solution.

Triggr is built around [libev](http://software.schmorp.de/pkg/libev.html), a light and fast event loop. [Current published version (0.2)](http://cran.r-project.org/web/packages/triggr/index.html) has this lib statically built in; development version (i.e. in this repository) requires libev to be installed on your system.

General idea, use-cased:
-----

7. Client connects, server accepts.
7. Client sends `\r\n\r\n`-terminated textual request, server stores it on queue.
  - If R is idle and there is something on queue, R callback is executed on the oldest request.
  - If R finishes a job, the textual result is sent back as `\r\n\r\n`-terminated string. Callback can also request termination of connection (for instance when error occurs).
  - If client terminates the connection, all queued jobs get removed from queue. Running job is not going to be cancelled because I suppose there is no good way of cleanly breaking it.
  - Client may send arbitrary large number of requests at random times; the server will not block them because of R activity or not-finished read.
  - Error in callback will result in (? sending pre-defined response to the client -- currently not in plans; may be emulated by `try()` in callback) and breaking connection.
7. Once started, Trigger will break only on callback returning terminate request.

This software is quite usable, but is still in development. For plans and progress, see [Issues](https://github.com/mbq/Triggr/issues).

