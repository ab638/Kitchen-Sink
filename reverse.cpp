// File: reverse.cpp
// Project 5
#include <iostream>     // cerr
#include <string>       // string
#include <stdlib.h>     // exit
#include <unistd.h>     // shm_open
#include <sys/mman.h>   // mmap,munmap
#include <fcntl.h>      // for O_* constants
#include <mqueue.h>     // mqd_t, mq_open, mq_receive, mq_close, 
// mq_unlink
#include <string.h>     // strncpy
#include <semaphore.h>  // sem_t, sem_open, sem_close, sem_unlink,
// sem_post, sem_wait
#include <algorithm>    // std::reverse
#include "sh_com.h"     // TEXT_SZ, struct shared_use_st, msg, MAXLEN


//  Important for the naming
// ./argv0    argv1    argv2  argv3     argv4
// ./reverse /reverse /upper /shm_proj /msgq


using namespace std;

int main(int argc, char* argv[]) {


    string rcdstr;
    // argument names
    char const * rev_name = argv[1];
    char const * up_name = argv[2];
    char const * shm_name = argv[3];
    char const * mq_name = argv[4];
    //-----------------------------

    // semaphore setup
    sem_t *toReverse, *toUpper;
    if((toReverse = sem_open(rev_name, 0))==SEM_FAILED){
        cerr << "sem_open error" << endl;
        exit(1);
    }
    if((toUpper = sem_open(up_name, 0))==SEM_FAILED){
        cerr << "sem_open error" << endl;
        exit(1);
    }
    //-------------------------------

    // shared memory setup
    void *shared_memory = (void *)0;
    struct shared_use_st *shared_stuff;
    int shmfd = shm_open(shm_name, O_RDWR, 0666);
    if (shmfd == -1) {
        cerr << "Shared Memory Open Error: /shm_proj" << endl;
        exit(1);
    }
    if(ftruncate(shmfd, sizeof(struct shared_use_st)) == -1){
        cerr << "ftruncate error: shmfd" << endl;
        exit(1);
    }
    shared_memory = mmap(NULL, sizeof(struct shared_use_st),
                         PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    close(shmfd); // don't need this now
    shared_stuff = (struct shared_use_st*)shared_memory;
    //------------------------------------------------------------------

    // message queue
    mqd_t received = mq_open(mq_name, O_RDONLY, 0, 0); // open message queue
    if (received == -1) {
        cerr << "Message Queue Open Error" << endl;
        exit(1);
    }
    msg recd; // received message
    recd.type = CLIMSG;
    while (1) {
        if (mq_receive(received, (char*)&recd, sizeof(recd), 0) == -1) {
            cerr << "Message Queue Receive Error" << endl;
            exit(1);
        };
        rcdstr = recd.data; // get recieved data
        if (strstr(rcdstr.c_str(), "EXIT") != NULL) {
            strncpy(shared_stuff->some_text, rcdstr.c_str(), TEXT_SZ);
            sem_post(toReverse);
            sem_wait(toUpper); // wait for upper to close
            break;  // we are done here
        }

        reverse(rcdstr.begin(), rcdstr.end()); // reverse the string
        strncpy(shared_stuff->some_text, rcdstr.c_str(), TEXT_SZ); // copy to shared memory
        sem_post(toReverse);

        sem_wait(toUpper);
    }

    // unlink, and close
    munmap(shared_memory, sizeof(struct shared_use_st));
    shm_unlink(shm_name);
    sem_close(toReverse);
    sem_close(toUpper);
    sem_unlink(rev_name);
    sem_unlink(up_name);
    mq_unlink(mq_name);
    mq_close(received);
    //----------------------------------------------------

    exit(EXIT_SUCCESS);
}