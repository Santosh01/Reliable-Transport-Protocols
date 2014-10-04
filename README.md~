Reliable-Transport-Protocols
============================



	CONTENTS OF THIS FILE :
	=======================
  	1. Compilation of Program
  	2. Validation of protocols
  	3. Performance comparison
  	4. The Important Variables Define
		a) Part 1 : defines for abt protocol(abt.c)
		b) Part 2 : defines for abt protocol(gbn.c)
		c) Part 3 : defines for abt protocol(sr.c)



1. How to compile the program ?
================================
type a simple command "make" as below,

$ make
The will create a binary executable of all C code files.

Now run each protocols individually

$ ./abt

$ ./gbn

$ ./sr

Above command will run all protocols.

$ make clean 

This will remove all the object file from the directory.


*********************************************************

2. Validation of protocols :
===========================

Run each of your protocols with a total number of 10 messages to be sent by entity A for the
Alternating-Bit-Protocol version and 20 messages for the Go-Back-N and the Selective-Repeat 
versions.Set the loss probability to 0.2, the corruption probability to 0.2, the trace level 
to 2 and the mean time between messages arrivals (from A’s layer5) to 1000 in the case of the 
Alternating-Bit-Protocol and 50 in the case of the other two protocols. For the Go-Back-N and 
the Selective-Repeat versions, set the window size to 10.

*********************************************************


3. Performance comparison :
===========================

In each of the following 2 experiments, run each of your protocols with a total number of 1000
messages to be sent by entity A, a mean time of 50 between message arrivals (from A’s layer5) and a
corruption probability of 0.2.

Experiment 1: 
~~~~~~~~~~~~~
With loss probabilities – {0.1, 0.2, 0.4, 0.6, 0.8}, compare the 3 protocols’ throughputs 
at the application layer of receiver B. Use 2 window sizes - {10, 50} for the 
Go-Back-N version and the Selective-Repeat Version.


Experiment 2: 
~~~~~~~~~~~~~
With window sizes – {10, 50, 100, 200, 500} for GBN and SR, compare the 3 protocols’
throughputs at the application layer of receiver B. Use 3 loss probabilities – {0.2, 0.5, 0.8} for all 3
protocols.

*********************************************************


4. The Important Variables Define :
===================================


Part 1: defines for abt protocol(abt.c):
----------------------------------------
//variables for sender
~~~~~~~~~~~~~~~~~~~~~~
/* Why we choose 20.0 time units as our time out?
   Because each packet's arrival time is between 1 and 10 time units,
   If we could not receive a ACK after 20 time units, the packet 
   MUST have been lost, so we could resend it */
 #define TIMEOUT 20.0		/* timeout */
int A_seqnum = 0;			/* next seqnum for sender */
enum sender_flag A_flag = WAIT_FOR_PKG;	/* current stage of sender */
struct pkt cur_packet;		/* buffer for packets, we only need buffer one packet for abt protocol */
//variables for receiver
int B_seqnum = 0;			/* next expected seqnum */
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


Part 2: defines for gbn protocol(gbn.c):
----------------------------------------

//variables for sender
	#define TIMEOUT 20.0		/* timeout */
	#define N 10				/* window size, may be 10 or 50 */
int base = 0;				/* the oldest not ACKed packet seqnum */
int nextseq = 0;			/* next seqnum for sender */
int packets_base = 0;		/* buffer start index */
struct pkt packets[N];		/* buffer for packets, current buffer size equals to window size */

//variables for receiver
int B_seqnum = 0;			/* next expected seqnum */


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 

Part 3: defines for sr protocol(sr.c):
----------------------------------------
	float g_cur_time = 0.0;		/* current time, when every timeout occur, add INTERVAL	*/
	#define INTERVAL 1.0		/* timer timeout, because we need multiple timers for Selective-Repeat
							   protocol and there is only one hardware timer, we must implement
							   our own's logic timer. As a result, we set the timer timeout after 
							   every INTERVAL time. Then compare current time with the packet's
							   send time to make sure whether we need to resend or not*/
	#define TIMEOUT 20.0		/* timeout for each packet */
	#define N 10				/* window size, maybe 10 or 50 */
	struct ring_buf {			/* buffer for sender and receive */
	  /* For receiver	0: acceptable, 1:data received.
	     For sender		0: acceptable, 1:data sended, 2:ACK received */
	     int flag;
  	     struct pkt packet;
	     float timeout;	/* packet send time */
	};

/* defines for sender */
int A_base = 0;				/* the oldest not ACKed packet seqnum */
int A_nextseq = 0;			/* next seqnum for sender */
/* ring buffer for all packets in window for sender */
int A_ring_start = 0;
struct ring_buf A_buf[N];	/* buffer size equals to window size */

/* defines for recver */
int B_base = 0;				/* the oldest seqnum for packet, that have not send to receiver */
/* ring buffer for all packets in window for receiver */
int B_ring_start = 0;
struct ring_buf B_buf[N];	/* buffer size equals to window size */


*********************************************************************************************************************

Copyright(c) Spring-2014 University at Buffalo, All right reserved.
Author : Santosh Kumar Dubey
	 
