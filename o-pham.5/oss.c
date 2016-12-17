/*$Author: o-pham $*/
/*$Date: 2016/04/12 05:59:50 $*/
/*$Log: oss.c,v $
 *Revision 1.8  2016/04/12 05:59:50  o-pham
 *good and working
 *
 *Revision 1.7  2016/04/11 03:43:40  o-pham
 **** empty log message ***
 *
 *Revision 1.6  2016/04/08 00:55:18  o-pham
 *fork works
 *randomization in children good
 *
 *Revision 1.5  2016/04/06 07:45:50  o-pham
 **** empty log message ***
 *
 *Revision 1.4  2016/04/05 05:57:54  o-pham
 **** empty log message ***
 *
 *Revision 1.3  2016/04/03 10:39:49  o-pham
 **** empty log message ***
 *
 *Revision 1.2  2016/04/03 02:31:42  o-pham
 **** empty log message ***
 *
 *Revision 1.1  2016/04/01 20:06:51  o-pham
 *Initial revision
 **/

#include <stdio.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "resource.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/resource.h>

s_m *shared_struct;

/****** function to handle the SIGCHLD signal when child exits *****/
void chi_exit()
{	
	//while(r_wait(NULL) > 0);
//	semop error but ideal usage with this wait
  	pid_t childPID;

	while(childPID = waitpid(-1,NULL,WNOHANG))
		if((childPID == -1)&&(errno != EINTR))
			break;
	
}

// OSS.C
int main(void) {
	srand(time(NULL));
/********** Set up Shared Mem **********/
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

/*********** Set up Semaphore **********/
	int semID;
	key_t semkey;
	if ((semkey = ftok("/classes/OS/o-pham/o-pham.5",101)) == (key_t) -1) {
		perror("ftok error");
		exit(1);
	}

	semID = semget(semkey,2,IPC_CREAT | 0666);	
	if (semID<0) {
		perror("semget failed");
		exit(1);
	}

	seminit(semID,0,0);//oss semaphore
	seminit(semID,1,0);//process semaphore

/********** Initialize Resource Descriptor **********/
	int i, j, r, p;
	bool dl;
	int share_counter = rand_gen(3,5);	//20% of 20 +/-

	for(i=0;i<20;i++) {
		for(j=0;j<18;j++) {
			/*initialize request, allocate, and need array*/
			sm->rs[i].req[j] = 0;
			sm->rs[i].reqC[j] = 0;
			sm->rs[i].allo[j] = 0;
			sm->rs[i].alloC[j] = 0;
			sm->rs[i].need[j] = 0;
			sm->rs[i].needC[j] = 0;
			sm->rs[i].max[j] = 0;
			sm->rs[i].maxC[j] = 0;
		}

		sm->rs[i].instance = rand_gen(1,10); // initialize instance
		sm->rs[i].avail[i] = sm->rs[i].instance;//initialize available array
		sm->rs[i].availC[i] = sm->rs[i].instance;//and its copy

		if (share_counter > 0) {	//allow random generation of shareable res.
			sm->rs[i].shareable = rand_gen(NO,YES);
			if(sm->rs[i].shareable == YES)
				share_counter--;
		}
		else	//if 3 / 5 shareable resource is instanced rest is non shareable
			sm->rs[i].shareable = NO;
	}

/********* Set up process structures *************/
	sm->throughput = 0;
	for(p=0;p<18;p++) {
		sm->p[p].cpu_util = 0;
		sm->p[p].waiting_time= 0;
		sm->p[p].turna_time_s = 0;
		sm->p[p].turna_time_n = 0;
	}

/********** Initialize taken array ********/
	for(i=0;i<18;i++) 
		sm->taken[i] = NO;
/********** Start Logical Clock ***********/
	sm->lclock.sec = 0;
	sm->lclock.nan = 0;
/********** Initialize Shared Mem ***********/
	
	sm->process_running = 0; //set the number of process running to 0 at start
	sm->p_num = 0;//counter of how many process ran
	signal (SIGCHLD, chi_exit);

/********** Fork at time 0 ************/
	pid_t pID, pgID;
	pgID = getpgrp();
	int index = 0;
	sleep(1); //so child random generator work
	pID = fork();//fork at ~time 0

    if(pID == -1) {
    	perror("fork error");
        exit(1);
    }
    if (pID == 0) { //the child
		pgID = getpgrp();
        char pass_index[sizeof(index)];
        snprintf(pass_index,sizeof(index),"%d",index);
        execl("./process","./process",pass_index,NULL);
    }

    sm->taken[index] = YES;//update the taken array
    sm->process_running+=1;//update the total # of process
	sm->p_num+=1;
    index = index_open(sm);//picks the free index in taken array
	
	//semwait(semID,0); //oss sleep
	
/********** Start Simulation Loop ************/
	int wait_to_fork = NO;
	signal(SIGINT, exit_proc); //interupt signal


	for(;;) {

		 //synchronize nanoseconds and seconds 
		if (sm->lclock.nan >= 999999999) {
			sm->lclock.nan = 0;
			sm->lclock.sec++;
			fprintf(stderr,"%d second\n",sm->lclock.sec);
		}

		//fprintf(stderr,"P%ld:%ld\n",sm->lclock.sec,sm->lclock.nan);
		
		/* generate the random time in millisecond to fork */
		if (wait_to_fork == NO) {
			sm->rand_fork_time = rand_gen(1000,500000); //1-500 milliseconds
			wait_to_fork = YES;
		}
		
		if (sm->rand_fork_time <= 0 && wait_to_fork == YES) {
			/* Limit fork process to 18 process */
			if (sm->process_running < 18) {
				sleep(1);//so child process random generator work
				pID = fork();
				if(pID == -1) {
					perror("fork error");
					exit(1);
				}
				else if (pID == 0) { //the child
					pgID = getpgrp();
					char pass_index[sizeof(index)];
					snprintf(pass_index,sizeof(index),"%d",index);
					execl("./process","./process",pass_index,NULL);
				}

				sm->taken[index] = YES;//update the taken array
				sm->process_running+=1;//update the total # of process
				sm->p_num+=1;
				index = index_open(sm);//picks the free index in taken array
				wait_to_fork = NO;
				//semwait(semID,0);
			}	
		}

		//scan for the needs of each process and also allocate
		//the resource that are shareable
		for(p=0;p<18;p++) 
			for(r=0;r<20;r++) {
				//get current needs of the process p
				sm->rs[r].need[p] = sm->rs[r].max[p] - sm->rs[r].allo[p];
				sm->rs[r].needC[p] = sm->rs[r].maxC[p] - sm->rs[r].alloC[p];
                		
			}	

//fprintf(stderr,"after calc need process1\n");
//test(20,0,sm);
//fprintf(stderr,"after calc need process2\n");
//test(20,1,sm);

		//give all the process the shareable resources
		for(p=0;p<18;p++)
			for(r=0;r<20;r++) {
				if(sm->rs[r].shareable == YES) {
					//fill allocae array with max claim of process
					sm->rs[r].allo[p] = sm->rs[r].max[p];
					sm->rs[r].alloC[p] = sm->rs[r].maxC[p];
		
					//reset process needs and request to 0
					sm->rs[r].need[p] = 0;	
					sm->rs[r].needC[p] = 0;
					sm->rs[r].req[p] = 0;
					sm->rs[r].reqC[p] = 0;
				}
			}

//fprintf(stderr,"after allocate shareable process1\n");
//test(20,0,sm);
//fprintf(stderr,"after allocate shareable process2\n");
//test(20,1,sm);

		//give the resource structure copies to test deadlock on
		//hypothetically give process the resources
		for(p=0;p<18;p++)
			for(r=0;r<20;r++) {
				//if(sm->rs[r].req[p] > sm->rs[r].need[p])
					//fprintf(stderr,"throw\n");
			
				if(sm->rs[r].req[p] <= sm->rs[r].avail[r]) {
					sm->rs[r].alloC[p] += sm->rs[r].reqC[p];
					sm->rs[r].availC[r] -= sm->rs[r].reqC[p];
                    sm->rs[r].needC[p] -= sm->rs[r].reqC[p];
                    sm->rs[r].reqC[p] = sm->rs[r].needC[p];
				}
			}		

//fprintf(stderr,"after allocate rest process1\n");
//test(20,18,sm);
//fprintf(stderr,"after allocate rest process2\n");
//test(20,18,sm);

		//use pointers to pass arrays from shared mem to deadlock func
		int *av, *re, *al;
		av = sm->rs[0].availC;
		re = sm->rs[0].reqC;
		al = sm->rs[0].alloC;
		
		dl = deadlock(av,20,18,re,al);
		if(dl == false) {
			//if no deadlock move the original state to the new state
			//fprintf(stderr,"not dl\n");
        	sm->rs[r].allo[p] = sm->rs[r].alloC[p];
      		sm->rs[r].avail[r] = sm->rs[r].availC[r];
        	sm->rs[r].need[p] = sm->rs[r].needC[p];
        	sm->rs[r].req[p] = sm->rs[r].reqC[p];
        }
                  
        else {
			//if deadlocked move new state back to its original
			//fprintf(stderr,"dl\n");
        	sm->rs[r].alloC[p] = sm->rs[r].allo[p];
        	sm->rs[r].availC[r] = sm->rs[r].avail[p];
       		sm->rs[r].needC[p] = sm->rs[r].need[p];
        	sm->rs[r].reqC[p] = sm->rs[r].req[p];
        }

	//}
			
		sm->rand_fork_time-=1; //decrement the time to wait to fork
		sm->lclock.nan+=1;	//increment clock by 1 nanosec each loop.

		//condition to run simulation for 5 logical clock seconds
		//then break out of simulation after 5 seconds
		if(sm->lclock.sec > 3) 
			break;

	}//simulation end	
	
	fprintf(stderr,"simulation is done\n");
	fprintf(stderr,"throughput is %d for %d 3 seconds\n",sm->throughput);

	fprintf(stderr,"deallocating shared mem and sems\n");
	
	if(shmctl(shmID,IPC_RMID,NULL) == -1) {
        perror("failed to remove shared memory");
        exit(1);
    }

    if(semctl(semID,0,IPC_RMID) == -1) {
        perror("failed to remove semaphores");
        exit(1);
    }
	killpg(0,SIGKILL);

return 0;
}//OSS.C

