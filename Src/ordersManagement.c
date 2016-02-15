#include "ordersManagement.h"

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
