#pragma once

const key_t connectionKey = 15071417;
const key_t shmKey = 111;
const key_t semKey = 111;

const enum TYPE {TYPE_DATA=1, TYPE_BUILD, TYPE_ATTACK, TYPE_ALIVE};
const enum ENITY_TYPE {LIGHT=0, HEAVY, CAVALRY, WORKER};

typedef struct Init {
    long mtype;
    int nextMsg;
}Init;

typedef struct Data {
    long mtype; //1
    int light;
    int heavy;
    int cavalry;
    int workers;
    int points;
    int resources;
    char info[120];
    char end;
}Data;

typedef struct Build {
    long mtype; //2
    int light;
    int heavy;
    int cavalry;
    int workers;
}Build;

typedef struct Attack {
    long mtype; //3
    int light;
    int heavy;
    int cavalry;
}Attack;

typedef struct Alive {
    long mtype; //4
    char lol;
}Alive;

typedef struct PlayerData{
    int light;
    int heavy;
    int cavalry;
    int workers;
    int points;
    int resources;
    //char info[120];
} PlayerData;

typedef struct GameData{
    PlayerData player[2];
    int connected;
    int connectedIDs[2];
    char stopServer;
    char stopGame;
} GameData;

int shmSize = sizeof(GameData);
