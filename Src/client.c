#include <signal.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "communicationStructures.h"
#include "kbhit.h"
#include "myipc.h"

void turnCursorOff();
void clear();
void overwriteSignals();
void handleSignal();
void connectToServer();
void printData(Data dataMsg);
void heartbeatPeriodially();
void handleEvents();
void prepareAndSendBuildMsg();
void sendBuildMsg(int type, int amount);
void prepareAndSendAttackMsg();
void sendAttackMsg(int lightAmount, int heavyAmount, int cavalryAmount);

char* stopGame;
int mode;
enum mode{MODE_VIEW,MODE_BUILD,MODE_ATTACK};
int msgId;
Data dataMsg;

char lastMessage[120]; // to be compatibile with others

// MAIN
int main() {
    //turnCursorOff();
    clear();
    overwriteSignals();

    int shmId = shmgetSave(getppid(), sizeof(char));
    stopGame = (char*)shmat(shmId, NULL, 0);
    *stopGame = 0;
    mode = MODE_VIEW;
    connectToServer();

    //Waiting for game to start
    int error = msgrcv(msgId, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR);
    if(error == -1){ printf("Server process is dead. Game is over.\n"); exitSave(0); }
    printData(dataMsg);

    // HEARTBEAT 
    int err;
    if ( (err=fork()) == 0){
        heartbeatPeriodially();
        exitSave(0);
    } else if(err == -1){
        perror("Fork"); exitSave(0);
    }

    while (*stopGame == 0){
        handleEvents();
        switch(mode){
            case MODE_VIEW:
                msgrcv(msgId, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR|IPC_NOWAIT);
                printData(dataMsg);
                printf("\n       (1) - Build entities"
                       "\n       (2) - Attack enemy"
                       "\n"
                       "\n       (q) - Quit game (and surrender)\n");
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
        if (dataMsg.end != 0) {
            msgctl(msgId,IPC_RMID,0); // to keep system clear
            *stopGame = 1;
        }
    }
    exitSave(0);
}

void clear(){
    printf("\033c");
}

void turnCursorOff(){
    printf("\e[?25l");
}

void overwriteSignals(){
    signal(SIGINT,handleSignal);
    signal(SIGQUIT,handleSignal);
    signal(SIGTERM,handleSignal);
}

void handleSignal(){
    exitSave(0);
}

void connectToServer(){
    int error;
    int msgid = msgget(connectionKey, 0640);
    if (msgid == -1){
        printf("No server available. Try again later...\n");
        exitSave(0);
    }

    Init initMsg;
    error = (int) msgrcv(msgid, &initMsg, sizeof(initMsg.nextMsg), 1, IPC_NOWAIT);
    if(error == -1){
        printf("No server available. Try again later...\n");
        exitSave(0);
    }
    int connectionKey = initMsg.nextMsg;
    initMsg.mtype=2;
    error = msgsnd(msgid,&initMsg,sizeof(initMsg.nextMsg),0);
    if(error == -1){
        perror("Sending");
        exitSave(0);
    }

    msgId = msgget(connectionKey, 0640);
    if (msgId == -1) {
        perror("Opening message queue");
        exitSave(0);
    }
}

void printData(Data dataMsg){
    clear();
    usleep(150*1000); // to prevent lagging screen
    printf(
            "                                            \n"
            "                       ~~~~~~~~~~~~~~       \n"
            "                        YOUR KINGDOM        \n"
            "                       ~~~~~~~~~~~~~~       \n"
            "                                            \n"
            "                        Points : %d         \n"
            "                       Resources : %d       \n"
            "                                            \n"
            "                 Lightweight warriors : %d  \n"
            "                 Heavyweight warriors : %d  \n"
            "                 Cavalry              : %d  \n"
            "                 Workers              : %d  \n"
            "                                            \n"
            "                                            \n"
            ,dataMsg.points, dataMsg.resources, dataMsg.light, dataMsg.heavy, dataMsg.cavalry, dataMsg.workers);

    // Center info
    if (strlen(dataMsg.info) != 0)
        strcpy(lastMessage,dataMsg.info);
    int infoSize = strlen(lastMessage);
    int i;
    for(i=0; i< 30 - (infoSize+4)/2; i++) printf(" ");
    printf("~ %s ~\n",lastMessage);//dataMsg.info);
}

void heartbeatPeriodially(){
    int error;
    int serverLates = 0;
    while(1){
        sleep(2);
        Alive aliveMsg;
        aliveMsg.mtype = TYPE_ALIVE_CLIENT;
        error = msgsnd(msgId,&aliveMsg,0,IPC_NOWAIT);
        if(error == -1) perror("Sending aliveMsg");
        error = msgrcv(msgId,&aliveMsg,0,TYPE_ALIVE_SERVER,IPC_NOWAIT);
        if(error == -1){
            if (++serverLates == 3){
                printf("Server process is dead. Game is over.\n");
                *stopGame = 1;
                break;
            }
        }
    }
}


void handleEvents(){
    if (  kbhit() ){
        char ch = getchar();
            if( (char)ch == '1'){
                mode = MODE_BUILD;
            }
            else if( (char)ch == '2'){
                mode = MODE_ATTACK;
            }
            else if( (char)ch == 'q'){
                *stopGame = 1;
                //exitSave(0);
            }
    }
}

void prepareAndSendBuildMsg(){
    char type = 0;
    int amount = -1;
    while(type != '1' && type !='2' && type !='3' && type !='4' && type != 'q'){
        msgrcv(msgId, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR|IPC_NOWAIT);
        printData(dataMsg);
        printf("\n       Enter type (1) - lightweight warriors,"
               "\n                  (2) - heavyweight warriors,"
               "\n                  (3) - cavalry,"
               "\n                  (4) - workers"
               "\n"
               "\n                  (q) - quit\n");
        if( kbhit() ){
            type = getchar();
        }
    }
    if (type == 'q') return;

    while(amount < 0){
        msgrcv(msgId, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR|IPC_NOWAIT);
        printData(dataMsg);
        printf("\n       Enter amount (0-9):\n");
        if( kbhit() ){
            char temp = getchar();
            if (temp>='0' && temp <='9')
                amount = temp-'0';
            else if (temp == 'q')
                return;
        }
    }

    switch(type){
        case '1':
            sendBuildMsg(LIGHT,amount);
            break;
        case '2':
            sendBuildMsg(HEAVY,amount);
            break;
        case '3':
            sendBuildMsg(CAVALRY,amount);
            break;
        case '4':
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
            printf("Wrong entity type.\n");
            return;
    }
    int error = (int) msgsnd(msgId, &buildMsg, sizeof(buildMsg)- sizeof(buildMsg.mtype), IPC_NOWAIT);
    if(error == -1){
        perror("Sending");
        exitSave(0);
    }
    return;
}

void prepareAndSendAttackMsg(){
    int lightAmount = -1, heavyAmount = -1, cavalryAmount = -1;

    while(lightAmount < 0){
        msgrcv(msgId, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR|IPC_NOWAIT);
        printData(dataMsg);
        printf("\n       Enter lightweight warriors amount (0-9):\n");
        if( kbhit() ){
            char temp = getchar();
            if (temp >='0' && temp <='9')
                lightAmount = temp-'0';
            else if (temp == 'q')
                return;
        }
    }
    while(heavyAmount < 0){
        msgrcv(msgId, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR|IPC_NOWAIT);
        printData(dataMsg);
        printf("\n       Enter heavyweight warriors amount (0-9):\n");
        if( kbhit() ){
            char temp = getchar();
            if (temp >='0' && temp <='9')
                heavyAmount = temp-'0';
            else if (temp == 'q')
                return;
        }
    }
    while(cavalryAmount < 0){
        msgrcv(msgId, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR|IPC_NOWAIT);
        printData(dataMsg);
        printf("\n       Enter cavalry amount (0-9):\n");
        if( kbhit() ){
            char temp = getchar();
            if (temp >='0' && temp <='9')
                cavalryAmount = temp-'0';
            else if (temp == 'q')
                return;
        }
    }
    sendAttackMsg(lightAmount,heavyAmount,cavalryAmount);
}

void sendAttackMsg(int lightAmount, int heavyAmount, int cavalryAmount){
    Attack attackMsg;
    attackMsg.mtype = TYPE_ATTACK;
    attackMsg.light = lightAmount;
    attackMsg.heavy = heavyAmount;
    attackMsg.cavalry = cavalryAmount;
    int error = (int) msgsnd(msgId, &attackMsg, sizeof(attackMsg)- sizeof(attackMsg.mtype), IPC_NOWAIT);
    if(error == -1){
        perror("Sending");
        exitSave(0);
    }
    return;
}
