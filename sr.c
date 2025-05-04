#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "emulator.h"
#include "sr.h"

/* ******************************************************************
   Selective Repeat protocol.  Adapted from J.F.Kurose Go-Back-N
   ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.2  

   Network properties:
   - one way network delay averages five time units (longer if there
   are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
   or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
   (although some can be lost).

   Modifications: 
   - removed bidirectional GBN code and other code not used by prac. 
   - fixed C style to adhere to current programming style
   - added Selective Repeat implementation
**********************************************************************/

#define RTT  16.0       /* round trip time.  MUST BE SET TO 16.0 when submitting assignment */
#define WINDOWSIZE 6    /* the maximum number of buffered unacked packet */
#define SEQSPACE 7      /* the min sequence space for SR must be at least windowsize + 1 */
#define NOTINUSE (-1)   /* used to fill header fields that are not being used */
#define UNACKED (0)     /* packet has not been acknowledged */
#define ACKED (1)       /* packet has been acknowledged */

/* generic procedure to compute the checksum of a packet.  Used by both sender and receiver  
   the simulator will overwrite part of your packet with 'z's.  It will not overwrite your 
   original checksum.  This procedure must generate a different checksum to the original if
   the packet is corrupted.
*/
int ComputeChecksum(struct pkt packet)
{
  int checksum = 0;
  int i;

  checksum = packet.seqnum;
  checksum += packet.acknum;
  for ( i=0; i<20; i++ ) 
    checksum += (int)(packet.payload[i]);

  return checksum;
}

bool IsCorrupted(struct pkt packet)
{
  if (packet.checksum == ComputeChecksum(packet))
    return (false);
  else
    return (true);
}


/********* Sender (A) variables and functions ************/

static struct pkt buffer[WINDOWSIZE];  /* array for storing packets waiting for ACK */
static int windowfirst, windowlast;    /* array indexes of the first/last packet awaiting ACK */
static int windowcount;                /* the number of packets currently awaiting an ACK */
static int A_nextseqnum;               /* the next sequence number to be used by the sender */

/* SR specific variables for sender */
static int ack_status[WINDOWSIZE];     /* track if packet is acknowledged */
static int timer_active;               /* flag to track if timer is active */
static int earliest_unacked;           /* track earliest unacked packet for timing */

/* helper function to find the index for a sequence number */
int find_buffer_index(int seqnum) {
  int i;
  for (i = 0; i < windowcount; i++) {
    int buffer_index = (windowfirst + i) % WINDOWSIZE;
    if (buffer[buffer_index].seqnum == seqnum) {
      return buffer_index;
    }
  }
  return -1;
}

/* helper function to find earliest unacked packet */
void find_earliest_unacked(void) {
  int i;
  earliest_unacked = -1;
  
  for (i = 0; i < windowcount; i++) {
    int buffer_index = (windowfirst + i) % WINDOWSIZE;
    if (ack_status[buffer_index] == UNACKED) {
      earliest_unacked = buffer_index;
      break;
    }
  }
}

/* called from layer 5 (application layer), passed the message to be sent to other side */
void A_output(struct msg message)
{
  struct pkt sendpkt;
  int i;

  /* if not blocked waiting on ACK */
  if ( windowcount < WINDOWSIZE) {
    if (TRACE > 1)
      printf("----A: New message arrives, send window is not full, send new messge to layer3!\n");

    /* create packet */
    sendpkt.seqnum = A_nextseqnum;
    sendpkt.acknum = NOTINUSE;
    for ( i=0; i<20 ; i++ ) 
      sendpkt.payload[i] = message.data[i];
    sendpkt.checksum = ComputeChecksum(sendpkt); 

    /* put packet in window buffer */
    windowlast = (windowlast + 1) % WINDOWSIZE; 
    buffer[windowlast] = sendpkt;
    windowcount++;
    
    /* mark packet as unacknowledged */
    ack_status[windowlast] = UNACKED;

    /* send out packet */
    if (TRACE > 0)
      printf("Sending packet %d to layer 3\n", sendpkt.seqnum);
    tolayer3 (A, sendpkt);

    /* start timer if no timer is active */
    if (!timer_active) {
      starttimer(A, RTT);
      timer_active = 1;
      earliest_unacked = windowlast;
    }

    /* get next sequence number, wrap back to 0 */
    A_nextseqnum = (A_nextseqnum + 1) % SEQSPACE;  
  }
  /* if blocked,  window is full */
  else {
    if (TRACE > 0)
      printf("----A: New message arrives, send window is full\n");
    window_full++;
  }
}


/* called from layer 3, when a packet arrives for layer 4 
   In this practical this will always be an ACK as B never sends data.
*/
void A_input(struct pkt packet)
{

  int found = 0;
  int buffer_index;

  /* if received ACK is not corrupted */ 
  if (!IsCorrupted(packet)) {
    if (TRACE > 0)
      printf("----A: uncorrupted ACK %d is received\n",packet.acknum);
    total_ACKs_received++;

    /* find the packet being acknowledged */
    buffer_index = find_buffer_index(packet.acknum);
    
    if (buffer_index != -1 && ack_status[buffer_index] == UNACKED) {
      
      found = 1;  /* Mark that we found the packet */
      
      /* mark packet as acknowledged */
      ack_status[buffer_index] = ACKED;
      new_ACKs++;
      
      if (TRACE > 0)
        printf("----A: ACK %d is not a duplicate\n",packet.acknum);
      
      /* if this ACK is for the packet we're timing, need to find next */
      if (buffer_index == earliest_unacked) {
        stoptimer(A);
        timer_active = 0;
        
        /* Find next unacked packet to time */
        find_earliest_unacked();
        
        if (earliest_unacked != -1) {
          starttimer(A, RTT);
          timer_active = 1;
        }
      }
        
      /* if this is the first packet in window, slide window */
      if (buffer_index == windowfirst) {
        /* slide window for all consecutive acknowledged packets */
        while (windowcount > 0 && ack_status[windowfirst] == ACKED) {
          windowfirst = (windowfirst + 1) % WINDOWSIZE;
          windowcount--;
        }
      }
    }
    
    /* If we didn't find the packet or it was already acked, it's a duplicate ACK */
    if (!found && TRACE > 0) {
      printf("----A: duplicate ACK received, do nothing!\n");
    }
  }
  else {
    if (TRACE > 0)
      printf ("----A: corrupted ACK is received, do nothing!\n");
  }
}

/* called when A's timer goes off */
void A_timerinterrupt(void)
{

  if (TRACE > 0)
    printf("----A: time out,resend packets!\n");

  /* find the packet that timed out and retransmit it */
  if (earliest_unacked != -1 && ack_status[earliest_unacked] == UNACKED) {
    
    if (TRACE > 0)
      printf ("---A: resending packet %d\n", buffer[earliest_unacked].seqnum);
    
    /* resend only the timed-out packet */
    tolayer3(A, buffer[earliest_unacked]);
    packets_resent++;
    
    /* restart timer for same packet */
    starttimer(A, RTT);
  } else {
    /* find next earliest unacked */
    find_earliest_unacked();
    
    if (earliest_unacked != -1) {
      starttimer(A, RTT);
    } else {
      timer_active = 0;
    }
  }
}       


/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
  int i;
  
  /* initialise A's window, buffer and sequence number */
  A_nextseqnum = 0;  /* A starts with seq num 0, do not change this */
  windowfirst = 0;
  windowlast = -1;   /* windowlast is where the last packet sent is stored.  
		     new packets are placed in winlast + 1 
		     so initially this is set to -1
		   */
  windowcount = 0;
  
  /* initialize SR specific variables */
  for (i = 0; i < WINDOWSIZE; i++) {
    ack_status[i] = UNACKED;
  }
  timer_active = 0;
  earliest_unacked = -1;
}


/********* Receiver (B)  variables and procedures ************/

static int expectedseqnum; /* the sequence number expected next by the receiver */
static int B_nextseqnum;   /* the sequence number for the next packets sent by B */


/* SR specific variables for receiver */
static struct pkt rcv_buffer[WINDOWSIZE];  /* buffer for out-of-order packets */
static int buffer_status[WINDOWSIZE];      /* track if buffer position is occupied */
static int rcv_base;                       /* base of receive window */

void B_input(struct pkt packet)
{
  struct pkt sendpkt;
  int i;
  int rel_seqnum;
  int buffer_index;
  int in_window = 0;

  /* if not corrupted */
  if (!IsCorrupted(packet)) {
    
    /* Check if packet is within receive window */
    rel_seqnum = packet.seqnum - rcv_base;
    if (rel_seqnum < 0)
      rel_seqnum += SEQSPACE;
      
    if (rel_seqnum < WINDOWSIZE) {
      in_window = 1;
    }
    
    if (in_window) {
      /* Packet is within receive window */
      packets_received++;  /* Count all correctly received packets */
      
      if (TRACE > 0)
        printf("----B: packet %d is correctly received, send ACK!\n", packet.seqnum);
      
      buffer_index = rel_seqnum;
      
      /* Store packet if not already buffered (don't buffer duplicates) */
      if (buffer_status[buffer_index] == 0) {
        rcv_buffer[buffer_index] = packet;
        buffer_status[buffer_index] = 1;
        
        /* If this is the expected packet, deliver it and consecutive buffered packets */
        if (packet.seqnum == expectedseqnum) {
          /* Deliver the expected packet */
          tolayer5(B, packet.payload);
          buffer_status[buffer_index] = 0;
          expectedseqnum = (expectedseqnum + 1) % SEQSPACE;
          
          /* Deliver consecutive buffered packets */
          while (buffer_status[0] == 1) {
            tolayer5(B, rcv_buffer[0].payload);
            
            /* Shift buffer */
            for (i = 0; i < WINDOWSIZE - 1; i++) {
              rcv_buffer[i] = rcv_buffer[i + 1];
              buffer_status[i] = buffer_status[i + 1];
            }
            buffer_status[WINDOWSIZE - 1] = 0;
            
            rcv_base = (rcv_base + 1) % SEQSPACE;
            expectedseqnum = (expectedseqnum + 1) % SEQSPACE;
          }
        }
      }
    }
    else {
      /* packet is outside window */
      if (TRACE > 0)
        printf("----B: packet %d is outside window, ignore\n", packet.seqnum);
    }
    
    /* Send ACK for received packet */
    sendpkt.acknum = packet.seqnum;
  }
  else {
    /* do not send ACK for corrupted packet */
    return;
  }

  /* create ACK packet */
  sendpkt.seqnum = B_nextseqnum;
  B_nextseqnum = (B_nextseqnum + 1) % 2;
    
  /* we don't have any data to send. fill payload with 0's */
  for (i = 0; i < 20; i++) 
    sendpkt.payload[i] = '0';  

  /* compute checksum */
  sendpkt.checksum = ComputeChecksum(sendpkt); 

  /* send out ACK packet */
  tolayer3 (B, sendpkt);
}

void B_init(void)
{
  int i;
  
  expectedseqnum = 0;
  B_nextseqnum = 1;
  
  /* initialize SR specific variables */
  rcv_base = 0;
  for (i = 0; i < WINDOWSIZE; i++) {
    buffer_status[i] = 0;
  }
}

/******************************************************************************
 * The following functions need be completed only for bi-directional messages *
 *****************************************************************************/

/* Note that with simplex transfer from a-to-B, there is no B_output() */
void B_output(struct msg message)  
{
}

/* called when B's timer goes off */
void B_timerinterrupt(void)
{
}