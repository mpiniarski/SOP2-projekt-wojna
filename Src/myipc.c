#include "myipc.h"

#include <signal.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


void exitSave(int val){
    kill(0,SIGKILL);
    exit(val);
}

int msggetSave(key_t key){
    int msgId = msgget(key, IPC_CREAT | 0640);
    if(msgId == -1){
        perror("Opening message queue");
        exitSave(0);
    }
    int error = msgctl(msgId,IPC_RMID,0); // remove to clean just in case
    if(error == -1){
        perror("Remove queue");
        exitSave(0);
    }
    msgId = msgget(key, IPC_CREAT | 0640);
    if(msgId == -1){
        perror("Opening message queue");
        exitSave(0);
    }
    return msgId;
}

int shmgetSave(key_t key, size_t size){
    int shmId = shmget(key, size, IPC_CREAT|0640);
    if (shmId == -1){
        shmId = shmget(key, 0, IPC_CREAT|0640);
        if (shmId == -1){
            perror("Shared memory creating");
            exitSave(0);
        }
        int error = shmctl(shmId,IPC_RMID,0);
        if(error == -1){
            perror("Shared memory clearing");
            exitSave(0);
        }
        shmId = shmget(key, size, IPC_CREAT|0640);
        if (shmId == -1){
            perror("Shared memory creating");
            exitSave(0);
        }
    }
    return shmId;
}
