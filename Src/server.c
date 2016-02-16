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

void initGameData();
void connectToClinets();
void checkClients();
void sendDataMsgToClient(int playerNumber, int endGame);
void handleBuildOrders(int playerNumber);
void handleAttackOrders(int playerNumber);
void updateResources();

GameData* gameData;
int semId;
enum SEM{SEM_BUILD1=0, SEM_BUILD2, SEM_DATA1, SEM_DATA2, SEM_SERVER_DATA};

int main() {
    int shmId = shmget(shmKey, shmSize, IPC_CREAT|0640);
    if (shmId == -1){
        perror("Shared memory creating");
        exit(0);
    }

    semId = semget(semKey,5,IPC_CREAT | 0666);
    if (semId == -1){
        perror("Semget");
        exit(0);
    }
    I(semId,0,1); // Clinet 1 - building : SEM_BUILD1
    I(semId,1,1); // Client 2 - building : SEM_BUILD2
    I(semId,2,1); // Client 1 - data : SEM_DATA1
    I(semId,3,1); // Client 2 - data : SEM_DATA2 
    I(semId,4,1); // Server data : SEM_SERVER_DATA 

    gameData = (GameData*)shmat(shmId, NULL, 0);
    initGameData();

    if ( fork() == 0){
        int stopGame;
        int stopServer;
        do{
            P(semId,SEM_SERVER_DATA);
            stopGame = gameData->stopGame;
            stopServer = gameData->stopServer;
            V(semId,SEM_SERVER_DATA);
            if (stopGame == 0){
                sleep(1);
                printf("UpdateRes\n");
                updateResources();
            }
        } while(stopServer == 0);
        exit(0);
    }

    int stopGame;
    int stopServer;
    do{
        usleep(100*1000);
        connectToClinets();
        P(semId,SEM_SERVER_DATA);
        stopGame = gameData->stopGame;
        stopServer = gameData->stopServer;
        V(semId,SEM_SERVER_DATA);
        if (stopGame == 0){
            printf("HandleAllShit\n");
            handleBuildOrders(0);
            handleBuildOrders(1);
            handleAttackOrders(0);
            handleAttackOrders(1);
        } 
    } while(stopServer == 0);
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


void resetPlayer(int);
void initGameData(){
    P(semId,SEM_DATA1);
    P(semId,SEM_DATA2);
    resetPlayer(0);
    resetPlayer(1);
    gameData->connectedIDs[0]=0;
    gameData->connectedIDs[1]=0;
    V(semId,SEM_DATA2);
    V(semId,SEM_DATA1);

    P(semId,SEM_SERVER_DATA);
    gameData->stopServer = 0;
    gameData->stopGame = 1;
    gameData->connected = 0;
    V(semId,SEM_SERVER_DATA);
}
void resetPlayer(int playerId){
    gameData->player[playerId].light=0;
    gameData->player[playerId].heavy=0;
    gameData->player[playerId].cavalry=0;
    gameData->player[playerId].workers=0;
    gameData->player[playerId].points=0;
    gameData->player[playerId].resources=300;
}

void connectToClinets(){ // TODO semaphores
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
                if(gameData->connected == 2){
                    gameData->stopGame = 0;
                    sendDataMsgToClient(0,0);
                    sendDataMsgToClient(1,0);
                }


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

void updateResources(){
    P(semId,SEM_DATA1);
    gameData->player[0].resources += 50 + 5*gameData->player[0].workers;
    V(semId,SEM_DATA1);
    sendDataMsgToClient(0,0);
    P(semId,SEM_DATA2);
    gameData->player[1].resources += 50 + 5*gameData->player[1].workers;
    V(semId,SEM_DATA2);
    sendDataMsgToClient(1,0);
}

void sendDataMsgToClient(int playerNumber, int endGame){
        Data dataMsg;
        dataMsg.mtype = TYPE_DATA;
        P(semId,playerNumber+2);
        int queueId = gameData->connectedIDs[playerNumber];
        dataMsg.light = gameData->player[playerNumber].light;
        dataMsg.heavy = gameData->player[playerNumber].heavy;
        dataMsg.cavalry = gameData->player[playerNumber].cavalry;
        dataMsg.workers = gameData->player[playerNumber].workers;
        dataMsg.points = gameData->player[playerNumber].points;
        dataMsg.resources = gameData->player[playerNumber].resources;
        //strcpy(dataMsg.info, gameData->player[playerNumber].info);
        dataMsg.end = endGame;
        int error = msgsnd(queueId,&dataMsg,sizeof(dataMsg)-sizeof(dataMsg.mtype),IPC_NOWAIT);
        if(error == -1){
            perror("Sending dataMsg");
        }
        V(semId,playerNumber+2);
}

void winGame(int playerNumber){
    sendDataMsgToClient(0,1);
    sendDataMsgToClient(1,1);
    initGameData();
}

void attackHandle(int attackingPlayerNumber, int lightAmount, int heavyAmount,int cavalryAmount){
    if(lightAmount+heavyAmount+cavalryAmount == 0) return;
    sleep(5);
    int defendingPlayerNumber = (attackingPlayerNumber + 1) % 2;
    int point = 0;

    P(semId,defendingPlayerNumber+2);
    int defendingLightAmount = gameData->player[defendingPlayerNumber].light;
    int defendingHeavyAmount = gameData->player[defendingPlayerNumber].heavy;
    int defendingCavalryAmount = gameData->player[defendingPlayerNumber].cavalry;

    int attackingAttackPower = 1*lightAmount + 1.2*heavyAmount + 2.5*cavalryAmount;
    int attackingDefendPower = 1.2*lightAmount + 3*heavyAmount + 1.2*cavalryAmount;
    int defendingAttackPower = 1*defendingLightAmount + 1.2*defendingHeavyAmount + 2.5*defendingCavalryAmount;
    int defendingDefendPower = 1.2*defendingLightAmount + 3*defendingHeavyAmount + 1.2*defendingCavalryAmount;
    

    if (attackingAttackPower > defendingDefendPower){
        defendingLightAmount = 0;
        defendingHeavyAmount = 0;
        defendingCavalryAmount = 0;
        point = 1;
    }
    else{
        defendingLightAmount -= defendingLightAmount * (float)attackingAttackPower/defendingDefendPower;
        defendingHeavyAmount -= defendingHeavyAmount * (float)attackingAttackPower/defendingDefendPower;
        defendingCavalryAmount -= defendingCavalryAmount * (float)attackingAttackPower/defendingDefendPower;
    }

    if (defendingAttackPower > attackingDefendPower){
        lightAmount = 0; 
        heavyAmount = 0; 
        cavalryAmount = 0; 
        printf("%d,%d,%d\n",lightAmount,heavyAmount,cavalryAmount);
    }
    else{
        lightAmount -= lightAmount * (float)defendingAttackPower/attackingDefendPower;
        heavyAmount -= heavyAmount * (float)defendingAttackPower/attackingDefendPower;
        cavalryAmount -= cavalryAmount * (float)defendingAttackPower/attackingDefendPower;
        //printf("%d,%d,%d\n",lightAmount,heavyAmount,cavalryAmount);
    }
        gameData->player[defendingPlayerNumber].light = defendingLightAmount;
        gameData->player[defendingPlayerNumber].heavy = defendingHeavyAmount;
        gameData->player[defendingPlayerNumber].cavalry = defendingCavalryAmount;
        V(semId,defendingPlayerNumber+2);

        P(semId,attackingAttackPower+2);
        gameData->player[attackingPlayerNumber].light += lightAmount;
        gameData->player[attackingPlayerNumber].heavy += heavyAmount;
        gameData->player[attackingPlayerNumber].cavalry += cavalryAmount;
        gameData->player[attackingPlayerNumber].points += point;
        if( gameData->player[attackingPlayerNumber].points == 5){
            V(semId,attackingAttackPower+2);
            winGame(attackingPlayerNumber);
        }
        else
            V(semId,attackingAttackPower+2);
    
}


void handleAttackOrders(int playerNumber){
        Attack attackMsg;

        P(semId,playerNumber+2);
        int queueId = gameData->connectedIDs[playerNumber];
        V(semId,playerNumber+2);

        int error = (int) msgrcv(queueId, &attackMsg, sizeof(attackMsg)-sizeof(attackMsg.mtype),TYPE_ATTACK, IPC_NOWAIT);
        if(error != -1){
            P(semId,playerNumber+2);
            if (attackMsg.light < 0 || attackMsg.light > gameData->player[playerNumber].light || 
                attackMsg.heavy < 0 || attackMsg.heavy > gameData->player[playerNumber].heavy ||
                attackMsg.cavalry < 0 || attackMsg.cavalry > gameData->player[playerNumber].cavalry){
                V(semId,playerNumber+2);
                //TODO Comunitate of failure
            }
            else{
                gameData->player[playerNumber].light -= attackMsg.light;
                gameData->player[playerNumber].heavy -= attackMsg.heavy;
                gameData->player[playerNumber].cavalry -= attackMsg.cavalry;
                V(semId,playerNumber+2);
                if (fork() != 0){
                    attackHandle(playerNumber,attackMsg.light,attackMsg.heavy,attackMsg.cavalry);
                    exit(0);
                }
            }
        }
}

void buildEntity(int playerNumber, int type){ 
    switch (type){
        case LIGHT:
            P(semId,playerNumber);
            sleep(2);
            P(semId,playerNumber+2);
            gameData->player[playerNumber].light++;
            break;
        case HEAVY:
            P(semId,playerNumber);
            sleep(3);
            P(semId,playerNumber+2);
            gameData->player[playerNumber].heavy++;
            break;
        case CAVALRY:
            P(semId,playerNumber);
            sleep(5);
            P(semId,playerNumber+2);
            gameData->player[playerNumber].cavalry++;
            break;
        case WORKER:
            P(semId,playerNumber);
            sleep(2);
            P(semId,playerNumber+2);
            gameData->player[playerNumber].workers++;
            break;
        default:
            break;
    }
    V(semId,playerNumber+2);
    V(semId,playerNumber);
    sendDataMsgToClient(playerNumber,0);
}

void handleBuildOrders(int playerNumber){
        Build buildMsg;

        P(semId,playerNumber+2);
        int queueId = gameData->connectedIDs[playerNumber];
        V(semId,playerNumber+2);
        int error = (int) msgrcv(queueId, &buildMsg, sizeof(buildMsg)-sizeof(buildMsg.mtype),TYPE_BUILD, IPC_NOWAIT);
        if(error != -1){
            int buildAmount[4]={buildMsg.light,buildMsg.heavy,buildMsg.cavalry,buildMsg.workers};
            int cost = 100*buildAmount[LIGHT] + 250*buildAmount[HEAVY] + 550*buildAmount[CAVALRY] + 150*buildAmount[WORKER];
            P(semId,playerNumber+2);
            if (cost > gameData->player[playerNumber].resources){
                V(semId,playerNumber+2);
                    // TODO send comunitact bejb
            }
            else{
                gameData->player[playerNumber].resources -= cost;
                V(semId,playerNumber+2);
                int entityType;
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
