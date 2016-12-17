/*$Author: o-pham $*/
/*$Date: 2016/03/08 17:02:31 $*/
/*$Log: slave.c,v $
 *Revision 1.3  2016/03/08 17:02:31  o-pham
 **** empty log message ***
 *
 *Revision 1.2  2016/03/07 21:57:56  o-pham
 **** empty log message ***
 *
 *Revision 1.1  2016/03/06 10:46:53  o-pham
 *Initial revision
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <signal.h>
#include "monitor.h"

void writeToFile(int);	// functio prototype

condition_var *cond;

// main function
int main(int argc, char *argv[]) {
	time_t  epoch_time;
    struct tm *t;
    epoch_time = time(NULL);
    t = localtime(&epoch_time);
	int slave_num = atoi(argv[1]);
/***************Set up Shared Memory**************/
    key_t shmkey = 100;
	int shmID;
	condition_var *c;

	shmID = shmget(shmkey,sizeof(condition_var), IPC_CREAT | 0666);
    if (shmID < 0) {
        perror("shmget failed in slave.c");
        exit(1);
    }

    c = (condition_var *)shmat(shmID,(void *)0,0);
    if (c < (condition_var *)(0)) {
        perror("shmat failed in slave.c");
        exit(1);
    }

    cond = c;

/******************Set up Sem Set****************/
	key_t semkey = 101;
	int semID;	
	semID = semget(semkey,3,IPC_CREAT | 0666);
	if (semID < 0) {
		perror("semget failed in slave.c");
		exit(1);
	}



/************** Start Of Process*****************/	
	int numOfWrites = 0;

	for (numOfWrites; numOfWrites < 3; numOfWrites++) {
		Pwait(mutex);
		sleep(1);
		//fprintf(stderr,"test\n");
		fprintf(stderr,"--%.2d:%.2d:%.2d slave %d entered critical section\n",t->tm_hour,t->tm_min,t->tm_sec,slave_num);
		sleep(1);

		fprintf(stderr,"writing.....\n");
		writeToFile(slave_num);		//critical section
		sleep(1);

		if (cond->next_count > 0) {
			fprintf(stderr,"**%.2d:%.2d:%.2d slave %d leaving critical section\n",t->tm_hour,t->tm_min,t->tm_sec,slave_num);
			sleep(1);
			Vsignal(next);
		}
	
		else {
	 		fprintf(stderr,"**%.2d:%.2d:%.2d slave %d leaving critical section\n",t->tm_hour,t->tm_min,t->tm_sec,slave_num);
			sleep(1);
			Vsignal(mutex);
		}
	}
	// remainder section here

	fprintf(stderr,"slave %d entered critical section 3x, "
		"slave %d is terminating now..\n",slave_num,slave_num);	

	return 0;
}

void writeToFile (int process_number) {
    time_t  epoch_time;
    struct tm *tm_p;
    epoch_time = time(NULL);
    tm_p = localtime(&epoch_time);

    FILE *fp;
    int p_n = process_number; // process number


    if ( (fp = fopen("cstest", "a")) == NULL) {
        perror("Error opening file");
    }

    fprintf(fp, "File modified by process number %d at time "
        "%.2d:%.2d:%.2d\n", p_n, tm_p-> tm_hour, tm_p-> tm_min, tm_p-> tm_sec);

    fclose(fp);
	return;
}
