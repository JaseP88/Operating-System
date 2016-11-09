#include "header.h"

#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

s_m *shared_struct;

struct sembuf WAIT = {0,-1,0};
struct sembuf SIGNAL = {0,1,0};

static fifo *head = NULL;
static fifo *tail = NULL;

////// Random Generator Function ////////
/////////////////////////////////////////
int rand_gen(int min, int max) {
    int num;
    num = (rand()%(max-min+1))+min;
    return num;
}

//////// Add value to Fifo queue /////////
//////////////////////////////////////////
void addToDeviceQ(mr m) {
    fifo *new;

    /////// Create a New Node to Insert Val //////////
    new = (fifo *)malloc(sizeof(fifo *));
    if (new == NULL) {
        perror("ERROR");
        exit (1);
    }
    new->mr.process = m.process;
    new->mr.page = m.page;
    new->mr.action = m.action;
    new->next = NULL;

    //////// If Head is Empty Assign the New Node To Head ////////
    //////// Else Put the New Node to Tail  //////////////////////
    if(head == NULL)
        head = new;
    else
        tail->next = new;

    //////// Update Tail to become new Node ////////////
    tail = new;
}

/////// Return the Firt Values Entered ///////////
//////////////////////////////////////////////////
mr getReferencingP() {
    mr returnVal, emptyVal;
    fifo *temp;

    emptyVal.process = -1;

    ///// If there is an Element in Queue return that value ///////
    ///// and set head as the next value /////////////////////////
    if(head!=NULL) {
        returnVal = head->mr;
        temp = head->next;
        free(head);
        head = NULL;
        head = temp;
    }
    
    else {
        printf("no Process in queue\n");
        return emptyVal;
    }

    return returnVal;
}

///////// Loops throught the Fifo structure and reset all nodes /////
/////////////////////////////////////////////////////////////////////
void clearDeviceQ() {
    fifo *temp;
    tail = NULL;
    while(head != NULL) {
        temp = head->next;
        free(head);
        head = NULL;
        head = temp;
    }
}

int isDeviceQEmpty() {
    if(head==NULL)
        return YES;
    else
        return NO;
}

///// Initialize Semaphore //////
/////////////////////////////////
int seminit(int semid,  int semnum, int semvalue) {
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } arg;
    arg.val = semvalue;
    return semctl(semid,semnum,SETVAL,arg);
}


///// Semaphore Wait Function /////
///////////////////////////////////
void semwait(int semID, int semnum) {
    WAIT.sem_num = semnum;
    if(semop(semID,&WAIT,1) == -1) {
        perror("semop wait error");
        exit(1);
    }
return;
}

///// Semaphore Signal Function /////
/////////////////////////////////////
void semsignal(int semID, int semnum) {
    SIGNAL.sem_num = semnum;
    if(semop(semID,&SIGNAL,1) == -1) {
        perror("semop signal error");
        exit(1);
    }
return;
}

//////// Get Page Routine Function /////////
////////////////////////////////////////////






///// Wait Function to Sucessfully Exit Child ///////
/////////////////////////////////////////////////////
pid_t r_wait(int *stat_loc) {
    int retval;

    while(((retval = wait(stat_loc)) == -1) && (errno == EINTR));
    return retval;
}

//// Function to Set Bits in Bit Vectors ///////////
////////////////////////////////////////////////////
void setBit(int A[], int k) {
    A[k/32] |= 1 << (k%32);
}

//// Function To Clear the Bits in the Bit Vectors to 0 ///////
///////////////////////////////////////////////////////////////
void clearBit(int A[], int k) {
    A[k/32] &= ~(1 << (k%32));
}

//// Function to Return Bit Value //////
////////////////////////////////////////
int testBit(int A[], int k) {
    return ( (A[k/32] & (1 << (k%32) )) != 0 );     
}

//// Function Used To Find Free Index To Use ///////////
////////////////////////////////////////////////////////
int index_open(s_m *s) {
    int i;
    for(i=0;i<18;i++) {
        if(s->taken[i] == NO)
            return i;
    }
    return -1;
}

///// Function to Find free Index in Reference Array ///////
////////////////////////////////////////////////////////////
int free_ref(s_m *s) {
    int i;
    for(i=0;i<20; i++) {
        if(s->reference[i].process == -1)
            return i;
    }
    //fprintf(stderr,"No room in reference array\n");
    return -1;
}

/////// Function to Find an Open Frame in Bit vector ///////
////////////////////////////////////////////////////////////
int free_frame(s_m *s) {
    int i, f;
    for(i=0; i<256; i++) {  
        f = testBit(s->bit_vector,i);
        if (f == 0)
            return i;
    }
    return -1;
}

//// Function To Signal That a Child Process Has Exited ////
////////////////////////////////////////////////////////////
void chi_exit() {
    pid_t childPID;

    while(childPID = waitpid(-1,NULL,WNOHANG))
        if((childPID == -1)&&(errno != EINTR))
            break;
}

//// Function to Handle an Interrupt Signal //////
//////////////////////////////////////////////////
void exit_proc(int i) {
    key_t shmkey;
    if((shmkey = ftok("/classes/OS/o-pham/o-pham.6",100)) == (key_t)-1) {
        perror("ftok error");
        exit(1);
    }
    int shmID;
    s_m *sm;

    shmID = shmget(shmkey,sizeof(s_m),IPC_CREAT | 0666);
    if(shmID<0) {
        perror("shmget failed");
        exit(1);
    }

    sm = (s_m *)shmat(shmID,(void *)0,0);
    if(sm<(s_m *) (0)) {
        perror("shmat failed");
        exit(1);
    }

    shared_struct = sm;

    ///////// Set Up Semaphore ///////////////
    //////////////////////////////////////////
    int semID;
    key_t semkey;
    if ((semkey = ftok("/classes/OS/o-pham/o-pham.6",101)) == (key_t)-1) {
        perror("ftok error");
        exit(1);
    }
    semID = semget(semkey,19,IPC_CREAT | 0666);
    if (semID<0) {
        perror("semget failed");
        exit(1);
    }

    //////////// The Handler /////////////////
    //////////////////////////////////////////
    signal(SIGINT, exit_proc);  //the signal

	printf("throughput is %d for %d:%09d\n",sm->throughput,sm->lclock.sec,sm->lclock.nan);

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

//////////// Function For Page To Write to Disk /////////////
/////////////////////////////////////////////////////////////
void disk(char *filename, int proc, int page) {
	FILE *fp;
	
	if(filename == NULL) {
    	perror("error");
   		return;
    }

    if((fp=fopen(filename,"a")) == NULL) {
        perror("error");
        return;
    }

    fprintf(fp,"process %d page %d put to disk\n",proc,page);
    fclose(fp);
}

//////////// Function to Log terminated Process ////////////
////////////////////////////////////////////////////////////
void savelog(char *filename, int p, unsigned int eat, unsigned int tt, unsigned int wait_time, unsigned int cpu) {
//void savelog(char *filename, int p, unsigned int eat) {	
	FILE *fp;

	if(filename == NULL) {
		perror("Filename ERROR");
		return;
	}

	if((fp=fopen(filename,"a")) == NULL) {
        perror("fopen error");
        return;
    }

	fprintf(fp,"process %d, eat:%d, cpu_u:%d, wait time:%d, turnaround time:%d\n",p,eat,cpu,wait_time,tt);
	//fprintf(fp,"process %d, eat:%d\n",p,eat);
	fclose(fp);
}


////////////// Function to Sync The Clock /////////////
///////////////////////////////////////////////////////
void sync_clock(unsigned int nan,s_m *sm) {
    unsigned int diff;
    unsigned int update;
	sm->lclock.nan_counter += nan;
    update = sm->lclock.nan+nan;
    if (update == 1000000000) {
        sm->lclock.sec++;
        sm->lclock.nan = 0;
    }
    if (update > 1000000000) {
            sm->lclock.sec++;
            diff = update - 1000000000;
            sm->lclock.nan = diff;
            //fprintf(stderr,"%d second\n",sm->lclock.sec);
    }
    else
        sm->lclock.nan = update;
    
}

///////////////////////////////////////////////////
///////////////// Daemon Function /////////////////
///////////////////////////////////////////////////
void run_daemon (s_m *s) {

    int i, j, k;
    int n;
    unsigned int oldest_counter;
    int oldest_page, oldest_process;
    int frame_counter = 0;
    int delimit;
    
    /////////// First checks amount used in frame ///////////
    /////////////////////////////////////////////////////////
    for(i=0; i<256; i++) //find the total used frames
        frame_counter += testBit(s->bit_vector,i);
	//fprintf(stderr,"frame_counter is %d\n",frame_counter);

    
    oldest_counter = 3000000000; //problem of overflow

    n = frame_counter * 0.05; //free n amount of frames


    if(frame_counter >= 230) { //sweep now!
		for(i=0; i<18; i++) {
            delimit = s->p[i].page_delimit; //get the page delimiter (max requirement) for each process
            for(j=0; j<delimit; j++) 
                if(s->p[i].pg_t[j].old == YES) {
					if(s->p[i].pg_t[j].dirty_bit == 1) { //if page replacing has a dirty bit
						disk("disk",i,j); //write to disk
                    	s->p[i].pg_t[j].dirty_bit = 0;	//reset the dirty_bit
					}
                    s->p[i].pg_t[j].val_bit = 0; //reset valid bits
                    clearBit(s->bit_vector,s->p[i].pg_t[j].frame_addr); //free the frame
                    s->p[i].pg_t[j].old == NO; //reset
                }
        }

        for(k=0; k<n; k++) {// repeat n amount of times
            for(i=0; i<18; i++) {
                delimit = s->p[i].page_delimit; //get the page delimiter (max requirement) for each process
                for(j=0; j<delimit; j++) 
     				if((s->p[i].pg_t[j].ref_counter < oldest_counter) && (s->p[i].pg_t[j].val_bit == 1) && (s->p[i].pg_t[j].old == NO)) {
                        oldest_counter = s->p[i].pg_t[j].ref_counter;
                        oldest_page = j;
                        oldest_process = i;
                    }
            }

            s->p[oldest_process].pg_t[oldest_page].val_bit = 0; //reset valid bits
			s->p[oldest_process].pg_t[oldest_page].old = YES;
            return;
        }
    }
	else
		return;
}
