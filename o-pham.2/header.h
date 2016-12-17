/*$Author: o-pham $*/
/*$Date: 2016/02/24 06:34:50 $*/
/*$Log: header.h,v $
 *Revision 1.5  2016/02/24 06:34:50  o-pham
 *finish version
 *
 *Revision 1.4  2016/02/19 07:29:26  o-pham
 *added new function like signals and stuff
 *
 *Revision 1.3  2016/02/18 10:42:15  o-pham
 **** empty log message ***
 *
 *Revision 1.1  2016/02/17 23:52:42  o-pham
 *Initial revision
 **/

typedef enum {idle, want_in, in_cs} state;

typedef struct data_structure {
	state flag[19]; // the state of each 19 proccesses
	int turn;
} p_s;

void process (const int i);
void critical_section (int);
void remainder_section (int, pid_t, int);
void writeToFile (int);
void times_up (int i);
void exit_proc (int i);
