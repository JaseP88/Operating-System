/*$Author: o-pham $*/
/*$Date: 2016/04/12 06:00:18 $*/
/*$Log: process.c,v $
 *Revision 1.6  2016/04/12 06:00:18  o-pham
 *need tweaks
 *
 *Revision 1.5  2016/04/11 03:43:37  o-pham
 **** empty log message ***
 *
 *Revision 1.4  2016/04/08 00:55:42  o-pham
 *forks fine
 *randomizaton good
 *
 *Revision 1.3  2016/04/05 07:57:09  o-pham
 **** empty log message ***
 *
 *Revision 1.2  2016/04/05 05:57:58  o-pham
 **** empty log message ***
 *
 *Revision 1.1  2016/04/03 10:39:33  o-pham
 *Initial revision
 **/

#include "resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <signal.h>
s_m *shared_struct;

// PROCESS.C
int main(int argc, char *argv[]) {
	srand(time(NULL));
/*********** Set up Shared Memory *************/
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
/*********** Initialize Process ************/
	
	int proc = sm->p_num;
	int p = atoi(argv[1]); //index argument passed in oss.c
	int i, r, request, max_claim;
	unsigned int start_time_sec = sm->lclock.sec;
    unsigned int start_time_nan = sm->lclock.nan;	

  	fprintf(stderr,"process %d forked at %d:%09d\n",sm->p_num,sm->lclock.sec,sm->lclock.nan);

	for(r=0;r<20;r++) {
		//generate a request for each resources 
		//instances in the system
		max_claim = rand_gen(0,sm->rs[r].instance);
		sm->rs[r].max[p] = max_claim;
		sm->rs[r].maxC[p] = max_claim;

		sm->rs[r].req[p] = max_claim;
		sm->rs[r].reqC[p] = max_claim;
	}

	//semsignal(semID,0);// signal oss to run
/*********** Start Loop Simulation *********/
	int terminate; 		//boolean test var to terminate or not
	int wait_to_fork = NO; 	//boolean test var 
	int rand_res_req_time = 0;
	unsigned int current_time_sec, current_time_nan;

	terminate = rand_gen(NO,YES);

	for(;;) {
		int res_counter = 0;		
		//update logical clock//
		if(sm->lclock.nan >= 999999999) {
			sm->lclock.nan = 0;
			sm->lclock.sec++;
			fprintf(stderr,"%d second\n",sm->lclock.sec);
		}	
		//fprintf(stderr,"C%ld:%ld\n",sm->lclock.sec,sm->lclock.nan);
	
		//generate to determined if to terminate or no
		//terminate = rand_gen(NO,YES);		

		//checks to see if oss allocated desired amount of resources
		//if so release the resources refilling the available array
		//and resetting request and allocate array back to 0 for another
		//process to use. Then exit
		for(r=0;r<20;r++) {
			sm->p[p].cpu_util++;
			if(sm->rs[r].allo[p] == sm->rs[r].max[p]) {
				//add a counter for every full resource this process has
				//if the counter is 20 then it means this process
				//has all necessary requested resources allocated to it
				res_counter++;
			}
		}
		if(res_counter == 20) {
			fprintf(stderr,"process %d got all of resource, releasing it now\n",proc);
			for(r=0;r<20;r++) {
				sm->p[p].cpu_util++;
				sm->rs[r].req[p] = 0;
				sm->rs[r].reqC[p] = 0;
				
				sm->rs[r].avail[p] += sm->rs[r].max[p];
				sm->rs[r].availC[p] += sm->rs[r].maxC[p];
				
				sm->rs[r].max[p] = 0;
				sm->rs[r].maxC[p] = 0;
				sm->rs[r].allo[p] = 0;
				sm->rs[r].alloC[p] = 0;
			}
			sm->taken[p] = NO;
			--sm->process_running;//=-1;
			//add completion time********
			//add throughput and its turnaround time before exit
			sm->throughput+=1;
			sm->p[p].turna_time_s = (sm->lclock.sec - start_time_sec);
			
			if(sm->lclock.nan < start_time_nan)
				sm->p[p].turna_time_n = start_time_nan - sm->lclock.nan;
			else
				sm->p[p].turna_time_n = sm->lclock.nan - start_time_nan;

			fprintf(stderr,"turnaround time for process %d is %d:%d\n",proc,
                sm->p[p].turna_time_s,sm->p[p].turna_time_n);
			fprintf(stderr,"cpu_util for process %d is %d nano\n",proc,sm->p[p].waiting_time);
			fprintf(stderr,"waiting time for process %d is %d nano\n",proc,
				sm->p[p].waiting_time);
			exit(1);
		}	

		//if one second has passed sinced forked and terminate bool is Yes then 
		//release the resources
		if((sm->lclock.sec >= start_time_sec+1) && (sm->lclock.nan >= start_time_nan) && (terminate == YES)) {
			fprintf(stderr,"process %d is terminating, releasing resources\n",proc);
			for(r=0;r<20;r++) {
				sm->p[p].cpu_util++;
				sm->rs[r].max[p] = 0;
				sm->rs[r].maxC[p] = 0;

				sm->rs[r].req[p] = 0;
				sm->rs[r].reqC[p] = 0;

				sm->rs[r].avail[p] += sm->rs[r].allo[p];
				sm->rs[r].availC[p] += sm->rs[r].alloC[p];

				sm->rs[r].allo[p] = 0;
				sm->rs[r].alloC[p] = 0;	
			}
			sm->taken[p] = NO;
			--sm->process_running;// =-1;
			sm->throughput+=1;
            sm->p[p].turna_time_s = (sm->lclock.sec - start_time_sec);
			if(sm->lclock.nan < start_time_nan)
                sm->p[p].turna_time_n = start_time_nan - sm->lclock.nan;
            else
                sm->p[p].turna_time_n = sm->lclock.nan - start_time_nan;

			fprintf(stderr,"turnaround time for process %d is %d:%d\n",proc,
				sm->p[p].turna_time_s,sm->p[p].turna_time_n);
			fprintf(stderr,"cpu_util for process %d is %d nano\n",proc,sm->p[p].waiting_time);
            fprintf(stderr,"waiting time for process %d is %d nano\n",proc,
                sm->p[p].waiting_time);
			exit(1);
		}

		//if it does not termiate process request for random amount resource.
		else if((sm->lclock.sec >= start_time_sec+1) && (sm->lclock.nan >= start_time_nan) && (terminate == NO)) {
			rand_res_req_time = rand_gen(1, 250000);	
			wait_to_fork = YES;
		}

		
		if(rand_res_req_time <= 0 && wait_to_fork == YES) {
			fprintf(stderr,"process %d is requesting...\n",proc);
			for(r=0;r<20;r++) {
				sm->p[p].cpu_util++;
				//requesting a random amount between 0 and max-allocate
				request = rand_gen(0,sm->rs[r].max[p]-sm->rs[r].allo[p]);
				sm->rs[r].req[p] = request;
				sm->rs[r].reqC[p] = request;
			}
			//after requests set wait_to_fork back to NO
			//and regenerate terminate
			wait_to_fork = NO;
			terminate = rand_gen(NO,YES);
		}	

		sm->rand_fork_time-=1;//for use in oss.c
		rand_res_req_time--;
		sm->lclock.nan+=1; //increment logic clock by 1 nanosec
		sm->p[p].waiting_time++;
	}//loop block

return 0;
}//PROCESS.C
