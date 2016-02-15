#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

#include <sys/shm.h>

#include <sys/select.h>
#include <termios.h>

#include "communicationStructures.h"
#include "kbhit.h"


int connectToServerAndGetCommunicationKey();
void sendBuildMsg(int type, int amount);
void printData(Data dataMsg);

int msgKey;

enum mode{MODE_VIEW,MODE_BUILD,MODE_ATTACK};
//int *mode;

int main() {
    char stopGame = 0;
    int mode;
//    int shmId = shmget(getppid(), sizeof(int), IPC_CREAT|0640);
//    if (shmId == -1){
//        perror("Shared memory creating");
//        exit(0);
//    }
//    mode = (int*)shmat(shmId, NULL, 0);
//
//    *mode = MODE_VIEW;

    int communicationKey = connectToServerAndGetCommunicationKey();
    msgKey = msgget(communicationKey, 0640);
    if (msgKey == -1) {
        perror("Opening message queue");
        exit(0);
    }

    while (stopGame == 0){
        Data dataMsg;
//        int error;
        char type = 0;
        int amount = 0;
        

        if (  kbhit() ){
            char ch = getchar();
                if ((char)ch >= 'a' && (char)ch <= 'z'){
                    if( (char)ch == 'b'){
                        mode = MODE_BUILD;
                    }
                }
        }

        switch(mode){
            case MODE_VIEW:
                msgrcv(msgKey, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR|IPC_NOWAIT);

                printData(dataMsg);

                if (dataMsg.end != 0) break;
                break;
            case MODE_BUILD:
                while(type != 'l' && type !='h' && type !='c' && type !='w'){
                    msgrcv(msgKey, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR|IPC_NOWAIT);
                    printData(dataMsg);
                    printf("Enter type - (l)ight, (h)eavy, (c)avalry or (w)orkers:\n");
                    if( kbhit() ){
                        type = getchar();
                    }
                }

                while(amount <= 0){
                    msgrcv(msgKey, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR|IPC_NOWAIT);
                    printData(dataMsg);
                    printf("Enter amount (0-9):\n");
                    if( kbhit() ){
                        char temp = getchar();
                        if (temp>='0' && temp <='9')
                            amount = temp-'0';
//                        char temp = getchar();
//                        if(temp == 13 && amount > 0) break;
//                        if (temp>='1' && temp <='9'){
//                            amount += (int)temp-'0' * pow(10,count++);
//                        }
                    }
                }

                switch(type){
                    case 'l':
                        sendBuildMsg(LIGHT,amount);
                        break;
                    case 'h':
                        sendBuildMsg(HEAVY,amount);
                        break;
                    case 'c':
                        sendBuildMsg(CAVALRY,amount);
                        break;
                    case 'w':
                         sendBuildMsg(WORKER,amount);
                        break;
                }
                mode = MODE_VIEW;
                break;
        }
    }
    return 0;
}

void printData(Data dataMsg){
    printf("\033c");
    usleep(100*1000); // to prevent lagging screen
    printf("Points : %d\nResources : %d\n\nLight : %d\nHeavy : %d\nCavalry : %d\nWorkers : %d\n\n"
            ,dataMsg.points, dataMsg.resources, dataMsg.light, dataMsg.heavy, dataMsg.cavalry, dataMsg.workers);
}

int connectToServerAndGetCommunicationKey(){
    int error;

    int msgid = msgget(connectionKey,IPC_CREAT | 0640);
    if (msgid == -1){
        perror("Opening message queue");
        exit(0);
    }

    Init initMsg;
    initMsg.mtype=1;
    initMsg.nextMsg=-1;

    error = msgsnd(msgid,&initMsg,sizeof(initMsg.nextMsg),0);
    if(error == -1){
        perror("Sending");
        exit(0);
    }

    error = (int) msgrcv(msgid, &initMsg, sizeof(initMsg.nextMsg), 2, 0);
    if(error == -1){
        perror("Receiveing");
        exit(0);
    }

    if(initMsg.nextMsg == 0){
        printf("Server is busy. Try again later.\n");
        exit(0);
    }

    return initMsg.nextMsg;
}

void sendBuildMsg(int type, int amount){
    Build buildMsg;
    buildMsg.mtype = TYPE_BUILD;
    switch(type){
        case LIGHT:
            buildMsg.light = amount;
            buildMsg.heavy = 0;
            buildMsg.cavalry = 0;
            buildMsg.workers = 0;
            break;
        case HEAVY:
            buildMsg.light = 0;
            buildMsg.heavy = amount;
            buildMsg.cavalry = 0;
            buildMsg.workers = 0;
            break;
        case CAVALRY:
            buildMsg.light = 0;
            buildMsg.heavy = 0;
            buildMsg.cavalry = amount;
            buildMsg.workers = 0;
            break;
        case WORKER:
            buildMsg.light = 0;
            buildMsg.heavy = 0;
            buildMsg.cavalry = 0;
            buildMsg.workers = amount;
            break;
        default:
            printf("Wrong entiry typ.\n");
            return;
    }
    int error = (int) msgsnd(msgKey, &buildMsg, sizeof(buildMsg)- sizeof(buildMsg.mtype), IPC_NOWAIT);
    if(error == -1){
        perror("Sending");
        exit(0);
    }
    return;
}
