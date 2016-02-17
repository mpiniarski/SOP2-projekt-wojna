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

#include <signal.h>


#include "communicationStructures.h"
#include "kbhit.h"


int connectToServerAndGetCommunicationKey();
void sendAttackMsg(int lightAmount, int heavyAmount, int cavalryAmount);
void prepareAndSendBuildMsg();
void prepareAndSendAttackMsg();
void sendBuildMsg(int type, int amount);
void printData(Data dataMsg);
void handleEvents();

char* stopGame;

Data dataMsg;
int msgKey;
int mode;
enum mode{MODE_VIEW,MODE_BUILD,MODE_ATTACK};


int main() {
    mode = MODE_VIEW;

    int shmId = shmget(getppid(), sizeof(char), IPC_CREAT|0640);
    if (shmId == -1){
        perror("Shared memory creating");
        exit(0);
    }
    stopGame = (char*)shmat(shmId, NULL, 0);
    *stopGame = 0;

    int communicationKey = connectToServerAndGetCommunicationKey();
    msgKey = msgget(communicationKey, 0640);
    if (msgKey == -1) {
        perror("Opening message queue");
        exit(0);
    }

    //Waiting for game to start
    int error = msgrcv(msgKey, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR);
    if(error == -1){
        perror("Waiting for game to start");
        exit(0);
    }

    printData(dataMsg);

    // HEARTBEAT 
    if ( fork() == 0){
        int error;
        int serverLates = 0;

        while(1){
            sleep(2);
            Alive aliveMsg;
            aliveMsg.mtype = TYPE_ALIVE_CLIENT;
            error = msgsnd(msgKey,&aliveMsg,0,IPC_NOWAIT);
            if(error == -1) perror("Sending aliveMsg");
            error = msgrcv(msgKey,&aliveMsg,0,TYPE_ALIVE_SERVER,IPC_NOWAIT);
            if(error == -1){
                if (++serverLates == 3){
                    printf("Server is DEAD\n");
                    *stopGame = 1;
                    break;
                }
            }
        }
        exit(0);
    }

    while (*stopGame == 0){
        handleEvents();
        switch(mode){
            case MODE_VIEW:
                msgrcv(msgKey, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR|IPC_NOWAIT);
                printData(dataMsg);
                break;
            case MODE_BUILD:
                prepareAndSendBuildMsg();
                mode = MODE_VIEW;
                break;
            case MODE_ATTACK:
                prepareAndSendAttackMsg();
                mode = MODE_VIEW;
                break;
        }
        if (dataMsg.end != 0) *stopGame = 1;
    }
    kill(0,SIGKILL);
    return 0;
}

int connectToServerAndGetCommunicationKey(){
    int error;
    int msgid = msgget(connectionKey, 0640);
    if (msgid == -1){
        printf("No server available. Try again later...\n");
        exit(0);
    }

    Init initMsg;
    error = (int) msgrcv(msgid, &initMsg, sizeof(initMsg.nextMsg), 1, IPC_NOWAIT);
    if(error == -1){
        printf("No server available. Try again later...\n");
        exit(0);
    }
    int connectionKey = initMsg.nextMsg;
    initMsg.mtype=2;
    error = msgsnd(msgid,&initMsg,sizeof(initMsg.nextMsg),0);
    if(error == -1){
        perror("Sending");
        exit(0);
    }

    return connectionKey;
}

void printData(Data dataMsg){
    printf("\033c");
    usleep(100*1000); // to prevent lagging screen
    printf("Points : %d\nResources : %d\n\nLight : %d\nHeavy : %d\nCavalry : %d\nWorkers : %d\n\n"
            ,dataMsg.points, dataMsg.resources, dataMsg.light, dataMsg.heavy, dataMsg.cavalry, dataMsg.workers);
}

void handleEvents(){
    if (  kbhit() ){
        char ch = getchar();
            if ((char)ch >= 'a' && (char)ch <= 'z'){
                if( (char)ch == 'b'){
                    mode = MODE_BUILD;
                }
                else if( (char)ch == 'a'){
                    mode = MODE_ATTACK;
                }
                else if( (char)ch == 'q'){
                    *stopGame = 1;
                }
            }
    }
}

void prepareAndSendBuildMsg(){
    char type = 0;
    int amount = -1;
    while(type != 'l' && type !='h' && type !='c' && type !='w'){
        msgrcv(msgKey, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR|IPC_NOWAIT);
        printData(dataMsg);
        printf("Enter type - (l)ight, (h)eavy, (c)avalry or (w)orkers:\n");
        if( kbhit() ){
            type = getchar();
        }
    }

    while(amount < 0){
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
            printf("Wrong entiry type.\n");
            return;
    }
    int error = (int) msgsnd(msgKey, &buildMsg, sizeof(buildMsg)- sizeof(buildMsg.mtype), IPC_NOWAIT);
    if(error == -1){
        perror("Sending");
        exit(0);
    }
    return;
}

void sendAttackMsg(int lightAmount, int heavyAmount, int cavalryAmount){
    Attack attackMsg;
    attackMsg.mtype = TYPE_ATTACK;
    attackMsg.light = lightAmount;
    attackMsg.heavy = heavyAmount;
    attackMsg.cavalry = cavalryAmount;
    int error = (int) msgsnd(msgKey, &attackMsg, sizeof(attackMsg)- sizeof(attackMsg.mtype), IPC_NOWAIT);
    if(error == -1){
        perror("Sending");
        exit(0);
    }
    return;
}

void prepareAndSendAttackMsg(){
    int lightAmount = -1, heavyAmount = -1, cavalryAmount = -1;

    while(lightAmount < 0){
        msgrcv(msgKey, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR|IPC_NOWAIT);
        printData(dataMsg);
        printf("Enter light warriors amount (0-9):\n");
        if( kbhit() ){
            char temp = getchar();
            if (temp >='0' && temp <='9')
                lightAmount = temp-'0';
        }
    }
    while(heavyAmount < 0){
        msgrcv(msgKey, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR|IPC_NOWAIT);
        printData(dataMsg);
        printf("Enter heavy warriors amount (0-9):\n");
        if( kbhit() ){
            char temp = getchar();
            if (temp >='0' && temp <='9')
                heavyAmount = temp-'0';
        }
    }
    while(cavalryAmount < 0){
        msgrcv(msgKey, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR|IPC_NOWAIT);
        printData(dataMsg);
        printf("Enter cavalry warriors amount (0-9):\n");
        if( kbhit() ){
            char temp = getchar();
            if (temp >='0' && temp <='9')
                cavalryAmount = temp-'0';
        }
    }
    sendAttackMsg(lightAmount,heavyAmount,cavalryAmount);
}
