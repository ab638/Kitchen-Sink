// File: master.cpp
// Project 5
#include <iostream>    // cout, cerr, cin, cin.eof()
#include <string> 	   // string, copy
#include <string.h>    // memset
#include <stdlib.h>    // exit
#include <unistd.h>    // shm_open, close, 
#include <sys/wait.h>  // waitpid
#include <sys/mman.h>  // mmap, munmap
#include <fcntl.h> 	   // for O_* constants
#include <mqueue.h>    // mqd_t, mq_open, mq_send
#include <semaphore.h> // sem_t, sem_open, sem_unlink, sem_close
#include <signal.h>    // SIGUSR, pause, sigaction
#include "sh_com.h"    // TEXT_SZ, struct shared_use_st, msg, MAXLEN

using namespace std;

// signal hangler
void sighandler(int sig) {
	return;
	// now go back
}

// Main Program
int main() {
	// Initialize variables
	string userInput; // read from cin
	int shmfd; // shared memory 1
	int pidUp, pidRev; // process IDs for upper and reverse
	int status1, status2; //  current status of program
	void *shared_memory = (void *)0; // shared memory map
	sem_t *toReverse; // shared semaphore 1
	sem_t *toUpper;  // shared semaphore 2
	//----------------------------------------------------------

	// names of the semaphores, shared memory, and message queue
	char const * rev_name = "/reverse";
	char const * up_name = "/upper";
	char const * shm_name = "/shm_proj";
	char const * mq_name = "/msgq";
	// ---------------------------------------------------------

	msg sent; // msg object
	sent.type = SRVMSG; // set type to server msg
	struct mq_attr attr;  // message queue attribute
	attr.mq_flags = 0;
	attr.mq_maxmsg = 1;
	attr.mq_msgsize = sizeof(sent);

	// open message queue
	mqd_t sent_id = mq_open(
	                    mq_name,
	                    O_CREAT | O_EXCL | O_RDWR,
	                    0600,
	                    &attr);
	// check if sent_id exitsts
	if (sent_id == -1) {
		cerr << "Message Queue Open Error: sent_id (/msgq)" << endl;;
		exit(1);
	}
	// open shared memory
	shmfd = shm_open(shm_name, O_CREAT | O_RDWR | O_EXCL | O_TRUNC, 0666);
	// check if shmfd opened
	if (shmfd == -1) {
		cerr << "Shared Memory Open Error: /shm_proj" << endl;
		exit(1);
	}

	// set size of memory object
	if (ftruncate(shmfd, sizeof(struct shared_use_st)) == -1) {
		cerr << "ftruncate: shmfd" << endl;
		exit(1);
	}

	// map the memory object
	shared_memory = mmap(NULL, sizeof(struct shared_use_st),
	                     PROT_READ | PROT_WRITE , MAP_SHARED, shmfd, 0);
	// make sure it was able to map it
	if (shared_memory == MAP_FAILED) {
		cerr << "Memory Map Error" << endl;
		exit(1);
	}
	close(shmfd); // close shmfd we don't need it anymore
	// open semaphores
	if((toReverse = sem_open(rev_name, O_CREAT | O_EXCL, 0600, 0))==SEM_FAILED){
		cerr << "sem_open error: toReverse"<< endl;;
		exit(1);
	}
	if((toUpper = sem_open(up_name, O_CREAT | O_EXCL, 0600, 0))==SEM_FAILED){
		cerr << "sem_open error: toUpper"<< endl;
		exit(1);
	}


	// fork to upper
	pidUp = fork();

	if (pidUp < 0) {
		// unlink names and close semaphores
		sem_unlink(up_name);
		sem_close(toUpper);
		sem_unlink(rev_name);
		sem_close(toReverse);
		cerr<<"Upper Fork Error"<<endl;;
		exit(1);
	}
	else if (pidUp == 0) { // child upper
		// exec to upper
		// upper takes 3 arguments:  2 semaphores and 1 shared memory segment
		execl("upper", "upper", rev_name, up_name, shm_name, NULL);

	}
	else { //parent process 1
		pidRev = fork(); // fork to reverse

		if (pidRev < 0) { // if fork error
			sem_unlink(rev_name);
			sem_close(toReverse);
			sem_unlink(up_name);
			sem_close(toUpper);
			cerr << "Reverse Fork Error" << endl;;
			exit(1);
		}
		else if (pidRev == 0) { // child reverse
			// reverse takes 4 arguments: 2 semaphores, one shared memory, and a message queue
			execl("reverse", "reverse", rev_name, up_name, shm_name, mq_name,  NULL);
		}
		else { // parent process 2

			// install the signal handler
			struct sigaction sig;
			sig.sa_handler = sighandler;
			sigemptyset(&(sig.sa_mask));
			sig.sa_flags = 0;
			sigaction(SIGUSR1, &sig, 0);
			//--------------------------_

			// get to looping
			while (1) {
				cout << "> "; // ask for input
				getline(cin, userInput);
				userInput[userInput.length()] = '\0'; // get rid of newline
				userInput.copy(sent.data, MAXLEN - 1); // copy to sent
				if (cin.eof()) { // check for ctrl+d
					cout << "^D" << endl; // output a ^D
					string quit = "EXIT"; // send this message to reverse
					quit.copy(sent.data, MAXLEN - 1); // copy the message to sent
					mq_send(sent_id, (char*)&sent, sizeof(sent), 0); // send EXIT
					waitpid(pidUp, &status1, 0); // wait for upper to exit
					waitpid(pidRev, &status2, 0); // wait for reverse to exit
					break; // get out of this loop
				}
				// send the data
				if (mq_send(sent_id, (char*)&sent, sizeof(sent), 0) == -1)
				{
					cerr << "Message Queue Send Error" << endl;;
					exit(1);
				}

				memset(sent.data, '\0', sizeof(sent.data));
				pause();
			}

		}
	} // done with this

	// unmap, unlink, and close everything
	munmap(shared_memory, sizeof(struct shared_use_st));
	shm_unlink(shm_name);
	sem_close(toReverse);
	sem_close(toUpper);
	sem_unlink(rev_name);
	sem_unlink(up_name);
	mq_unlink(mq_name);
	mq_close(sent_id);
	//----------------------------------------------------

	exit(EXIT_SUCCESS); // exit
}
