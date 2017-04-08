#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "../include/simulator.h"
#include <iostream>
#include <algorithm>

using namespace std;
#define   A    0
#define   B    1
//#define TIMEOUT 15.0*3
#define TIMEOUT_MIN 15.0
#define TIMEOUT_MAX 2000.0
#define TIMEOUT_COFF 1.3 //1.05
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
int totalseqnumA;
struct pkt *sndpktA[1010];
//float sndpktA_sendtime[1010];
int fastRetransCount;
float adaptive_timeout;


int expectedseqnumB;
struct pkt sndpktB;

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

struct pkt *make_pkt(int seq, int ack, char *data){

    struct pkt *retpkt = (struct pkt *)malloc(sizeof(struct pkt));
    retpkt->seqnum = seq;
    retpkt->acknum = ack;
    strncpy(retpkt->payload,data,20);
    retpkt->checksum = genChecksum(seq, ack, retpkt->payload);
    return retpkt;
}

//void restarttimer(int AorB){
//    starttimer(AorB, sndpktA_sendtime[baseA]+TIMEOUT-get_sim_time());
//}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
//    printf(">>>>send Windows Size is %d, baseA: %d, nextseq: %d, total: %d\n"
//            , nextseqnumA-baseA, baseA, nextseqnumA, totalseqnumA);

    //进来先放在数组里
    sndpktA[totalseqnumA] = make_pkt(totalseqnumA, -1, message.data);
    totalseqnumA++;

    if(nextseqnumA < baseA + NwndA && nextseqnumA < totalseqnumA){
        tolayer3(A, *sndpktA[nextseqnumA]);
        if(baseA == nextseqnumA){
//            starttimer(A, TIMEOUT);
            starttimer(A, adaptive_timeout);
        }
        nextseqnumA++;
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    if(isValidChecksum(packet) && packet.acknum >= baseA){
        baseA = packet.acknum + 1;
        adaptive_timeout = TIMEOUT_MIN;
        if(baseA == nextseqnumA){
            stoptimer(A);
            if(nextseqnumA < totalseqnumA){
                starttimer(A, adaptive_timeout);
                while (nextseqnumA < baseA + NwndA && nextseqnumA < totalseqnumA){
                    tolayer3(A, *sndpktA[nextseqnumA]);
                    nextseqnumA++;
                }
            }
        } else{
//            stoptimer(A);
            starttimer(A, adaptive_timeout);
            while (nextseqnumA < baseA + NwndA && nextseqnumA < totalseqnumA){
                tolayer3(A, *sndpktA[nextseqnumA]);
                nextseqnumA++;
            }
        }
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    adaptive_timeout *= TIMEOUT_COFF;
    starttimer(A, min((float)adaptive_timeout, (float)TIMEOUT_MAX));
    printf("&&&&Retransmit all window packets: %d\n", nextseqnumA - baseA);
    for (int i = baseA; i < nextseqnumA; ++i) {
        tolayer3(A, *sndpktA[i]);
    }
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    baseA = 1;
    nextseqnumA = 1;
    totalseqnumA = 1;
    NwndA = getwinsize();
    adaptive_timeout = TIMEOUT_MIN;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if(isValidChecksum(packet) && packet.seqnum == expectedseqnumB){
        tolayer5(B, packet.payload);
        printf("***** I delived %s\n", packet.payload);
        sndpktB = *make_pkt(expectedseqnumB, packet.seqnum, packet.payload);
        tolayer3(B, sndpktB);
        expectedseqnumB++;
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



