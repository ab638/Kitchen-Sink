// File: upper.cpp
// Project 5
#include <iostream>    // cout, cerr
#include <string>      // string
#include <unistd.h>    // ftruncate
#include <sys/mman.h>  //mmap, munmap, shm_open
#include <fcntl.h>     // for O_* constants
#include <semaphore.h> // sem_t, sem_open, sem_close, sem_unlink,
                       // sem_post, sem_wait
#include <string.h>    // strstr
#include <algorithm>   // transform
#include <signal.h>    // SIGUSR1, kill
#include "sh_com.h"    // TEXT_SZ, struct shared_use_st, msg, MAXLEN


//  Important for the naming 
// ./argv0    argv1    argv2  argv3     
// ./upper   /reverse /upper /shm_proj 


using namespace std;

int main(int argc, char* argv[]) {
    int shmfd; // shared memory file descriptor
    int status1, status2; // status 1 and 2
    void * shared_memory = (void *)0;
    struct shared_use_st *shared_stuff;
    // semaphore setup
    sem_t *toUpper, *toReverse;
    char const * rev_name = argv[1];
    if((toReverse = sem_open(rev_name, 0))==SEM_FAILED){
        cerr << "sem_open error" << endl;
        exit(1);
    }
    char const * up_name = argv[2];
    if((toUpper = sem_open(up_name, 0))==SEM_FAILED){
        cerr << "sem_open error" << endl;
        exit(1);
    }

    // open the shared memory
    char const * shm_name = argv[3];
    shmfd = shm_open(shm_name, O_RDWR, 0666);
    if(shmfd == -1){
        cerr << "shm_open error" << endl;
        exit(1);
    }
    if(ftruncate(shmfd, sizeof(struct shared_use_st))==-1){
        cerr << "ftruncate error" << endl;
        exit(1);
    }
    // map the memory
    shared_memory = mmap(NULL, sizeof(struct shared_use_st),
                         PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

    close(shmfd); // close the fd because it isnt needed now
    while (1) {
        sem_wait(toReverse);
        shared_stuff = (struct shared_use_st*)shared_memory;
        string output = shared_stuff->some_text; // read from shared memory
        if (strstr(output.c_str(), "EXIT")!=NULL) {
            sem_post(toUpper);
            break;
        }
        sem_post(toUpper);
        // uppercase conversion
        transform(output.begin(), output.end(), output.begin(), ::toupper);
        cout << output << endl; // output to terminal
        memset(&output, '\0', sizeof(output));
        kill(getppid(), SIGUSR1); // send SIGUSR1
    }

    munmap(shared_memory, sizeof(struct shared_use_st));
    shm_unlink(shm_name);
    sem_unlink(rev_name);
    sem_unlink(up_name);
    sem_close(toUpper);
    sem_close(toReverse);
  
    exit(EXIT_SUCCESS);
}

