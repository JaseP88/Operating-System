#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "header.h"
#include <unistd.h>
#include <sys/shm.h>
#include <signal.h>


p_s *ps;	//shared memory

int main (int argc, char *argv[]) {

	int pn = atoi(argv[1]);	// get process number from argument
	int pn_index = pn -1;	// changes process number to use as index flag array

	int shmid;
	key_t key = 100;
	p_s *shm_ps;

	// create the shared memory
	shmid = shmget(key, sizeof(p_s), IPC_CREAT | 0666);
	if (shmid < 0) {
		perror("shmget failed\n");
		exit(1);
	}

	// attach shared memory with shm_ps
	shm_ps = (p_s *)shmat(shmid, (void *)0, 0);
	if (shm_ps == (p_s *)(-1)) {
		perror("shmat failed\n");
		exit(1);
	}
	
	ps = shm_ps;
	
	// call the process function to begin determining which slave process
	process(pn_index);

	
	
	/*printf("Hi i am process # %d, my pid is %ld and my parent "
		"pid is %ld\n", pn, (long)getpid(), (long)getppid());
	
	while(1);
	*/

	return 0;
}


// this function writes the message into cstest
void writeToFile (int process_number) {
	time_t	epoch_time;
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
}

void process (const int i) {

	int numOfWrites = 0;
	int slave_num = i+1;
	time_t  epoch_time;
    struct tm *t;
    epoch_time = time(NULL);
    t = localtime(&epoch_time);
	int j;
		
	do {
		fprintf(stderr,"Slave %d is executing code to enter CS %.2d:%.2d:%.2d"
			"\n", slave_num, t->tm_hour, t->tm_min, t->tm_sec);
		do {
			ps-> flag[i] = want_in;
			j = ps-> turn;
			while (j != i) 
				j = (ps-> flag[j] != idle) ? ps-> turn : (j+1)%19;
			
			ps-> flag[i] = in_cs;

			for (j = 0; j < 19; j++)
				if ((j != i) && (ps-> flag[j] == in_cs)) 
					break;
		}
		while ((ps-> turn != i && ps-> flag[ps-> turn] != idle) || j < 19);
		
		// check max writes if 3 then set that flag to idle and kill child process
		if (numOfWrites == 3) {
			ps->flag[i] = idle;
            fprintf(stderr,"slave %d achieved maxwrite, gonna die now\n",slave_num);
            kill(getpid(),SIGTERM);
			exit(0);
        }

		ps-> turn = i;

		// Entering the critical section
		fprintf(stderr,"Slave:%d ENTERING CRIT SECTION %.2d:%.2d:%.2d"
			"\n",slave_num, t->tm_hour, t->tm_min, t->tm_sec);
		sleep(1);
		writeToFile(slave_num);	// writing onto the valuable resource
		fprintf(stderr,"Slave:%d ***LEAVING CRIT SECTION %.2d:%.2d:%.2d"
			"\n",slave_num, t->tm_hour, t->tm_min, t->tm_sec);
		sleep(1);
		
		j = (ps-> turn+1)%19;
		while (ps-> flag[j] == idle)
			j = (j+1)%19;

		ps-> turn = j;

		ps-> flag[i] = idle;

        // Remainder Section
		numOfWrites++;	//increment the max number of writes so far

	}
	while (1);
}


