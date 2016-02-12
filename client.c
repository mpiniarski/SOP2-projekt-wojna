#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>

#include "communicationStructures.h"

int main() {
    int error;

    int msgkey = msgget(connectionKey, IPC_CREAT | 0640);
    if (msgkey == -1){
        perror("Opening message queue");
        exit(0);
    }

    Init initMsg;
    initMsg.mtype=1;
    initMsg.nextMsg=-1;

    error = msgsnd(msgkey,&initMsg,sizeof(initMsg.nextMsg),0);
    if(error == -1){
        perror("Sending");
        exit(0);
    }

    error = (int) msgrcv(msgkey, &initMsg, sizeof(initMsg.nextMsg), 2, 0);
    if(error == -1){
        perror("Receiveing");
        exit(0);
    }

    if(initMsg.nextMsg == -1){
        printf("Server is busy. Try again later.");
        exit(0);
    }
    int communicationKey = initMsg.nextMsg;
    msgkey = msgget(communicationKey, IPC_CREAT | 0640);
    if (msgkey == -1){
        perror("Opening message queue");
        exit(0);
    }

}


