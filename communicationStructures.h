#ifndef WOJNA_STRUKTURYKOMUNIKACJI_H
#define WOJNA_STRUKTURYKOMUNIKACJI_H

const key_t connectionKey = 15071410;

const enum TYPE {DATA=1, BUILD, ATTACK, IS_DEAD};

enum bool {false, true};


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
    char* info;
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


#endif //WOJNA_STRUKTURYKOMUNIKACJI_H
