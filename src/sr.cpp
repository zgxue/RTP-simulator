#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include "../include/simulator.h"
#include <list>
#include <algorithm>


using namespace std;
#define   A    0
#define   B    1
#define TIMEOUT_MIN 15.0
#define TIMEOUT_MAX 200.0
static float timeout_coff = 2.0;

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


class Timer{
public:
    int position;
    float start_time;
    float timeout;
    Timer(int p, float st, float to){
        position = p; start_time = st; timeout = to;
    }
};

bool timer_sort(Timer &left, Timer &right){
    return (left.start_time+left.timeout < right.start_time+right.timeout);
}

list<Timer> timers;  //store the machine time when inserted
struct pkt *sndpktA[1100];
bool isackReceived[1100];  //store the position that has been received by receiver
//float firstSendtime[1100];
//float lastSendtime[1100];
//float EstimatedRTT;
//float alpha;
//float DevRTT;
//float beta;
//float TimeoutInterval;

int baseB;
int NwndB;
struct pkt rcvpktB[1100];
bool ispktReceived[1100];


////////////////////////////////////////////////////
////////////
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

void restarttimer(int AorB){
    starttimer(AorB, timers.front().start_time+ timers.front().timeout -get_sim_time());
}
bool eraseTimeoutByPos(int position){
    for(list<Timer>::iterator iter = timers.begin(); iter != timers.end(); ++iter){
        if((*iter).position == position){
            timers.erase(iter);
            return true;
        }
    }
    return false;
}

/*Timer findTimerByPos(int position){
    for(list<Timer>::iterator iter = timers.begin(); iter != timers.end(); ++iter){
        if((*iter).position == position){
            return *iter;
        }
    }
    return NULL;
}*/

//////////////////////////////////////////////////////
/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    printf(">>>>send Windows Size is %d, baseA: %d, nextseq: %d, total: %d\n"
            , nextseqnumA-baseA, baseA, nextseqnumA, totalseqnumA);

    //进来先放在数组里
    sndpktA[totalseqnumA] = make_pkt(totalseqnumA, -1, message.data);
    totalseqnumA++;

    //检查如果在窗口内，就立即发送。并set timeout 记录。
    if(nextseqnumA < baseA + NwndA && nextseqnumA < totalseqnumA){
        //发到下层
        tolayer3(A, *sndpktA[nextseqnumA]);
        //加入这个 seqnum 到 timers 里

        Timer *t = new Timer(nextseqnumA, get_sim_time(), TIMEOUT_MIN);
        timers.push_back(*t);
        timers.sort(timer_sort);
        //找到timers 里面最早的时间点，倒计时
        restarttimer(A);
        nextseqnumA++;
    }

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    if(isValidChecksum(packet) && packet.acknum >= baseA && packet.acknum < nextseqnumA){
        isackReceived[packet.acknum] = true;
//        //计算对应的 RTT 并更新 timeoutInteval
//        float SampleRTT = get_sim_time() - firstSendtime[packet.acknum];
//        EstimatedRTT = (1-alpha)*EstimatedRTT + alpha*SampleRTT;
//
//        DevRTT = (1-beta)*DevRTT
//                + beta * (SampleRTT - EstimatedRTT)?(SampleRTT - EstimatedRTT):(EstimatedRTT-SampleRTT);
//        TimeoutInterval = EstimatedRTT + 4*DevRTT;
//        printf("@@@@@@timeoutInterval: %f, sampleRTT: %d, estimatedRTT: %f, devRTT: %f\n", TimeoutInterval,SampleRTT, EstimatedRTT, DevRTT);

        while (isackReceived[baseA]){
            //删除对应计时器
            eraseTimeoutByPos(baseA);
            if(timers.empty()){
                stoptimer(A);
            }else{
                restarttimer(A);
            }
            baseA++;

            //如果有新的数据落入窗口中了
            if(nextseqnumA < baseA + NwndA && nextseqnumA < totalseqnumA){
                tolayer3(A, *sndpktA[nextseqnumA]);

                Timer *t = new Timer(nextseqnumA, get_sim_time(), TIMEOUT_MIN);
                timers.push_back(*t);
                timers.sort(timer_sort);
//                timers.push_back(make_pair(nextseqnumA, get_sim_time()));

                restarttimer(A);
                nextseqnumA++;
            }
        }
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    //重新设新的countdown timer
    int whotimeout = timers.front().position;

    Timer *t = new Timer(timers.front().position, get_sim_time(), min((float)(timers.front().timeout*timeout_coff), (float)TIMEOUT_MAX));
    timers.push_back(*t);
    timers.pop_front();
    timers.sort(timer_sort);
    restarttimer(A);

    //重发
    tolayer3(A, *sndpktA[whotimeout]);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    baseA = 1;
    nextseqnumA = 1;
    NwndA = getwinsize();
    totalseqnumA = 1;

//    EstimatedRTT = 10;
//    alpha = 0.125;
//    DevRTT = 0;
//    beta = 0.25;
//    TimeoutInterval = EstimatedRTT + 4*DevRTT;

    for (int i = 0; i < 1100; ++i) {
        isackReceived[i] = false;
    }
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if(isValidChecksum(packet)){
//        printf(">>>Now baseB is %d, packreceived sequ is %d\n", baseB, packet.seqnum);
        if(packet.seqnum >= baseB && packet.seqnum <= baseB + NwndB - 1){
            if(!ispktReceived[packet.seqnum]){
                ispktReceived[packet.seqnum] = true;
                rcvpktB[packet.seqnum] = packet;
            }
            packet.acknum = packet.seqnum;
            tolayer3(B, packet);
//            tolayer3(B, *make_pkt(packet.seqnum, packet.seqnum, packet.payload));

            while (ispktReceived[baseB]){
                tolayer5(B, (rcvpktB[baseB]).payload);
                baseB++;
            }
        }else if (packet.seqnum >= baseB - NwndB && packet.seqnum <= baseB - 1){
            tolayer3(B, *make_pkt(packet.seqnum, packet.seqnum, packet.payload));
        }
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    baseB = 1;
    NwndB = getwinsize();
    for (int i = 0; i < 1100; ++i) {
        ispktReceived[i] = false;
    }
}








