/*$Author: o-pham $*/
/*$Date: 2016/03/08 17:02:31 $*/
/*$Log: master.c,v $
 *Revision 1.3  2016/03/08 17:02:31  o-pham
 **** empty log message ***
 *
 *Revision 1.2  2016/03/07 21:57:01  o-pham
 **** empty log message ***
 *
 *Revision 1.1  2016/03/06 01:23:53  o-pham
 *Initial revision
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include "monitor.h"

#define TIMER 300

// prototypes for the signals
void times_up(int);
void exit_proc(int);

condition_var *cond;

int main(void) {
	key_t shmkey = 100;
	key_t semkey = 101;
	int shmID, semID, j;
	condition_var *c;

	int i, slave_num;
	pid_t childpid;
	pid_t waitreturn;
	pid_t pgID;
	struct itimerval time_val;

/***************Set up Shared Memory**************/
	shmID = shmget(shmkey,sizeof(condition_var), IPC_CREAT | 0666);
	if (shmID < 0) {
		perror("shmget failed in master.c");
		exit(1);
	}

	c = (condition_var *)shmat(shmID,(void *)0,0);
	if (c < (condition_var *)(0)) {
		perror("shmat failed in master.c");
		exit(1);
	}

	cond = c;

/*********** set up the semaphore sets ********
 *********** 3 semaphores in the set **********
 *********** mutex = 0 ************************
 *********** next = 1 *************************
 *********** sem = 2 **************************
*/
	semID = semget(semkey,3,IPC_CREAT | 0666);
	if (semID < 0) {
		perror("semget failed in master.c");
		exit(1);
	}
	
	initelement(semID,mutex,1);	// initialize mutex to be 1
	initelement(semID,next,0); 	// initialize next to be 0
	initelement(semID,sem,0);	// initialize x-sem to be 0
	cond->num_waiting = 0;
	cond->next_count = 0;

/**************Setting Timer**********/
	time_val.it_interval.tv_sec = 0;
	time_val.it_interval.tv_usec = 0;
	time_val.it_value.tv_sec = TIMER;
	time_val.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &time_val, 0);

	signal(SIGALRM, times_up);
	signal(SIGINT, exit_proc);

/************Create 19 child processes*******/	
	for (i = 0; i < 19; i++)
		if ((childpid = fork()) <= 0) {
			slave_num = i+1;
			break;
		}
	
	if (childpid == 0 ) {	// child process
		pgID = getpgrp();
	
		char proc_num[sizeof(slave_num)];
		snprintf(proc_num, sizeof(slave_num), "%d", slave_num);
		execl("./slave", "./slave", proc_num, NULL);
	}

	else {	// parent process
		pgID = getpgrp();
		while (childpid != (waitreturn = wait(NULL)))
			if ((childpid == -1) && (errno != EINTR))
				break;

		sleep(1);
		fprintf(stderr,"Hey all children are done, deallocating shared "
			"memory and semaphores\n");

		if (shmctl(shmID, IPC_RMID, NULL) < 0) {
			perror("failed to remove shared memory");
			exit(1);
		}
		
        if (semctl(semID, 0, IPC_RMID) == -1) {
            perror("process done failed to remove semaphores");
            exit(1);
        }		

		exit(0);
	}

	return 0;
}

// function to end all processes when time is up
void times_up(int i) {
/***************Set up Shared Memory**************/
	key_t shmkey = 100;
	key_t semkey = 101;
	int shmID, semID, j;
	condition_var *c;

	shmID = shmget(shmkey,sizeof(condition_var), IPC_CREAT | 0666);
    if (shmID < 0) {
        perror("shmget failed in master.c");
        exit(1);
    }

    c = (condition_var *)shmat(shmID,(void *)0,0);
    if (c < (condition_var *)(0)) {
        perror("shmat failed in master.c");
        exit(1);
    }

    cond = c;

/*********** set up the semaphore sets *********
 ************ 3 semaphores in the set **********
 ************ mutex = 0 ************************
 ************ next = 1 *************************
 ************ sem = 2 **************************
*/
    semID = semget(semkey,3,IPC_CREAT | 0666);
    if (semID < 0) {
        perror("semget failed in master.c");
        exit(1);
    }
	
	fprintf(stderr,"%d seconds is up master and all childs are closing\n",TIMER);
	fprintf(stderr,"shared memory and semaphore are deallocating...\n");
		
	// deallocate shared mem
	if (shmctl(shmID, IPC_RMID, NULL) < 0) {
    	perror("failed to remove shared memory");
        exit(1);
    }
	
	killpg(0,SIGKILL); // kill processes

	// deallocate semaphore set
    if (semctl(semID, 0, IPC_RMID) == -1) {
    	perror("times_up failed to remove semaphores");
    	exit(1);
    }	

	exit(0);
}

// function to handle interrupt signal
void exit_proc (int i) {
/***************Set up Shared Memory**************/
    key_t shmkey = 100;
    key_t semkey = 101;
    int shmID, semID, j;
    condition_var *c;

    shmID = shmget(shmkey,sizeof(condition_var), IPC_CREAT | 0666);
    if (shmID < 0) {
        perror("shmget failed in master.c");
        exit(1);
    }

    c = (condition_var *)shmat(shmID,(void *)0,0);
    if (c < (condition_var *)(0)) {
        perror("shmat failed in master.c");
        exit(1);
    }

    cond = c;

/*********** set up the semaphore sets **********
 ************* 3 semaphores in the set **********
 ************* mutex = 0 ************************
 ************* next = 1 *************************
 ************* sem = 2 **************************
*/
    semID = semget(semkey,3,IPC_CREAT | 0666);
    if (semID < 0) {
        perror("semget failed in master.c");
        exit(1);
    }

	signal(SIGINT, exit_proc);	//the signal

	printf("interrupt signal ^c received, master and slave dieing\n");
	printf("shared memory and semaphore are deallocating..\n");
	
	// deallocate shared mem
	if (shmctl(shmID, IPC_RMID, NULL) < 0) {
        perror("failed to remove shared memory");
        exit(1);
    }
    
	// deallocate semaphore set
	if (semctl(semID, 0, IPC_RMID) == -1) {
    	perror("exit_proc failed to remove semaphores");
       	exit(1);
   	}

	exit(0);
}
