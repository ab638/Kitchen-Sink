/* A common header file to describe the shared memory we wish to pass about. */
      
#define TEXT_SZ 256

struct shared_use_st {
    int written_by_you;
    char some_text[TEXT_SZ];
};

const int MAXLEN = 256;

enum msgtype { SRVMSG=1, CLIMSG };

struct msg { long type; char data[MAXLEN]; };
  