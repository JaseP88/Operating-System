#include <unistd.h>

#define YES 1
#define NO 0
#define RAISE 1
#define DOWN 0
#define READ 0
#define WRITE 1


struct sembuf WAIT;
struct sembuf SIGNAL;

////////////// Page Table Structure ////////////
////////////////////////////////////////////////
typedef struct page_table_structure {
	//int delimiter; // used to cap local  process page table, dependent on size
	unsigned short action : 1;
	unsigned short old : 1;
	//256k system memory each page 1k size.
	//system can fit 256 pages, 2^8 ~ 256;
	//8 bits used to do addressing
	unsigned short frame_addr : 8;

	//val/invalid bit indicates if the page is
	//in memory or not in memory
	//let 1 be in mem, 0 is not in mem
	unsigned short val_bit : 1;

	//utilize by the LRU algorithm
	//unsigned short ref_bit : 1;

	//indicates if the page has been modified
	unsigned short dirty_bit : 1;

	//???
	unsigned int ref_counter; 
} page_t;


/////////// Process Structure //////////////
////////////////////////////////////////////
typedef struct process_structure {
	unsigned int start_time;
	unsigned int end_time;
	unsigned int turnaround;
	
	unsigned int wait_start;
	unsigned int wait_stop;
	unsigned int wait_time;

	unsigned int cpu_util;

	int page_delimit;
	int referencing;
	int page;
	int action;
	
	unsigned int mat_start; //memory access time
	unsigned int mat_stop;
	unsigned int eat;	//effective access time
	unsigned int num_of_ref;

    page_t pg_t[32];
} process;


//////////// Memory Reference Struct ///////////////
////////////////////////////////////////////////////
typedef struct mem_ref_struct {
	int process;
	int page;
	int action;
} mr;

///////////// FIFO Queue Structure /////////////////
////////////////////////////////////////////////////
typedef struct fifo_struct {
    mr mr; //contains the reference from each process **/process/page/action
    struct fifo_struct *next;
} fifo;


///////// Shared Memory Structure //////////////
////////////////////////////////////////////////
typedef struct shared_memory_structure {
	
	//this is the bit vectors to test the system frame if taken or not
	int bit_vector[8]; //8 index for 0-255 bits
	process p[18];

	int throughput;	

	int taken[18]; //boolean array to see which index is free for new process use

	int process_running; //keeps track of the total process_running at a time
	int process_num; //keeps track of the total amount of process forked
	
	mr reference[20]; //reference array;
	
	struct logical_clock { //logical clock for the processes
		unsigned int sec;
		unsigned int nan;
		unsigned int nan_counter;
	} lclock;

	struct page_clock { //logical clock for the pages
		unsigned int page_counter;
	} pclock;

} s_m;


//////////////////////////////////////////////////////
/////////////// All Function Prototypes //////////////
//////////////////////////////////////////////////////
void clearDeviceQ();
mr getReferencingP();
void addToDeviceQ(mr);
int rand_gen(int,int);
int seminit(int,int,int);
void semwait(int,int);
void semsignal(int,int);
pid_t r_wait(int *);	
void setBit(int [], int);
void clearBit(int [], int);
int testBit(int [], int);
int index_open(s_m *);
int free_ref(s_m *);
int free_frame(s_m *);
void chi_exit();
void exit_proc(int);
void sync_clock(unsigned int,s_m *);
int isDeviceQEmpty();
void run_daemon(s_m *);
void disk(char *,int,int);
void savelog(char *,int, unsigned int, unsigned int, unsigned int, unsigned int); 
//void savelog(char *, int, unsigned int);
