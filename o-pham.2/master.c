/*$Author: o-pham $*/
/*$Date: 2016/02/24 06:34:33 $*/
/*$Log: master.c,v $
 *Revision 1.9  2016/02/24 06:34:33  o-pham
 *finished version
 *
 *Revision 1.8  2016/02/20 05:07:04  o-pham
 **** empty log message ***
 *
 *Revision 1.7  2016/02/19 23:33:16  o-pham
 *added signals and moved operations to slave
 *
 *Revision 1.6  2016/02/18 23:44:13  o-pham
 *currently working.
 *
 *Revision 1.5  2016/02/18 10:42:22  o-pham
 **** empty log message ***
 *
 *Revision 1.4  2016/02/18 10:30:19  o-pham
 *added critical section function
 *
 *Revision 1.3  2016/02/18 08:55:29  o-pham
 *added multiple solution to processes function and share emm mem
 *
 *Revision 1.2  2016/02/17 23:53:07  o-pham
 *fanned the processes, gave it a value 1-19
 *
 *Revision 1.1  2016/02/17 22:22:43  o-pham
 *Initial revision
 **/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include "header.h"
#include <sys/time.h>
#include <signal.h>
#include <sys/shm.h>

#define TIMER  300// timer value

p_s *ps;

int main (void) {


/*************SET SHARED MEM IN MAIN********/
	 key_t key = 100;
    int shmID;
    p_s *shm_mem;

    shmID = shmget(key,sizeof(p_s), IPC_CREAT | 0666);
    if (shmID < 0) {
        perror("shmget failed\n");
        exit(1);
    }

    shm_mem = (p_s *)shmat(shmID, (void *)0, 0);
    if (shm_mem < (p_s *)(0)) {
        perror("shmat failed\n");
        exit(1);
    }

    ps = shm_mem;	




	int i, slave_num;
	pid_t childpid;
	pid_t waitreturn;
	pid_t pgID;
	struct itimerval time_val;


/*********SETTING TIMER ***********/
	time_val.it_interval.tv_sec = 0;
	time_val.it_interval.tv_usec =0;
	time_val.it_value.tv_sec = TIMER;	// set the timer
	time_val.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &time_val, 0);

	// signals to catch
	signal(SIGALRM, times_up);
	signal(SIGINT, exit_proc);
	
	// creates a fan of 19 child process and 1 parent process
	// Used i to set the slave number for each process the last
	// process 20 is the master.
	for (i = 0; i < 19; i++)
		if ((childpid = fork()) <= 0) {
			slave_num = i + 1; // start at 1
			break;
		}

	if (childpid == 0) { // slave process
		pgID = getpgrp();	// establish a group pid to kill
		
		char proc_num[sizeof(slave_num)];	// convert the slave_num to a string
		snprintf(proc_num, sizeof(slave_num), "%d", slave_num);

		// exec child process to run slave process
		execl("./slave", "./slave", proc_num, NULL); 
	}

	else {
		pgID = getpgrp();	// set the group pid
		
		// wait for children to end
		while (childpid != (waitreturn = wait(NULL)))
			if ((childpid == -1) && (errno != EINTR))
				break;
		// when all children finishes delete shared memory and exit
		fprintf(stderr,"Hey all children are done, deallocating shared mem... LATA\n");
		shmctl(shmID, IPC_RMID, NULL);
		exit(0);
	}
		
	
	return 0;
}


// these are handler functions for the signals
void times_up (int i) {
	/* **********SHARED MEMORY*********/
    key_t key = 100;
    int shmID;
    p_s *shm_mem;

    shmID = shmget(key,sizeof(p_s), IPC_CREAT | 0666);
    if (shmID < 0) {
		perror("shmget failed\n");
		exit(1);
	}

	shm_mem = (p_s *)shmat(shmID, (void *)0, 0);
	if (shm_mem < (p_s *)(0)) {
		perror("shmat failed\n");
		exit(1);
	}

    ps = shm_mem;

	printf("%d seconds is up master and all childs are closing\n", TIMER);
	printf( "shared memory is deallocating...\n");
	shmctl(shmID, IPC_RMID, NULL);
	killpg(0, SIGKILL); // kill children group pid
	exit(0);
}

void exit_proc (int i) {
	/* **********SHARED MEMORY*********/
    key_t key = 100;
    int shmID;
    p_s *shm_mem;

    shmID = shmget(key,sizeof(p_s), IPC_CREAT | 0666);
	if (shmID < 0) {
        perror("shmget failed\n");
        exit(1);
    }

    shm_mem = (p_s *)shmat(shmID, (void *)0, 0);
	if (shm_mem < (p_s *)(0)) {
        perror("shmat failed\n");
        exit(1);
    }


    ps = shm_mem;


	signal(SIGINT, exit_proc);
	printf("interrupt signal ^c received, master and slave dieing\n");
	printf("shared memory is deallocating..\n");
	shmctl(shmID, IPC_RMID, NULL);
	exit(0);
}


