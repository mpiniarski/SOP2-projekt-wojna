#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>


#include <sys/select.h>
#include <termios.h>

#include "communicationStructures.h"

int connectToServerAndGetCommunicationKey();
void sendBuildMsg(int type, int amount);

int msgKey;

int getch()
{
    struct termios old;
    struct termios tmp;
    int ch;
    if (tcgetattr(STDIN_FILENO, &old)) return -1;
    memcpy(&tmp, &old, sizeof(old));
    tmp.c_lflag &= ~ICANON & ~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSANOW, (const struct termios*) &tmp)) return -1;
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, (const struct termios*) &old);
    return ch;
}

enum mode{MODE_VIEW,MODE_BUILD,MODE_ATTACK};
int mode;

int main() {
//    cannonicalOff();
    char stopGame = 0;
    mode = MODE_VIEW;

    int communicationKey = connectToServerAndGetCommunicationKey();
    msgKey = msgget(communicationKey, 0640);
    if (msgKey == -1) {
        perror("Opening message queue");
        exit(0);
    }

    if (fork() == 0){
        while (stopGame == 0){
            if (mode == MODE_VIEW){
                int ch;
                if ( (ch = getch()) ){
                    if ((char)ch >= 'a' && (char)ch <= 'z'){
                        if( (char)ch == 'b'){
                            mode=MODE_BUILD;
                        }
                    }
                }
            }
        }
        exit(0);
    }

    while (stopGame == 0){
        Data dataMsg;
        int error;
        char type;
        int amount;
        switch(mode){
            case MODE_VIEW:
                error = (int) msgrcv(msgKey, &dataMsg, sizeof(dataMsg)- sizeof(dataMsg.mtype),TYPE_DATA, MSG_NOERROR);
                if(error == -1){
                    perror("Receiveing");
                    exit(0);
                }

                printf("Points : %d\nResources : %d\n\nLight : %d\nHeavy : %d\nCavalry : %d\nWorkers : %d\n\n"
                        ,dataMsg.points, dataMsg.resources, dataMsg.light, dataMsg.heavy, dataMsg.cavalry, dataMsg.workers);


                if (dataMsg.end != 0) break;
                break;
            case MODE_BUILD:
                do{
                    printf("Enter type: (l)ight, (h)eavy, (c)avalry or (w)orkers\n");
                    type = getch();
                }
                while(type != 'l' && type !='h' && type !='c' && type !='w');

                do{
                    printf("Enter amount:\n");
                    scanf("%d",&amount);
                }
                while(amount<=0);

                sendBuildMsg(LIGHT,3);
                break;
        }

    }
    
    return 0;
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
}
