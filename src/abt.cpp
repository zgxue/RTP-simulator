#include <cstdlib>
#include <cstring>
#include <list>
#include <cstdio>
#include "../include/simulator.h"
using namespace std;

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
#define TIMEOUT 15

int MachineStatusA;  //0, 1, 2, 3
int MachineStatusB;  //0, 1
list<msg> bufferedMsgs;
struct pkt sndpktA;
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

void A_sendMsg(struct msg &message){
    sndpktA = make_pkt(MachineStatusA/2, -1, message.data);
    tolayer3(0, sndpktA);
    starttimer(0, TIMEOUT);
    MachineStatusA = (MachineStatusA + 1) % 4;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    if(MachineStatusA%2 == 0 && bufferedMsgs.size()==0){
        A_sendMsg(message);
    }else {
        bufferedMsgs.push_back(message);
    }

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    if(MachineStatusA%2 == 1 && isValidChecksum(packet) && isACK(packet, (MachineStatusA-1)/2)){
        stoptimer(0);
        MachineStatusA = (MachineStatusA + 1) % 4;
        if(!bufferedMsgs.empty()){
            struct msg nextMsg = bufferedMsgs.front();
            A_sendMsg(nextMsg);
            bufferedMsgs.pop_front();
        }
    }

}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    tolayer3(0,sndpktA);
    starttimer(0,TIMEOUT);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    MachineStatusA = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if(isValidChecksum(packet) && MachineStatusB == packet.seqnum){
        tolayer5(1, packet.payload);

        sndpktB = make_pkt(packet.seqnum, packet.seqnum, packet.payload);
        tolayer3(1, sndpktB);
        MachineStatusB = (MachineStatusB+1)%2;
    }else{
        sndpktB = make_pkt(packet.seqnum, (MachineStatusB+1)%2, packet.payload);
        tolayer3(1, sndpktB);
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    MachineStatusB = 0;
}
