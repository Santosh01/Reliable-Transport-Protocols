#include <stdio.h>
#include <stdlib.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
                           /* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[20];
  };

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
    };

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/


/* comm defines */
#define   A    0
#define   B    1
/* timeout for the timer */
#define INTERVAL 1.0
#define TIMEOUT 20.0
/* when A send a data package to B,
   we do not need to set acknum,
   so we set it to this default number */
#define DEFAULT_ACK 111
/* define window size */
#define N 10
struct ring_buf {
  /* For receiver	0: acceptable, 1:data received.
     For sender		0: acceptable, 1:data sended, 2:ACK received */
  int flag;
  struct pkt packet;
  float timeout;	/* packet send time */
};
float g_cur_time = 0.0;

/* defines for recver */
int B_base = 0;
int B_ring_start = 0;
struct ring_buf B_buf[N];

/* defines for statistic */
int A_from_layer5 = 0;
int A_to_layer3 = 0;
int B_from_layer3 = 0;
int B_to_layer5 = 0;

/* defines for sender */
int A_base = 0;
int A_ring_start = 0;
int A_nextseq = 0;
/* ring buffer for all packets in window */
struct ring_buf A_buf[N];

/* extra buffer used when window is full */
struct node {
	struct msg message;
	struct node *next;
};
struct node *list_head = NULL;
struct node *list_end = NULL;
int Extra_BufSize = 0;
void append_msg(struct msg *m)
{
	int i;
	/*allocate memory*/
	struct node *n = malloc(sizeof(struct node));
	if(n == NULL) {
		printf("No enough memory");
		return;
	}
	n->next = NULL;
	/*copy packet*/
	for(i = 0; i < 20; ++i) {
		n->message.data[i] = m->data[i];
	}

	/* if list empty, just add into the list*/
	if(list_end == NULL) {
		list_head = n;
		list_end = n;
		++Extra_BufSize;
		return;
	}
	/* otherwise, add at the end*/
	list_end->next = n;
	list_end = n;
	++Extra_BufSize;
}
struct node *pop_msg()
{
	struct node *p;
	/* if the list is empty, return NULL*/
	if(list_head == NULL) {
		return NULL;
	}

	/* retrive the first node*/
	p = list_head;
	list_head = p->next;
	if(list_head == NULL) {
		list_end = NULL;
	}
	--Extra_BufSize;

	return p;
}


/* common tools */
/*This is the implementation of checksum. Data packets and ACK packets use the same method. */
/* 
 * There will be no carry flow.
 * seqnum < 1000;
 * acknum < 1000;
 * each payload < 255;  
 * so the maximum of checksum < 1000 + 1000 + 20*255 < MAX_INT
 * Why add seqnum and acknum?
Because checksum is used to make sure the packet is correct, not corrupted.
Seqnum and acknum may also be corrupted, so we need to add seqnum and acknum.
 
 * About the check sum:
 * There is no perfect method for check sum, only suitable method.  
 * In this assignment, we only need to add all of them as it is simple and it works well.
*/

int calc_checksum(struct pkt *p)
{
  int i;
  int checksum = 0; /*First init to Zero*/
  if(p == NULL)
  {
    return checksum;
  }
/*Add all the characters in payload*/
  for (i=0; i<20; i++)
  {
    checksum += (unsigned char)p->payload[i];
  }
  /*add the seqnum*/
  checksum += p->seqnum;
  /*add the acknum*/
  checksum += p->acknum;
 /*Then we get the final checksum.*/
  return checksum;
}
void Debug_Log(int AorB, char *msg, struct pkt *p, struct msg *m)
{
  char ch = (AorB == A)?'A':'B';
  if(AorB == A)
  {
    if(p != NULL) {
      printf("[%c] %s. Window[(%d,%d) Packet[seq=%d,ack=%d,check=%d,data=%c..]\n", ch, msg,
      A_base, A_nextseq, p->seqnum, p->acknum, p->checksum, p->payload[0]);
    } else if(m != NULL) {
      printf("[%c] %s. Window[(%d,%d) Message[data=%c..]\n", ch, msg, A_base, A_nextseq, m->data[0]);
    } else {
      printf("[%c] %s.Window[(%d,%d)\n", ch, msg, A_base, A_nextseq);
    }
  }
  else
  {
    if(p != NULL) {
      printf("[%c] %s. Base[%d] Packet[seq=%d,ack=%d,check=%d,data=%c..]\n", ch, msg,
      B_base, p->seqnum, p->acknum, p->checksum, p->payload[0]);
    } else if(m != NULL) {
      printf("[%c] %s. Base[%d] Message[data=%c..]\n", ch, msg, B_base, m->data[0]);
    } else {
      printf("[%c] %s. Base[%d]\n", ch, msg, B_base);
    }
  }
}
/* helper functions for the window */
int window_isfull()
{
  if(A_nextseq >= A_base + N)
    return 1;
  else
    return 0;
}
/* copy packet */
void copy_packet(struct pkt *dst, struct pkt *src)
{
  int i;
  if(dst == NULL || src == NULL)
    return;
  dst->seqnum = src->seqnum;
  dst->acknum = src->acknum;
  dst->checksum = src->checksum;
  for(i = 0; i < 20; ++i)
  {
    dst->payload[i] = src->payload[i];
  }
}
/* functions for B */
struct ring_buf *get_ring_buf(int AorB, int seqnum)
{
  int cur_index = 0;
  if(AorB == A)
  {
    if(seqnum < A_base || seqnum >= A_base + N)
    {
      //Debug_Log(A, "Seqnum is not within the window", NULL, NULL);
      return NULL;
    }

    cur_index = (A_ring_start + seqnum - A_base) % N;
    return &(A_buf[cur_index]);
  }
  else
  {
    if(seqnum < B_base || seqnum >= B_base + N)
    {
      //Debug_Log(B, "Seqnum is not within the window", NULL, NULL);
      return NULL;
    }

    cur_index = (B_ring_start + seqnum - B_base) % N;
    return &(B_buf[cur_index]);
  }
}

/* check if we need to free send buffer */
void free_send_buf()
{
  int i;
  int count = 0;
  /* check the receive window */
  for(i = A_base; i < A_base + N; ++i)
  {
    struct ring_buf *p = get_ring_buf(A, i);
    /* if not sequence, break the search */
    if(p == NULL || p->flag != 2)
      break;
    /* send packet to layer 5 */
    p->flag = 0;
    ++count;
  }

  /* adjust expected number and ring base */
  A_base += count;
  A_ring_start += count;

  /*when the window size decreases,check the extra buffer, if we need any message need to send */
  while(count>0) {
    struct node *n = pop_msg();
    if(n == NULL) {
      break;
    }
    A_output(n->message);
    free(n);
    count--;
  }
}

/* called from layer 5, passed the data to be sent to other side */
/* Every time there is a new packet come, 
 * a) we append this packet at the end of the extra buffer.
 * b) Then we check the window is full or not; If the window is full, we just leave the packet in the extra buffer;
 * c) If the window is not full, we retrieve one packet at the beginning of the extra buffer, and process it. 
 */
A_output(message)
  struct msg message;
{
printf("================================ Inside A_output===================================\n");
  int i;
  int checksum = 0;
  struct ring_buf *p = NULL;
  struct node *n;

   /* append message to buffer */
  /*Step (a)*/
  append_msg(&message);

  /* If the last packet have not been ACKed, just drop this message */
  /*Step (b)*/
  if(window_isfull())
  {
    Debug_Log(A, "Drop this message, the window is full already", NULL, &message);
    return;
  }

  /* pop a message from the extra buffer */
  /* Step(c)*/
  n = pop_msg();
  if(n == NULL)
  {
	printf("No message need to process\n");
    return 0;
  }

  /* get a free packet from the buffer */
  p = get_ring_buf(A, A_nextseq);
  if(p == NULL || p->flag == 1)
  {
    Debug_Log(A, "BUG! The window is full already", NULL, &message);
    return;
  }
  ++A_from_layer5;
  Debug_Log(A, "Receive a message from layer5", NULL, &message);
  /* copy data from msg to pkt */
  for (i=0; i<20; i++)
  {
    p->packet.payload[i] = n->message.data[i];
  }
  /* after used, free the node */
  free(n);

  /* set current seqnum */
  p->packet.seqnum = A_nextseq;
  /* we are send package, do not need to set acknum */
  p->packet.acknum = DEFAULT_ACK;

  /* calculate check sum including seqnum and acknum */
  checksum = calc_checksum(&(p->packet));
  /* set check sum */
  p->packet.checksum = checksum;

  p->flag = 1;
  p->timeout = g_cur_time + TIMEOUT;

  /* send pkg to layer 3 */
  tolayer3(A, p->packet);
  ++A_to_layer3;
  ++A_nextseq;

  Debug_Log(A, "Send packet to layer3", &(p->packet), &message);

  return 0;
 printf("================================ Outside A_output===================================\n");
}

B_output(message)  /* need be completed only for extra credit */
  struct msg message;
{
}

/* called from layer 3, when a packet arrives for layer 4 */
A_input(packet)
  struct pkt packet;
{
printf("================================ Inside A_input===================================\n");
  struct ring_buf *p;

  Debug_Log(A, "Receive ACK packet from layer3", &packet, NULL);

  /* check checksum, if corrupted, do nothing */
  if(packet.checksum != calc_checksum(&packet))
  {
    Debug_Log(A, "ACK packet is corrupted", &packet, NULL);
    return;
  }

  /* Duplicate ACKs, do nothing */
  if(packet.acknum < A_base)
  {
    Debug_Log(A, "Receive duplicate ACK", &packet, NULL);
    return;
  }

  p = get_ring_buf(A, packet.acknum);
  if(p == NULL)
  {
    Debug_Log(A, "BUG: receive ACK of future packets", &packet, NULL);
    return;
  }

  p->flag = 2;

  /* go to the next seq, and stop the timer */
  free_send_buf();

  Debug_Log(A, "ACK packet process successfully accomplished!!", &packet, NULL);
 printf("================================ Outside A_input===================================\n");
}

/* called when A's timer goes off */
A_timerinterrupt()
{
  int i;

  g_cur_time += INTERVAL;

  /* if current package timeout, we resend it */
  for(i = A_base; i < A_nextseq; ++i)
  {
    struct ring_buf *p = get_ring_buf(A, i);
    if(p != NULL && p->flag == 1 && g_cur_time > p->timeout)
    {
      tolayer3(A, p->packet);
      ++A_to_layer3;
      /* reset the timeout of current packet */
      p->timeout = (g_cur_time + TIMEOUT);
      Debug_Log(A, "Timeout! Send out the package again", &(p->packet), NULL);
    }
  }

  /* start the timer again */
  starttimer(A, INTERVAL);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
A_init()
{
  int i;
  for(i = 0; i < N; ++i)
  {
    A_buf[i].flag = 0;
  }

  /* start the timer, timeout is INTERVAL, not TIMEOUT */
  starttimer(A, INTERVAL);
}


/* check if we need to send data to layer5 */
void free_recv_buf()
{
  int i;
  int count = 0;
  /* check the receive window */
  for(i = B_base; i < B_base + N; ++i)
  {
    struct ring_buf *p = get_ring_buf(B, i);
    /* if not sequent, break the search */
    if(p == NULL || p->flag != 1)
      break;
    /* send packet to layer 5 */
    tolayer5(B, p->packet.payload);
    ++B_to_layer5;
    Debug_Log(B, "Send packet to layer5", &(p->packet), NULL);
    p->flag = 0;
    ++count;
  }

  /* adjust expected number and ring base */
  B_base += count;
  B_ring_start += count;
}
/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
B_input(packet)
  struct pkt packet;
{
printf("================================ Inside B_input===================================\n");
  Debug_Log(B, "Receive a packet from layer3", &packet, NULL);
  ++B_from_layer3;

  /* check checksum, if corrupted, just drop the package */
  if(packet.checksum != calc_checksum(&packet))
  {
    Debug_Log(B, "Packet is corrupted", &packet, NULL);
    return;
  }

  /* if packet outside of the window, MUST be a error packet */
  if(packet.seqnum >= B_base + N)
  {
    Debug_Log(B, "BUG! receive packet with large seqnum", &packet, NULL);
    return;
  }

  struct ring_buf *p = get_ring_buf(B, packet.seqnum);
  /* duplicate package, send back a ACK.
     case 1: seqnum < B_base;
     case 2: the packet already buffered  */
  if(p == NULL || p->flag == 1)
  {
    Debug_Log(B, "Duplicate packet detected", &packet, NULL);
    packet.acknum = packet.seqnum;
    packet.checksum = calc_checksum(&packet);
    tolayer3(B, packet);
    Debug_Log(B, "Send ACK packet to layer3", &packet, NULL);
    return;
  }

  /* buffer current packet */
  p->flag = 1;
  copy_packet(&p->packet, &packet);

  /* send ack to sender */
  packet.acknum = packet.seqnum;
  packet.checksum = calc_checksum(&packet);
  tolayer3(B, packet);
  Debug_Log(B, "Send ACK packet to layer3", &packet, NULL);

  /* check if we need to send data to layer5 */
  free_recv_buf();
  printf("================================ Outside B_input(packet) =========================\n");
}

/* called when B's timer goes off */
B_timerinterrupt()
{
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
B_init()
{
  int i;
  for(i = 0; i < N; ++i)
  {
    B_buf[i].flag = 0;
  }
}


/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
 };
struct event *evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */ 
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */   
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

main()
{
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;
   
   int i,j;
   char c; 
  
   init();
   A_init();
   B_init();
   
   while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
           goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
           evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0)
	       printf(", timerinterrupt  ");
             else if (eventptr->evtype==1)
               printf(", fromlayer5 ");
             else
	     printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
           }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim==nsimmax)
	  break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5 ) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */    
            j = nsim % 26; 
            for (i=0; i<20; i++)  
               msg2give.data[i] = 97 + j;
            if (TRACE>2) {
               printf("          MAINLOOP: data given to student: ");
                 for (i=0; i<20; i++) 
                  printf("%c", msg2give.data[i]);
               printf("\n");
	     }
            nsim++;
            if (eventptr->eventity == A) 
               A_output(msg2give);  
             else
               B_output(msg2give);  
            }
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)  
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet by calling */
   	       A_input(pkt2give);            /* appropriate entity */
            else
   	       B_input(pkt2give);
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A) 
	       A_timerinterrupt();
             else
	       B_timerinterrupt();
             }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
   printf("===========================================================================\n");
   printf("Simulator terminated at time %f,\nAfter sending %d msgs from layer5\n",time,nsim);
   /* print statistical messages */
   printf("\n[%d] of messages still left in extra buffer of Sender A.\n==========================================================================\n", Extra_BufSize);
   printf("Result: Protocol: [gbn]\n");
   printf("***********************\n");
   printf("[%u] of packets sent from the Application Layer of Sender A\n", A_from_layer5);
   printf("[%u] of packets sent from the Transport Layer of Sender A\n", A_to_layer3);
   printf("[%u] packets received at the Transport layer of Receiver B\n", B_from_layer3);
   printf("[%u] of packets received at the Application layer of Receiver B\n", B_to_layer5);
   printf("Total time: [%f] time units\n", time/lambda);
   printf("Throughput = [%f] packets/time units\n", B_to_layer5/time);
}



init()                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();
  
  
   printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: ");
   scanf("%d",&nsimmax);
   printf("Enter  packet loss probability [enter 0.0 for no loss]:");
   scanf("%f",&lossprob);
   printf("Enter packet corruption probability [0.0 for no corruption]:");
   scanf("%f",&corruptprob);
   printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
   scanf("%f",&lambda);
   printf("Enter TRACE:");
   scanf("%d",&TRACE);

   srand(9999);              /* init random number generator */
   sum = 0.0;                /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" ); 
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(1);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time=0.0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() 
{
  double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */ 
  x = rand()/mmm;            /* x should be uniform in [0,1] */
  return(x);
}  

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
 
generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
    //char *malloc();
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");
 
   x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                             /* having mean of lambda        */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
} 


insertevent(p)
   struct event *p;
{
   struct event *q,*qold;

   if (TRACE>2) {
      printf("            INSERTEVENT: time is %lf\n",time);
      printf("            INSERTEVENT: future time will be %lf\n",p->evtime); 
      }
   q = evlist;     /* q points to header of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
        }
     else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
              qold=q; 
        if (q==NULL) {   /* end of list */
             qold->next = p;
             p->prev = qold;
             p->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p->next=evlist;
             p->prev=NULL;
             p->next->prev=p;
             evlist = p;
             }
           else {     /* middle of list */
             p->next=q;
             p->prev=q->prev;
             q->prev->next=p;
             q->prev=p;
             }
         }
}

printevlist()
{
  struct event *q;
  int i;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
    }
  printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
stoptimer(AorB)
int AorB;  /* A or B is trying to stop timer */
{
 struct event *q,*qold;

 if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time);
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
       /* remove this event */
       if (q->next==NULL && q->prev==NULL)
             evlist=NULL;         /* remove first and only event on list */
          else if (q->next==NULL) /* end of list - there is one in front */
             q->prev->next = NULL;
          else if (q==evlist) { /* front of list - there must be event after */
             q->next->prev=NULL;
             evlist = q->next;
             }
           else {     /* middle of list */
             q->next->prev = q->prev;
             q->prev->next =  q->next;
             }
       free(q);
       return;
     }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


starttimer(AorB,increment)
int AorB;  /* A or B is trying to stop timer */
float increment;
{

 struct event *q;
 struct event *evptr;
 //char *malloc();

 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time);
 /* be nice: check to see if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)  
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }
 
/* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
} 


/************************** TOLAYER3 ***************/
tolayer3(AorB,packet)
int AorB;  /* A or B is trying to stop timer */
struct pkt packet;
{
 struct pkt *mypktptr;
 struct event *evptr,*q;
 //char *malloc();
 float lastime, x, jimsrand();
 int i;


 ntolayer3++;

 /* simulate losses: */
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0)    
	printf("          TOLAYER3: packet being lost\n");
      return;
    }  

/* make a copy of the packet student just gave me since he/she may decide */
/* to do something with the packet after we return back to him/her */ 
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 mypktptr->seqnum = packet.seqnum;
 mypktptr->acknum = packet.acknum;
 mypktptr->checksum = packet.checksum;
 for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	  mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
        printf("%c",mypktptr->payload[i]);
    printf("\n");
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
 lastime = time;
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) ) 
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();
 


 /* simulate corruption: */
 if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
       mypktptr->payload[0]='Z';   /* corrupt payload */
      else if (x < .875)
       mypktptr->seqnum = 999999;
      else
       mypktptr->acknum = 999999;
    if (TRACE>0)    
	printf("          TOLAYER3: packet being corrupted\n");
    }  

  if (TRACE>2)  
     printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
} 

tolayer5(AorB,datasent)
  int AorB;
  char datasent[20];
{
  int i;  
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)  
        printf("%c",datasent[i]);
     printf("\n");
   }
  
}
