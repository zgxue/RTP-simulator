#include <cstdlib>
#include <cstring>
#include "../include/simulator.h"
#include <list>

using namespace std;
#define   A    0
#define   B    1
#define TIMEOUT 15.0
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

int baseA;
int nextseqnumA;
int NwndA;
list<msg> bufferedMsgs;
struct pkt sndpktA[1010];

int expectedseqnumB;
struct pkt sndpktB;

int genChecksum(int seqnum, int acknum, char *data);
bool isValidChecksum(struct pkt &rcvpkt);
bool isACK(struct pkt &rcvpkt, int wantedACK);
struct pkt make_pkt(int seq, int ack, char *data);
int getacknum(struct pkt &packet);
void A_sendMsg(struct msg &message);
bool hasseqnum(struct pkt &rcvpkt, int expectedseqnum);

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    if(bufferedMsgs.empty() && nextseqnumA < baseA + NwndA){
        A_sendMsg(message);
        if(baseA == nextseqnumA){
            starttimer(A, TIMEOUT);
        }
        ++nextseqnumA;
    }else{
        bufferedMsgs.push_back(message);
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    if(isValidChecksum(packet) ){
        baseA = getacknum(packet) + 1;
        if(baseA == nextseqnumA)
            stoptimer(A);
        else
            starttimer(A, TIMEOUT);

//        if(!bufferedMsgs.empty()){
//            A_sendMsg(bufferedMsgs.front());
//            bufferedMsgs.pop_front();
//            ++nextseqnumA;
//        }
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    starttimer(A, TIMEOUT);
    for (int i = baseA; i < nextseqnumA; ++i) {
        tolayer3(A, sndpktA[i]);
    }
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    baseA = 1;
    nextseqnumA = 1;
    NwndA = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if(isValidChecksum(packet) && hasseqnum(packet, expectedseqnumB)){
        tolayer5(B, packet.payload);
        sndpktB = make_pkt(expectedseqnumB, packet.seqnum, packet.payload);
        tolayer3(B, sndpktB);
        ++expectedseqnumB;
    }else{
        tolayer3(B, sndpktB);
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    expectedseqnumB = 1;
}


int genChecksum(int seqnum, int acknum, char *data){
    int retChecksum = 0;
    retChecksum += seqnum;
    retChecksum += acknum;
    for (int i = 0; i < 20 && data[i] != '\0'; ++i) {
        retChecksum += (int)data[i];
    }
    return retChecksum;
}

bool isValidChecksum(struct pkt &rcvpkt){
    return rcvpkt.checksum == genChecksum(rcvpkt.seqnum, rcvpkt.acknum, rcvpkt.payload);
}

bool isACK(struct pkt &rcvpkt, int wantedACK){
    return rcvpkt.acknum == wantedACK;
}

struct pkt make_pkt(int seq, int ack, char *data){

    struct pkt *retpkt = (struct pkt *)malloc(sizeof(struct pkt));
    retpkt->seqnum = seq;
    retpkt->acknum = ack;
    strncpy(retpkt->payload,data,20);
    retpkt->checksum = genChecksum(seq, ack, retpkt->payload);
    return *retpkt;
}
int getacknum(struct pkt &packet){
    return packet.acknum;
}
void A_sendMsg(struct msg &message){
    sndpktA[nextseqnumA] = make_pkt(nextseqnumA, -1, message.data);
    tolayer3(A, sndpktA[nextseqnumA]);
}

bool hasseqnum(struct pkt &rcvpkt, int seqnum){
    return rcvpkt.seqnum == seqnum;
}
