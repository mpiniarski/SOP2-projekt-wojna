#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>

#include <sys/shm.h>
#include <sys/sem.h>

#include "communicationStructures.h"
#include "my_semafors.h"

GameData* gameData;
int semId;

void initGameData();
void connectToClinets();
void checkClients();
void sendDataMsgToClient(int playerNumber);
void checkBuidOrders(int playerNumber);
void checkAttackOrders(int playerNumber);

int main() {
    int shmId = shmget(shmKey, shmSize, IPC_CREAT|0640);
    if (shmId == -1){
        perror("Shared memory creating");
        exit(0);
    }

    semId = semget(semKey,1,IPC_CREAT | 0666);
    if (semId == -1){
        perror("Semget");
        exit(0);
    }
    I(semId,0,1);
 

    gameData = (GameData*)shmat(shmId, NULL, 0);
    initGameData();

    if ( fork() == 0){
        while(gameData->stopServer == 0){
            if(gameData->stopGame == 0){
                sendDataMsgToClient(0);
                sendDataMsgToClient(1);
                sleep(1);
            }
        }
        exit(0);
    }

    if ( fork() == 0){
        while(gameData->stopServer == 0){
            if (gameData->stopGame == 0){
                sleep(1);
                gameData->player[0].resources += 50 + 5*gameData->player[0].workers;
                gameData->player[1].resources += 50 + 5*gameData->player[1].workers;
            }
        }
        exit(0);
    }

    while(gameData->stopServer == 0){
        connectToClinets();
        if(gameData->stopGame == 0){
            checkBuidOrders(0);
            checkBuidOrders(1);
            checkAttackOrders(0);
            checkAttackOrders(1);
        }
    }
//
//    if ( fork() > 0){
////        checkClients();
//        exit(0);
//    }
//
//
//    if ( fork() > 0){
//        exit(0);
//    }
}

void resetPlayer(int playerId){
    gameData->player[playerId].light=0;
    gameData->player[playerId].heavy=0;
    gameData->player[playerId].cavalry=0;
    gameData->player[playerId].workers=0;
    gameData->player[playerId].points=0;
    gameData->player[playerId].resources=300;
}

void initGameData(){
    resetPlayer(0);
    resetPlayer(1);
    gameData->connected = 0;
    gameData->connectedIDs[0]=0;
    gameData->connectedIDs[1]=0;
    gameData->stopServer = 0;
    gameData->stopGame = 1;
}

void connectToClinets(){
//    int msgId = msgget(connectionKey, IPC_CREAT|IPC_EXCL | 0640);
//    if (msgId == -1){
//        error = msgctl(connectionKey,IPC_RMID,0); // remove just in case
//        if(error == -1){
//            perror("Remove queue");
//        }
//        msgId = msgget(connectionKey, IPC_CREAT|IPC_EXCL | 0640);
//        if (msgId == -1){
//            perror("Opening main message queue");
//            exit(0);
//        }
//    }
    int msgId = msgget(connectionKey, IPC_CREAT | 0640);
    if (msgId == -1){
        perror("Opening message queue");
        exit(0);
    }

    int error;
    Init initMsg;
    while(gameData->stopServer == 0){
        error = (int) msgrcv(msgId, &initMsg, sizeof(initMsg.nextMsg), 1, MSG_NOERROR|IPC_NOWAIT);
        if(error == -1){
            return;
        }
        else{
            key_t key;
            if (gameData->connected<2){
                key = getpid()+gameData->connected; // generate uniqe key

                gameData->connectedIDs[gameData->connected] = msgget(key, IPC_CREAT|IPC_EXCL | 0640); // save msgget
                if (gameData->connectedIDs[gameData->connected] == -1){
                    error = msgctl(key,IPC_RMID,0); // remove just in case
                    if(error == -1){
                        perror("Remove queue");
                    }
                    gameData->connectedIDs[gameData->connected] = msgget(key, IPC_CREAT|IPC_EXCL | 0640);
                    if (gameData->connectedIDs[gameData->connected] == -1){
                        perror("Opening message queue with client");
                        exit(0);
                    }
                }

                (gameData->connected)++;
                if(gameData->connected == 2) 
                    gameData->stopGame = 0;

                initMsg.mtype=2;
                initMsg.nextMsg=key;

                error = msgsnd(msgId,&initMsg,sizeof(initMsg.nextMsg),0);
                if(error == -1){
                    perror("Sending");
                    exit(0);
                }
            }
            else{
                key = 0;
            }

            printf("%d : %d\n",gameData->connected, key);
            return;
        }
    }
}

void checkClients(){
    int error;
    while(gameData->stopServer == 0){
        Alive aliveMsg;
        aliveMsg.mtype = 4;
        aliveMsg.lol='s';
        error = (int) msgsnd(gameData->connectedIDs[0], &aliveMsg, sizeof(aliveMsg)-sizeof(aliveMsg.mtype),0);
        if(error == -1){
            perror("Sending");
            exit(0);
        }
        error = (int) msgsnd(gameData->connectedIDs[1], &aliveMsg, sizeof(aliveMsg)-sizeof(aliveMsg.mtype),0);
        if(error == -1){
            perror("Sending");
            exit(0);
        }

        sleep(3);

        error = (int) msgrcv(gameData->connectedIDs[0], &aliveMsg, sizeof(aliveMsg)-sizeof(aliveMsg.mtype),TYPE_ALIVE,IPC_NOWAIT);
        if(error == -1){
            perror("Receiveing");
            exit(0);
        }
        if (aliveMsg.lol == 's'){
            gameData->stopGame = 1;
        }

        error = (int) msgrcv(gameData->connectedIDs[1], &aliveMsg, sizeof(aliveMsg)-sizeof(aliveMsg.mtype),TYPE_ALIVE,IPC_NOWAIT);
        if(error == -1){
            perror("Receiveing");
            exit(0);
        }
        if (aliveMsg.lol == 's'){
            gameData->stopGame = 1;
        }
    }
}

void sendDataMsgToClient(int playerNumber){
        Data dataMsg;
        dataMsg.mtype = TYPE_DATA;
        dataMsg.light = gameData->player[playerNumber].light;
        dataMsg.heavy = gameData->player[playerNumber].heavy;
        dataMsg.cavalry = gameData->player[playerNumber].cavalry;
        dataMsg.workers = gameData->player[playerNumber].workers;
        dataMsg.points = gameData->player[playerNumber].points;
        dataMsg.resources = gameData->player[playerNumber].resources;
        dataMsg.info = "No info bro";
        dataMsg.end = 0;
        int error = msgsnd(gameData->connectedIDs[playerNumber],&dataMsg,sizeof(dataMsg)-sizeof(dataMsg.mtype),IPC_NOWAIT);
        if(error == -1){
            perror("Sending dataMsg");
        }
}

void checkAttackOrders(int playerNumber){
        Attack attackMsg;
        int error = (int) msgrcv(gameData->connectedIDs[playerNumber], &attackMsg, sizeof(attackMsg)-sizeof(attackMsg.mtype),TYPE_ATTACK, IPC_NOWAIT);
        if(error != -1){
            if (fork() != 0){
//                exec();
                exit(0);
            }
        }
}

void buildEntity(int playerNumber, int type){
    switch (type){
        case LIGHT:
            P(semId,0);
            sleep(2);
            gameData->player[playerNumber].light++;
            break;
        case HEAVY:
            P(semId,0);
            sleep(3);
            gameData->player[playerNumber].heavy++;
            break;
        case CAVALRY:
            P(semId,0);
            sleep(5);
            gameData->player[playerNumber].cavalry++;
            break;
        case WORKER:
            P(semId,0);
            sleep(2);
            gameData->player[playerNumber].workers++;
            break;
        default:
            break;
    }
    V(semId,0);
}

void checkBuidOrders(int playerNumber){
        Build buildMsg;
        int error = (int) msgrcv(gameData->connectedIDs[playerNumber], &buildMsg, sizeof(buildMsg)-sizeof(buildMsg.mtype),TYPE_BUILD, IPC_NOWAIT);
        if(error != -1){
            int buildAmount[4]={buildMsg.light,buildMsg.heavy,buildMsg.cavalry,buildMsg.workers};
            int entityType;

            int cost = 100*buildAmount[LIGHT] + 250*buildAmount[HEAVY] + 550*buildAmount[CAVALRY] + 150*buildAmount[WORKER];

            if (cost > gameData->player[playerNumber].resources){
                // TODO send comunitact bejb
            }
            else{
                gameData->player[playerNumber].resources -= cost;
                for(entityType=0; entityType<4; entityType++){
                    int i;
                    for(i=0;i<buildAmount[entityType];i++){
                        if (fork() == 0){
                            buildEntity(playerNumber, entityType);
                            exit(0);
                        }
                    }
                }
            }
            return;
        }
}
