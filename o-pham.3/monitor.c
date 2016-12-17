/*$Author: o-pham $*/
/*$Log: monitor.c,v $
 *Revision 1.5  2016/03/08 17:02:31  o-pham
 **** empty log message ***
 *
 *Revision 1.4  2016/03/07 21:57:05  o-pham
 *ci slave.c
 *ls
 *
 *Revision 1.3  2016/03/06 02:04:59  o-pham
 **** empty log message ***
 *
 *Revision 1.2  2016/03/05 23:52:47  o-pham
 **** empty log message ***
 *
 *Revision 1.1  2016/03/05 09:19:14  o-pham
 *Initial revision
 **/
/*$Date: 2016/03/08 17:02:31 $*/
#include <sys/sem.h>
#include "monitor.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <stdio.h>
condition_var *cond;

// operations for semop
struct sembuf WAIT = {0,-1,0};
struct sembuf SIGNAL = {0,1,0};

/* used to initialize mutex and next (used as instance) */
int initelement (int semid, int semnum, int semvalue) {
	union semun {
		int val;
		struct semid_ds *buf;
		unsigned short *array;
	} arg;
	arg.val = semvalue;
	return semctl(semid,semnum,SETVAL,arg);
}

/* function to perform the locking wait operation using semop
 * takes in the semaphore set element (mutex,next,sem) */
void Pwait(int condition_sem) {
	int semID;
	key_t semkey = 101;
	semID = semget(semkey,3,IPC_CREAT | 0666);
	if (semID < 0) {
		perror("semget failed in Pwait");
		exit(1);
	}
	WAIT.sem_num = condition_sem;

	if (semop(semID,&WAIT,1) == -1) {
		perror("semop wait error");
		exit(1);
	}
	return;
}

void Vsignal(int condition_sem) {
    int semID;
    key_t semkey = 101;
    semID = semget(semkey,3,IPC_CREAT | 0666);
    if (semID < 0) {
        perror("semget failed in Vsignal");
        exit(1);
    }
    SIGNAL.sem_num = condition_sem;

    if (semop(semID,&SIGNAL,1) == -1) {
        perror("semop signal error");
        exit(1);
    }
    return;
}

/* condition wait function of monitor */
void cwait(int condition_sem) {
	key_t shmkey = 100;		//create shared mem key
	key_t semkey = 101;		//create semaphore set key
	int shmID, semID;
	condition_var *c;

/* Set up the shared memory for the condition variable in monitor*/
	shmID = shmget(shmkey, sizeof(condition_var), IPC_CREAT | 0666);
	if (shmID < 0) {
		perror("shmget failed in monitor.c");
		exit(1);
	}

	c = (condition_var *)shmat(shmID, (void *)0,0);
	if (c < (condition_var *)(0)) {
		perror("shmat failed in monitor.c");
		exit(1);
	}

	cond = c;

/* Get the ID of semaphore set for the monitor*/
	semID = semget(semkey,3,IPC_CREAT | 0666);
	if (semID < 0) {
		perror("semget failed in monitor.c");
		exit(1);
	}
	
	cond->num_waiting++;
	if (cond->next_count > 0) 
		Vsignal(next);
	else 
		Vsignal(mutex);

	Pwait(sem);
	cond->num_waiting--;
}

/* condition signal function of the monitor */
void csignal(int condition_sem) {
    key_t shmkey = 100;     //create shared mem key
    key_t semkey = 101;     //create semaphore set key
    int shmID, semID;
    condition_var *c;

/* Set up the shared memory for the condition variable in monitor*/
    shmID = shmget(shmkey, sizeof(condition_var), IPC_CREAT | 0666);
    if (shmID < 0) {
        perror("shmget failed in monitor.c");
        exit(1);
    }

    c = (condition_var *)shmat(shmID, (void *)0,0);
    if (c < (condition_var *)(0)) {
        perror("shmat failed in monitor.c");
        exit(1);
    }

    cond = c;

/* Get the ID of semaphore set for the monitor*/
    semID = semget(semkey,3,IPC_CREAT | 0666);
    if (semID < 0) {
        perror("semget failed in monitor.c");
        exit(1);
    }
	
	if (cond->num_waiting <= 0)
		return;

	cond->next_count++;
	Vsignal(sem);
	Pwait(next);
	cond->next_count--;
}
