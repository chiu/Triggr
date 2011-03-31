struct _Connection;
typedef struct _Connection Connection;

struct _OutBuffer;
typedef struct _OutBuffer OutBuffer;

struct _WorkBuffer;
typedef struct _WorkBuffer WorkBuffer;

struct _InBuffer;
typedef struct _InBuffer InBuffer;

struct _OutBuffer{
 char* buffer;
 size_t size;
 size_t alrSent;
 int streamming;
 OutBuffer* nxt;
 OutBuffer* prv;
 Connection* c;
};


struct _InBuffer{
 char *buffer;
 size_t actualSize;
 size_t truSize;
 int state; //0-uncompleted, 1-finished, 2-error, 3-eof
};

struct _WorkBuffer{
 char* buffer;
 int working;
 Connection* c;
 int orphaned;
 WorkBuffer* nxt;
 WorkBuffer* prv;
 WorkBuffer* globalNxt;
 WorkBuffer* globalPrv;
};

struct _Connection{
 struct ev_io qWatch;
 struct ev_io aWatch;
 Connection* prv;
 Connection* nxt;
 OutBuffer* tailOut;
 OutBuffer* headOut;
 WorkBuffer* tailWork;
 WorkBuffer* headWork;
 int canRead;
 int canWrite;
 InBuffer IB;
 int ID;
}; 


int working=0;
int active=1;
int port;

int count=0;
int clients=0;
int curClients=0;

int acceptFd;

int processedJobs;

struct ev_loop *lp;
struct ev_async idleAgain;

//Block the main thread during nothing-to-be-processed
pthread_cond_t idleC=PTHREAD_COND_INITIALIZER;
pthread_mutex_t idleM=PTHREAD_MUTEX_INITIALIZER;

//Block the main thread until async event will not convert the result into OutBuffer
pthread_cond_t outSchedC=PTHREAD_COND_INITIALIZER;
pthread_mutex_t outSchedM=PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t gqM=PTHREAD_MUTEX_INITIALIZER;

Connection *lastDoneConnection;
char *lastResult;
int lastOrphaned;

//Global state object
struct{
 WorkBuffer* tailWork;
 WorkBuffer* headWork;
 Connection* tailCon;
 Connection* headCon;
 int curCon;
} GlobalQueue;


