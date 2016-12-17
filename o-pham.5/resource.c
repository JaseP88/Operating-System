/*$Author: o-pham $*/
/*$Date: 2016/04/12 06:00:09 $*/
/*$Log: resource.c,v $
 *Revision 1.4  2016/04/12 06:00:09  o-pham
 *good and working.
 *
 *Revision 1.3  2016/04/05 05:58:02  o-pham
 **** empty log message ***
 *
 *Revision 1.2  2016/04/03 02:31:47  o-pham
 **** empty log message ***
 *
 *Revision 1.1  2016/04/02 10:55:41  o-pham
 *Initial revision
 **/

#include "resource.h"
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
s_m *shared_struct;

struct sembuf WAIT = {0,-1,0};
struct sembuf SIGNAL = {0,1,0};

/**** random function generator *****/
int rand_gen(int min, int max) {
	int num;
	num = (rand()%(max-min+1))+min;
	return num;
}

/**** initialize semaphore *****/
int seminit(int semid,  int semnum, int semvalue) {
	union semun {
		int val;
		struct semid_ds *buf;
		unsigned short *array;
	} arg;
	arg.val = semvalue;
	return semctl(semid,semnum,SETVAL,arg);
}

void update_clock(s_m *s) {
	s->lclock.sec = s->lclock.nan % 1000000000;
}




void semwait(int semID, int semnum) {
	WAIT.sem_num = semnum;
	if(semop(semID,&WAIT,1) == -1) {
		perror("semop wait error");
		exit(1);
	}
}

void semsignal(int semID, int semnum) {
	SIGNAL.sem_num = semnum;
	if(semop(semID,&SIGNAL,1) == -1) {
		perror("semop signal error");
		exit(1);
	}
}	

int index_open(s_m *s) {
	int i;
	//goes through the bit array and pick the free index and return index
	for(i=0;i<18;i++) {
		if(s->taken[i] == NO)
			return i;
	}
}

void test( int r,int p, s_m *sm) {
fprintf(stderr,"**shareable resources**\n");
for(r=0;r<20;r++)
    if(sm->rs[r].shareable == YES)
        fprintf(stderr,"%d ",r+1);
        fprintf(stderr,"\n");

fprintf(stderr,"**instances**\n");
	for(r=0;r<20;r++)
		fprintf(stderr,"%d| ",sm->rs[r].instance);
	fprintf(stderr,"\n");

fprintf(stderr,"**max claim array**\n");
	for(r=0;r<20;r++)
		fprintf(stderr,"%d| ",sm->rs[r].max[p]);
	fprintf(stderr,"\n");

fprintf(stderr,"**req array**\n");
for(r=0;r<20;r++)
fprintf(stderr,"%d| ",sm->rs[r].req[p]);
fprintf(stderr,"\n");


fprintf(stderr,"**need array**\n");
for(r=0;r<20;r++)
fprintf(stderr,"%d| ",sm->rs[r].need[p]);
fprintf(stderr,"\n");


fprintf(stderr,"**alloc array**\n");
for(p=0;p<18;p++) {
for(r=0;r<20;r++)
fprintf(stderr,"%d| ",sm->rs[r].allo[p]);
fprintf(stderr,"\n\n");
}


fprintf(stderr,"**avail array**\n");
for(r=0;r<20;r++)
fprintf(stderr,"%d| ",sm->rs[r].avail[r]);
fprintf(stderr,"\n\n");
}
			
bool req_lt_avail(int req[], int avail[], int pnum, int num_res) {
	int i;
	for(i=0;i<num_res;i++)
		if(req[pnum*num_res+i] > avail[i])
			break;

	return(i == num_res);
}

bool deadlock(int avail[], int m, int n, int req[], int allocated[]) {
	//m is..resource 20
	//n is..processes 18

	int work[m];
	bool finish[n];
	int i;

	for(i=0;i<m;work[i]=avail[i++]);
	for(i=0;i<n;finish[i++]=false);

	int p;
	for(p=0;p<n;p++) {
		if(finish[p]) continue;
		if(req_lt_avail(req,work,p,m)) {
			finish[p] = true;
			for(i=0;i<m;i++)
				work[i]+=allocated[p*m+i];
			p=-1;
		}
	}

	for(p=0;p<n;p++)
		if(!finish[p])
			break;

	return(p!=n);
}

void exit_proc(int i) {
    key_t shmkey;
    if ((shmkey = ftok("/classes/OS/o-pham/o-pham.5",100)) == (key_t)-1) {
        perror("ftok error");
        exit(1);
    }
    int shmID;
    s_m *sm;

    shmID = shmget(shmkey,sizeof(s_m),IPC_CREAT | 0666);
    if (shmID<0) {
        perror("shmget failed");
        exit(1);
    }

    sm = (s_m *)shmat(shmID,(void *)0,0);
    if (sm<(s_m *)(0)) {
        perror("shmat failed");
        exit(1);
    }

    shared_struct = sm;

/*********** Set up Semaphore ***************/

    int semID;
    key_t semkey;
    if ((semkey = ftok("/classes/OS/o-pham/o-pham.5",101)) == (key_t)-1) {
        perror("ftok error");
        exit(1);
    }
    semID = semget(semkey,2,IPC_CREAT | 0666);
    if (semID<0) {
        perror("semget failed");
        exit(1);
    }


	signal(SIGINT, exit_proc);  //the signal

    printf("interrupt signal ^c received, master and slave dieing\n");
    printf("shared memory and semaphore are deallocating..\n");
	
	if(shmctl(shmID,IPC_RMID,NULL) == -1) {
		perror("failed to remove shared memory");
		exit(1);
	}

	if(semctl(semID,0,IPC_RMID) == -1) {
		perror("failed to remove semaphores");
		exit(1);
	}
	exit(1);
}

pid_t r_wait(int *stat_loc) {
	int retval;

	while(((retval = wait(stat_loc)) == -1) && (errno == EINTR));
	return retval;
}
