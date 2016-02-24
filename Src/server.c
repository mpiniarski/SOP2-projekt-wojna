#include <signal.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "communicationStructures.h"
#include "my_semafors.h"
#include "myipc.h"

void clear();
void overwriteSignals();
void handleSignal();
void initSemaphores();
void initGameData();
void resetPlayer(int playerId);
void connectToClinets();
void sendDataMsgToClient(int playerNumber, int endGame);
void updateResourcesPeriodically();
void updateResources();
void heartbeatPeriodially();
void winGame(int playerNumber);
void handleOrdersPeriodically();
void handleAttackOrders(int playerNumber);
void attackHandle(int attackingPlayerNumber, int lightAmount, int heavyAmount,int cavalryAmount);
void handleBuildOrders(int playerNumber);
void buildEntity(int playerNumber, int type);

int semId;
enum SEM{SEM_BUILD1=0, SEM_BUILD2, SEM_DATA1, SEM_DATA2, SEM_SERVER_DATA};

GameData* gameData;
int initMsgId;

// MAIN 
int main() {
    clear();
    overwriteSignals();

    printf("Initializing server...\n");
    initSemaphores();
    int shmId = shmgetSave(getpid(),sizeof(GameData));
    gameData = (GameData*)shmat(shmId, NULL, 0);
    initGameData();
    connectToClinets();

    //Start of game
    int err;
    if ( (err=fork()) == 0){
        updateResourcesPeriodically();
        exitSave(0);
    } else if(err == -1){
        perror("Fork"); exitSave(0);
    }

    if ( (err=fork()) == 0){
        heartbeatPeriodially();
        exitSave(0);
    } else if(err == -1){
        perror("Fork"); exitSave(0);
    }
    
    handleOrdersPeriodically();

    exitSave(0);
}

void clear(){
    printf("\033c");
}

void overwriteSignals(){
    signal(SIGINT,handleSignal);
    signal(SIGQUIT,handleSignal);
    signal(SIGTERM,handleSignal);
}

void handleSignal(){
    msgctl(initMsgId,IPC_RMID,0);
    msgctl(gameData->connectedIDs[0],IPC_RMID,0);
    msgctl(gameData->connectedIDs[0],IPC_RMID,0);
    exitSave(0);
}

void initSemaphores(){
    semId = semget(getpid(),5,IPC_CREAT | 0666);
    if (semId == -1){ perror("Semget"); exitSave(0);}
    I(semId,0,1); // Clinet 1 - building : SEM_BUILD1
    I(semId,1,1); // Client 2 - building : SEM_BUILD2
    I(semId,2,1); // Client 1 - data : SEM_DATA1
    I(semId,3,1); // Client 2 - data : SEM_DATA2 
    I(semId,4,1); // Server data : SEM_SERVER_DATA 
}

void initGameData(){
    clear();
    printf("Initializing game...\n");
    P(semId,SEM_SERVER_DATA);
    gameData->stopServer = 0;
    gameData->stopGame = 1;
    V(semId,SEM_SERVER_DATA);

    P(semId,SEM_DATA1);
    P(semId,SEM_DATA2);
    resetPlayer(0);
    resetPlayer(1);
    gameData->connectedIDs[0]=0;
    gameData->connectedIDs[1]=0;
    V(semId,SEM_DATA2);
    V(semId,SEM_DATA1);
}

void resetPlayer(int playerId){
    gameData->player[playerId].light=0;
    gameData->player[playerId].heavy=0;
    gameData->player[playerId].cavalry=0;
    gameData->player[playerId].workers=0;
    gameData->player[playerId].points=0;
    gameData->player[playerId].resources=300;
    strcpy(gameData->player[playerId].info,"Greetings my Lord. We are awaiting Your orders.\0");
}

void connectToClinets(){
    printf("Connecting to players...\n");
    initMsgId = msggetSave(connectionKey);
    int queueId[2]={0};
    Init initMsg;
    for (int playerNumber=0; playerNumber<2; playerNumber++){
        key_t key = getpid()+ playerNumber; // generate uniqe key
        queueId[playerNumber] = msggetSave(key);
        initMsg.mtype=1;
        initMsg.nextMsg=key;
        int error = msgsnd(initMsgId,&initMsg,sizeof(initMsg.nextMsg),0);
        if(error == -1){
            perror("Sending");
            exitSave(0);
        }
    }
    int connected = 0;
    while(connected < 2){
        printf("Waiting for player %d.\n",connected+1);
        int error = msgrcv(initMsgId,&initMsg,sizeof(initMsg.nextMsg),2,0);
        if(error == -1){
            perror("Receiving");
            exitSave(0);
        }
        connected ++;
        printf("Player %d connected.\n",connected);
    }
    msgctl(initMsgId,IPC_RMID,0); // remove just in case
    P(semId,SEM_DATA1);
    gameData->connectedIDs[0] = queueId[0];
    V(semId,SEM_DATA1);
    P(semId,SEM_DATA2);
    gameData->connectedIDs[1] = queueId[1];
    V(semId,SEM_DATA2);
    //Start game
    sendDataMsgToClient(0,0);
    sendDataMsgToClient(1,0);
    P(semId,SEM_SERVER_DATA);
    gameData->stopGame = 0;
    V(semId,SEM_SERVER_DATA);
    printf("--- GAME STARTED ---\n");
    return;
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
        strcpy(dataMsg.info, gameData->player[playerNumber].info);
        dataMsg.end = endGame;
        int error = msgsnd(queueId,&dataMsg,sizeof(dataMsg)-sizeof(dataMsg.mtype),IPC_NOWAIT);
        if(error == -1){
            perror("Sending dataMsg");
        }
        V(semId,playerNumber+2);
}

void updateResourcesPeriodically(){
    P(semId,SEM_SERVER_DATA);
    while(gameData->stopServer == 0){
        V(semId,SEM_SERVER_DATA);
        sleep(1);
        P(semId,SEM_SERVER_DATA);
        if (gameData->stopGame == 0){
            V(semId,SEM_SERVER_DATA);
            updateResources();
            P(semId,SEM_SERVER_DATA);
        }
    }
    V(semId,SEM_SERVER_DATA);
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

void heartbeatPeriodially(){
    int error;
    int clientLates[2]={0,0};

    P(semId,SEM_SERVER_DATA);
    while(gameData->stopServer == 0){
        V(semId,SEM_SERVER_DATA);
        sleep(2);
        P(semId,SEM_SERVER_DATA);
        if (gameData->stopGame == 0){
            V(semId,SEM_SERVER_DATA);
            for(int playerNumber=0; playerNumber<2; playerNumber++){
                Alive aliveMsg;
                aliveMsg.mtype = TYPE_ALIVE_SERVER;
                error = msgsnd(gameData->connectedIDs[playerNumber],&aliveMsg,0,IPC_NOWAIT);
                if(error == -1) perror("Sending aliveMsg");
                error = msgrcv(gameData->connectedIDs[playerNumber],&aliveMsg,0,TYPE_ALIVE_CLIENT,IPC_NOWAIT);
                if(error == -1){
                    if (++clientLates[playerNumber] == 3){
                        printf("Player %d process is dead: 3/3.\n",playerNumber+1);
                        clientLates[0]=0;
                        clientLates[1]=0;
                        winGame( (playerNumber+1)%2 );
                    }
                    printf("Player %d process is not responding: %d/3.\n",playerNumber+1,clientLates[playerNumber]);
                }
                else if(clientLates[playerNumber>0]){
                    printf("Player %d process responded. False alarm.\n",playerNumber+1);
                    clientLates[playerNumber]=0;
                }
            }
            P(semId,SEM_SERVER_DATA);
        }
    }
    V(semId,SEM_SERVER_DATA);
}


void winGame(int playerNumber){
    printf("--- PLAYER %d WON. GAME IS OVER. ---\n",playerNumber);
    int oppositePlayerNumber = (playerNumber+1)%2;
    P(semId,SEM_DATA1);
    strcpy(gameData->player[playerNumber].info,"My lord! The victory is our!\0");
    V(semId,SEM_DATA1);
    P(semId,SEM_DATA2);
    strcpy(gameData->player[oppositePlayerNumber].info,"Sir, enemy is at the gates... We are doomed, my lord.\0");
    V(semId,SEM_DATA2);
    
    sendDataMsgToClient(0,1);
    sendDataMsgToClient(1,1);

    initGameData();
    connectToClinets();
}

void handleOrdersPeriodically(){
    P(semId,SEM_SERVER_DATA);
    while(gameData->stopServer == 0){
        V(semId,SEM_SERVER_DATA);
        usleep(100*1000);
        P(semId,SEM_SERVER_DATA);
        if (gameData->stopGame == 0){
            V(semId,SEM_SERVER_DATA);
            handleBuildOrders(0);
            handleBuildOrders(1);
            handleAttackOrders(0);
            handleAttackOrders(1);
        }
        else{
            V(semId,SEM_SERVER_DATA);
            sleep(1);
        }
        P(semId,SEM_SERVER_DATA);
    }
    V(semId,SEM_SERVER_DATA);
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
                    strcpy(gameData->player[playerNumber].info,"Sir, our army is too small. Attack is not possible...");
            }
            else{
                gameData->player[playerNumber].light -= attackMsg.light;
                gameData->player[playerNumber].heavy -= attackMsg.heavy;
                gameData->player[playerNumber].cavalry -= attackMsg.cavalry;
                strcpy(gameData->player[playerNumber].info,"We march on the enemy, sir.\0");
                V(semId,playerNumber+2);
                if (fork() == 0){
                    attackHandle(playerNumber,attackMsg.light,attackMsg.heavy,attackMsg.cavalry);
                    exit(0);
                }
            }
        }
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

    int attackingAttackPower = 1*lightAmount + 1.5*heavyAmount + 3.5*cavalryAmount;
    int attackingDefendPower = 1.2*lightAmount + 3*heavyAmount + 1.2*cavalryAmount;
    int defendingAttackPower = 1*defendingLightAmount + 1.5*defendingHeavyAmount + 3.5*defendingCavalryAmount;
    int defendingDefendPower = 1.2*defendingLightAmount + 3*defendingHeavyAmount + 1.2*defendingCavalryAmount;
    

    if (attackingAttackPower > defendingDefendPower){
        defendingLightAmount = 0;
        defendingHeavyAmount = 0;
        defendingCavalryAmount = 0;
        point = 1;
    }
    else{
        defendingLightAmount -= (int)((float)defendingLightAmount * (float)attackingAttackPower/(float)defendingDefendPower);
        defendingHeavyAmount -= (int)((float)defendingHeavyAmount * (float)attackingAttackPower/(float)defendingDefendPower);
        defendingCavalryAmount -= (int)((float)defendingCavalryAmount * (float)attackingAttackPower/(float)defendingDefendPower);
    }

    if (defendingAttackPower > attackingDefendPower){
        lightAmount = 0; 
        heavyAmount = 0; 
        cavalryAmount = 0; 
    }
    else{
        lightAmount -= (int)((float)lightAmount * (float)defendingAttackPower/(float)attackingDefendPower);
        heavyAmount -= (int)((float)heavyAmount * (float)defendingAttackPower/(float)attackingDefendPower);
        cavalryAmount -= (int)((float)cavalryAmount * (float)defendingAttackPower/(float)attackingDefendPower);
    }
    gameData->player[defendingPlayerNumber].light = defendingLightAmount;
    gameData->player[defendingPlayerNumber].heavy = defendingHeavyAmount;
    gameData->player[defendingPlayerNumber].cavalry = defendingCavalryAmount;
    if(point) strcpy(gameData->player[defendingPlayerNumber].info,"Sir, the enemy attacked. We've lost the battle, but the war isn't over.\0");
    else strcpy(gameData->player[defendingPlayerNumber].info,"Sir, the enemy attacked, but Your kingdom is save.\0");
    V(semId,defendingPlayerNumber+2);

    P(semId,attackingPlayerNumber+2);
    gameData->player[attackingPlayerNumber].light += lightAmount;
    gameData->player[attackingPlayerNumber].heavy += heavyAmount;
    gameData->player[attackingPlayerNumber].cavalry += cavalryAmount;
    gameData->player[attackingPlayerNumber].points += point;
    if(point) strcpy(gameData->player[attackingPlayerNumber].info,"The battle is our, my lord! We are now 1 step closer to victory.\0");
    else strcpy(gameData->player[attackingPlayerNumber].info,"My lord... The attack wes repulsed. \0");
    if( gameData->player[attackingPlayerNumber].points == 5){
        V(semId,attackingPlayerNumber+2);
        winGame(attackingPlayerNumber);
    }
    else
        V(semId,attackingPlayerNumber+2);
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
            strcpy(gameData->player[playerNumber].info,"We don't have enough resources to recrute these people, my Lord.\0");
            V(semId,playerNumber+2);
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

void buildEntity(int playerNumber, int type){ 
    switch (type){
        case LIGHT:
            P(semId,playerNumber);
            sleep(2);
            P(semId,playerNumber+2);
            gameData->player[playerNumber].light++;
            strcpy(gameData->player[playerNumber].info,"Sir, one lightweight warrior arrived.\0");
            break;
        case HEAVY:
            P(semId,playerNumber);
            sleep(3);
            P(semId,playerNumber+2);
            gameData->player[playerNumber].heavy++;
            strcpy(gameData->player[playerNumber].info,"Sir, one heavyweight warrior arrived.\0");
            break;
        case CAVALRY:
            P(semId,playerNumber);
            sleep(5);
            P(semId,playerNumber+2);
            gameData->player[playerNumber].cavalry++;
            strcpy(gameData->player[playerNumber].info,"Sir, one cavalryman arrived.\0");
            break;
        case WORKER:
            P(semId,playerNumber);
            sleep(2);
            P(semId,playerNumber+2);
            gameData->player[playerNumber].workers++;
            strcpy(gameData->player[playerNumber].info,"Sir, one worker arrived.\0");
            break;
        default:
            break;
    }
    V(semId,playerNumber+2);
    V(semId,playerNumber);
    sendDataMsgToClient(playerNumber,0);
}
