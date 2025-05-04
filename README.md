rule 1: make sure you understand the question clearly

the main task is modify gbn to sr -> duplicate them first

systems components:

Application Layer (Layer 5) on Host A
        ↓
Transport Layer (Layer 4) - code this
        ↓
Network Layer (Layer 3) - Emulator
        ↓
[Network - can corrupt/lose packets]
        ↓
Network Layer (Layer 3) - Emulator
        ↓
Transport Layer (Layer 4) - code this
        ↓
Application Layer (Layer 5) on Host B


Feature                 Go-Back-N               Selective Repeat 
Out-of-order handling   Discards                Buffers
ACK type                Cumulative              Individual
Buffer requirement      None                    Large buffer
Delivery order          Always sequential       Maintained through buffering

Sender(A) functions:

A_output(): called when app has data to send

A_input(): called when ACk arrives from Receiver
A_timerinterrupt(): called when timer expires
A_intit(): initalize sender state

Receiver(B) Functions:
B_input(): called when packet arrive from sender
B_init(): initialize receiver state



Window size should be less than or equal to half the sequence number in SR protocol. This is to avoid packets being recognized incorrectly. 


SR requires a sequence space >= window size + 1 to distinguish new vs old packets. I need to set some constants here

**RECEIVER

#define WINDOWSIZE 6
#define SEQSPACE   (WINDOWSIZE + 1)
#define RTT        16.0    // ensure timeouts match simulator expectations

#define NOTINUSE  -1
#define UNACKED    0
#define ACKED      1

keep the ComputeCheckSum() and Iscorrupted() unchange.
cuz both GBN and SR need reliable corruption detection

static int ack_status[WINDOWSIZE];     // UNACKED or ACKED
static int timer_active;   // 1 if timer is running
static int timer_seqnum[WINDOWSIZE]; // sequence number associated with each timer

Unlike GBN's single sliding window timer, SR must mange multiple independent timers. 

these arrays let me start/stop timers and lookip which packet timed out

A_init(): Clears any old state so SR starts clean.

how to sending packet?
A_output()
first check window count < WINDOWSIZE
build packet (set seqnum, copy payload, compute checksum)

buffer in circular buffer[]

mark ack_status[slot] = UNACKED
send to layer 3

A single timer watches the oldest unACKed packet; new ones wait their turn.

A_input():


I ignored corrupted packets.

If the ACK matched an unacknowledged packet, I marked it as ACKED.

If it acknowledged the packet under timeout watch, I stopped the timer and scanned for the next oldest unacknowledged one.

I also advanced the window front past any now-ACKed packets.

A_timerinterrupt()
If the timer expired:

I scanned the window from the front for the first unacknowledged packet.

Once found, I retransmitted it, restarted the timer for it, and updated the timer’s tracking variable.

The idea was always to resend the oldest needed packet — nothing more — keeping bandwidth usage low.

B_input()

I tracked expected sequence numbers and stored out-of-order arrivals.

I acknowledged every valid packet.

If the packet was in order, I delivered it and any consecutively buffered ones that followed.

This allowed out-of-order ACKs and ensured the application received only properly ordered data, just as SR promises.