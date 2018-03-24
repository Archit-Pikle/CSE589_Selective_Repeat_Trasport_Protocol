#include<stdlib.h>
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

#define buffersize 2000
#define TIMEOUT 40 

int compute_checksum(struct pkt);

int expectedseqnum;
int recv_base;
struct msg recv_buffer[buffersize];
int recv_index;

int window;
int base;
int bufferindex;
int nextseqnum;
struct pkt packet_buffer[buffersize];

int acksent[buffersize];
int pktreceived[buffersize];

struct node
{
        int seqnum;
        float time;
        struct node* next;
};

struct timers
{
        struct node *head;
        struct node *tail;
        int size;
};

struct timers timerlist;

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{       //storepacketinbuffer
        struct pkt sending_packet;
        for(int i =0;i<20;i++)
                sending_packet.payload[i] = message.data[i];
        sending_packet.seqnum = bufferindex;
        sending_packet.checksum = compute_checksum(sending_packet);
        packet_buffer[bufferindex] = sending_packet;
        bufferindex ++;

        if(nextseqnum < (base + window))
        {
                struct node *newnode;
                newnode = malloc(sizeof(struct node));
                newnode->seqnum = nextseqnum;
                newnode->time = get_sim_time();
                if (timerlist.head == NULL)
                {
                        timerlist.head = newnode;
                        timerlist.tail = newnode;
                        timerlist.head->next = NULL;
                }
                else
                {
                        timerlist.tail->next = newnode;
                        timerlist.tail = timerlist.tail->next;
                        timerlist.tail->next = NULL;
                }

                timerlist.size ++;
                tolayer3(0,packet_buffer[nextseqnum]);
                if(base == nextseqnum)
                        starttimer(0,TIMEOUT);

                nextseqnum ++;
        }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
        if(packet.checksum == compute_checksum(packet))
        {
                if(packet.acknum >= base && packet.acknum <= base + window -1)
                {
                        pktreceived[packet.acknum] = 1;
                        struct node *current, *trav_node;

                        current = malloc(sizeof(struct node));
                        trav_node = malloc(sizeof(struct node));
                        trav_node = timerlist.head;
                        current = trav_node->next;

                        if(timerlist.head->seqnum == packet.acknum)
                        {
                                if(current != NULL)
                                {
                                        float newtimeout = TIMEOUT - get_sim_time() + current->time;
                                        stoptimer(0);
                                        starttimer(0,newtimeout);
                                        timerlist.head = current;
                                }
                                else
                                {
                                        stoptimer(0);
                                        timerlist.head = current;
                                }
                        }
                        else
                        {
                                while(current != NULL)
                                {
                                        if(timerlist.tail->seqnum == current->seqnum && packet.acknum == current->seqnum)
                                        {
                                                trav_node->next = NULL;
                                                timerlist.tail = trav_node;
                                                break;
                                        }
                                        else if(packet.acknum == current->seqnum)
                                        {
                                                trav_node->next = current->next;
                                                break;
                                        }
                                        current = current->next;
                                        trav_node = trav_node->next;
                                }
                        }
                        int test = -1;
                        for(int i = base;i <= base+window-1;i++)
                        {
                                if(pktreceived[i] == 1)
                                {
                                        test = i;
                                }
                                else
                                        break;
                        }
                        if(test != -1)
                        {
                                base = test + 1;
                                while(nextseqnum<base+window && nextseqnum < bufferindex)
                                {
                                        struct node *newnode;
                                        newnode = malloc(sizeof(struct node));
                                        newnode->seqnum = nextseqnum;
                                        newnode->time = get_sim_time();
                                        if (timerlist.head == NULL)
                                        {
                                                timerlist.head = newnode;
                                                timerlist.tail = newnode;
                                                timerlist.head->next = NULL;
                                        }
                                        else
                                        {
                                                timerlist.tail->next = newnode;
                                                timerlist.tail = timerlist.tail->next;
                                                timerlist.tail->next = NULL;
                                        }

                                        timerlist.size++;
                                        tolayer3(0,packet_buffer[nextseqnum]);
                                        if(base == nextseqnum)
                                                starttimer(0,TIMEOUT);
                                        nextseqnum++;
                                }
                        }
                }
        }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
        int timeoutsequence = timerlist.head->seqnum;
        if(timerlist.head->next == NULL)
                starttimer(0,TIMEOUT);
        else
        {
                float newtimeout = TIMEOUT - get_sim_time() + timerlist.head->next->time;
                timerlist.head = timerlist.head->next;
                starttimer(0,newtimeout);
        }
        timerlist.size = timerlist.size - 1;

        //resend packet

        struct node *newnode;
        newnode = malloc(sizeof(struct node));
        newnode->seqnum = timeoutsequence;
        newnode->time = get_sim_time();
        if (timerlist.head == NULL)
        {
                timerlist.head = newnode;
                timerlist.tail = newnode;
                timerlist.head->next = NULL;
        }
        else
        {
                timerlist.tail->next = newnode;
                timerlist.tail = timerlist.tail->next;
                timerlist.tail->next = NULL;
        }

        timerlist.size++;
        tolayer3(0,packet_buffer[timeoutsequence]);
        if(base == nextseqnum)
                starttimer(0,TIMEOUT);

}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
        window = getwinsize();
        base = 0;
        bufferindex = 0;
        nextseqnum = 0;
        timerlist.head = NULL;
        timerlist.tail = NULL;
        timerlist.size = 0;

        for(int i =0;i<buffersize;i++)
        {
                packet_buffer[i].acknum = -1;
                pktreceived[i] = 0;
        }
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
        if(packet.checksum == compute_checksum(packet))
        {
                if(packet.seqnum >= (recv_base) && packet.seqnum <= recv_base + window - 1)
                {
                        struct pkt ack;
                        ack.acknum = packet.seqnum;
                        ack.checksum = compute_checksum(ack);
                        tolayer3(1,ack);
                        if(acksent[packet.seqnum] == 0)
                        {
                                struct msg message;
                                for(int i=0;i<20;i++)
                                        message.data[i] = packet.payload[i];
                                recv_buffer[packet.seqnum] = message;
                                acksent[packet.seqnum] = 1;
                        }

                        int test = -1;
                        for(int i = recv_base;i<recv_base + window -1;i++)
                        {
                                if(acksent[i] == 1)
                                        test =  i;
                                else
                                        break;
                        }
                        if(test != -1)
                        {

                                int start = recv_base;
                                while(start <= test)
                                {
                                        if(acksent[start]==1)
                                        {
                                                tolayer5(1,recv_buffer[start].data);
                                                recv_base++;

                                        }
                                        start++;
                                }

                        }
                }

                else if (packet.seqnum >= (recv_base - window) && packet.seqnum <= (recv_base - 1))
                {
                        struct pkt ack;
                        ack.acknum = packet.seqnum;
                        ack.checksum = compute_checksum(ack);
                        tolayer3(1,ack);
                }
        }
}


/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */

void B_init()
{
        expectedseqnum = 0;
        recv_base = 0;

        recv_index = 0;
        for(int i=0;i<buffersize;i++)
                acksent[i] = 0;

}
int compute_checksum(package)
struct pkt package;
{

        int sum = 0;
        sum += package.seqnum + package.acknum;
        for(int i = 0; i<20;i++)
                sum += (int) package.payload[i];
        sum = ~sum;
        return(sum);
}


