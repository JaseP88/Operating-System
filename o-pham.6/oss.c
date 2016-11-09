///////////////////////////////////////////////////
////////////////// OSS.C //////////////////////////
///////////////////////////////////////////////////

#include "header.h"

#include <stdio.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/resource.h>

s_m *shared_struct;

int main(void) {
	srand(time(NULL));
	int i, j;
	
	////// Set Up Shared Memory ///////////
	///////////////////////////////////////
    key_t shmkey;
    
		
	if ((shmkey = ftok("/classes/OS/o-pham/o-pham.6",100)) == (key_t)-1) {
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

    //////// Set Up Semaphore ///////////
    /////////////////////////////////////
	int semID;
    key_t semkey;
		
    if ((semkey = ftok("/classes/OS/o-pham/o-pham.6",101)) == (key_t) -1) {
        perror("ftok error");
        exit(1);
    }
		
    semID = semget(semkey,19,IPC_CREAT | 0666);
    if (semID<0) {
        perror("semget failed");
        exit(1);
    }

	//////// Initialize Semaphores ////////
	///////////////////////////////////////
	for(i=0;i<19;i++)
		seminit(semID,i,0);

	
	/////// Initialize Bit Vectors to 0s //////////
	///////////////////////////////////////////////
	for(i=0;i<256;i++) 
		clearBit(sm->bit_vector,i);		
		//setBit(sm->bit_vector,i);

/////// Bit Vector Debugg //////////////////
////////////////////////////////////////////	
/*
int bitCounter=0;
//bitCounter = testBit(sm->bit_vector,136);
//fprintf(stderr,"bitCOunter is %d\n", bitCounter);

for(i=0;i<256;i++) {
	bitCounter+=testBit(sm->bit_vector,i);
}
if(bitCounter == 0)
fprintf(stderr,"all bits are 0\n");

if(bitCounter == 256)
fprintf(stderr,"all bits are 1\n");

else if(bitCounter > 0 && bitCounter < 256)
fprintf(stderr,"some bits are 1 and some are 0\n");
//////////////////////////////////////////
//////////////////////////////////////////
*/

	/////////// Initialize the Memory Reference members //////
	//////////////////////////////////////////////////////////

	for(i=0; i<18; i++) {
		sm->p[i].page_delimit = 0;
		sm->p[i].page = 0;
		sm->p[i].referencing = NO;
		sm->p[i].action = 0;
		sm->p[i].mat_start = 0;
		sm->p[i].mat_stop = 0;
		sm->p[i].eat = 0;
		sm->p[i].num_of_ref = 0;
		for(j=0; j<32; j++) {
			sm->p[i].pg_t[j].action = 0;
			sm->p[i].pg_t[j].val_bit = 0;
			sm->p[i].pg_t[j].dirty_bit = 0;
			sm->p[i].pg_t[j].ref_counter = 0;
			sm->p[i].pg_t[j].old = NO;
		}
	}

	/////////// Set up the process index boolean array ///////
	//////////////////////////////////////////////////////////
	for(i=0; i<18; i++) {
		sm->taken[i] = NO;
	}

	///////// Start Logical Clock & Shared Mem //////////////
	/////////////////////////////////////////////////////////
	sm->lclock.sec = 0;
	sm->lclock.nan = 0;
	sm->process_running = 0;
	sm->process_num = 0;
	sm->pclock.page_counter = 0;

	////////// Signals /////////////
	////////////////////////////////
	signal(SIGCHLD, chi_exit);
	signal(SIGINT, exit_proc);

	///////// First Fork at Time 0 ///////////////
	//////////////////////////////////////////////
	pid_t pID, pgID;
	pgID = getpgrp();
	int index = 0;
	sleep(1); // so random gen work with child
	
	pID = fork();

	if(pID == -1) {
		perror("fork error");
		exit(1);
	}

	if(pID == 0) { // the child
		pgID = getpgrp();
		char pass_index[sizeof(index)];
		snprintf(pass_index,sizeof(index),"%d",index);
		execl("./process","./process",pass_index,NULL);
	}

	sm->taken[index] = YES;
	sm->process_running++;
	sm->process_num++;
	
/*	index = index_open(sm);
	if(index == -1) {
		fprintf(stderr,"Error No index left for new process process\n");
	}
*/
	////////////// Start of Simulation ////////////////
	///////////////////////////////////////////////////

	int addRand, rand_fork_time_s, rand_fork_time_n;
	int wait_to_fork = NO;
	int isEmpty;
	mr mem_r; //memory reference variable contains process/page/action
	

	for(;;) {

		if(wait_to_fork == NO) {
			addRand = rand_gen(1000,500000);
			rand_fork_time_s = sm->lclock.sec;
			rand_fork_time_n = sm->lclock.nan + addRand;
			if(rand_fork_time_n > 999999999) {
				rand_fork_time_s++;
				rand_fork_time_n = rand_fork_time_n - 999999999;
			}
			wait_to_fork = YES;
		}
		if(sm->lclock.sec >= rand_fork_time_s && sm->lclock.nan >= rand_fork_time_n &&wait_to_fork == YES) {
		//if (sm->rand_fork_time <= 0 && wait_to_fork == YES) {
			/* Limit fork process to 18 process */
			if (sm->process_running < 18) {
				index = index_open(sm);
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

				sm->process_running+=1;//update the total # of process
				sm->process_num+=1;
				sm->taken[index] = YES;
				//index = index_open(sm);//picks free index in taken array for next fork

				/*if(index == -1) {
					fprintf(stderr,"Error No index left for new process\n");
				}*/

				wait_to_fork = NO;
			}	
		}


		///////////////////////////////////////////////////////////////
		/////// Go through process and and Look if the flag ///////////
		/////// for memory reference is ON				 	///////////
		///////////////////////////////////////////////////////////////
		
		for(i=0; i<18; i++) {
			int pg, actn;
			mr mem_r;
			if(sm->p[i].referencing == YES) { //if process is making a reference
				mem_r.page= sm->p[i].page;
				mem_r.action = sm->p[i].action;
				mem_r.process = i;
				
				sm->p[i].mat_stop = sm->lclock.nan_counter;
				sm->p[i].eat += ((sm->p[i].mat_stop - sm->p[i].mat_start)/sm->p[i].num_of_ref);
//fprintf(stderr,"eat %d\n",sm->p[i].eat);
				sm->pclock.page_counter++; //increment seperate logical clock just for LRU
				sm->p[i].pg_t[mem_r.page].ref_counter = sm->pclock.page_counter; //set the page counter for LRU

				if(sm->p[i].pg_t[mem_r.page].val_bit == 0 || mem_r.action == WRITE) {
					if(sm->p[i].pg_t[mem_r.page].old == YES) { //if the referencign page is old reset to not old
						sm->p[i].pg_t[mem_r.page].old = NO;
						sm->p[i].pg_t[mem_r.page].val_bit = 1; //turn valid bit back on
					}
					else
						addToDeviceQ(mem_r); //throw in device Q
				}

				else 
					sync_clock(10,sm); //page already in frame table.
			}

			sm->p[i].referencing == NO; //reset the referencing flag for that process
			semsignal(semID,i);	//then signal
		}

		/////////////////////////////////////////////////
		//////////////// Device Queue ///////////////////
		/////////////////////////////////////////////////
		while(isEmpty = isDeviceQEmpty() != YES) { //while the queue is not empty
			sync_clock(15000,sm); //Simulate Device time
			mr referencingP;
			referencingP = getReferencingP();
			int process_ref = referencingP.process;
			int page_addr_ref = referencingP.page;
			int page_action_ref = referencingP.action;

			run_daemon(sm);			


			//if the refernce is write and already in frame update dirty bit
			if(page_action_ref == WRITE && sm->p[process_ref].pg_t[page_addr_ref].val_bit == 1) 
				sm->p[process_ref].pg_t[page_addr_ref].dirty_bit = 1; // set the dirty bit
			

			else {

				int frame = free_frame(sm); //find the free frame
				if(frame == -1)  //when no free frame left
					fprintf(stderr,"Error there is no frame left\n");
				

				sm->p[process_ref].pg_t[page_addr_ref].frame_addr = frame;	//put the referencing process page into frame table
				sm->p[process_ref].pg_t[page_addr_ref].val_bit = 1;	//set the referencing proess valid/invalid bit to valid
				setBit(sm->bit_vector,frame);	//set the bitvector to mark the frame is in use

//fprintf(stderr,"Process: %d| Page: %d | Frame: %d\n",process_ref+1,page_addr_ref,frame);
	
			}
		}	
/*
		//////////semaphore debug//////////
		///////////////////////////////////		
		for(i=0; i<18; i++) {
			int ncnt;
			ncnt = semctl(semID, i, GETNCNT);
			//fprintf(stderr,"ncnt for sem %d is %d\n",i,ncnt);
		}
*/
/////////////////////////////////////////////////////////////////////
//////////////////////// Displaying Frame Table /////////////////////
/////////////////////////////////////////////////////////////////////
for(i=0; i<256; i++) {
	if(testBit(sm->bit_vector,i) == 1) {
		if(i==31 || i==63 || i==95 || i==127 || i==159 || i==191 || i==223)
			fprintf(stderr,"1\n");
		else if(i == 255)
			fprintf(stderr,"1\n\n");
		else
			fprintf(stderr,"1");
	}

	else {
		if(i==31 || i==63 || i==95 || i==127 || i==159 || i==191 || i==223)
			fprintf(stderr,".\n");
		else if(i == 255) 
			fprintf(stderr,".\n\n");
		else
			fprintf(stderr,".");
	}
}
		
		sync_clock(1,sm);
//fprintf(stderr,"%d\n",sm->lclock.nan_counter);
		if(sm->lclock.sec >= 3) //ends simulation after 3 seconds
			break;
	}//End of simulation

fprintf(stderr,"3 second up simulation done\n");
fprintf(stderr,"throughput is %d for 3 seconds\n",sm->throughput);
fprintf(stderr,"removing shared sems and memory\n");
	
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
}
