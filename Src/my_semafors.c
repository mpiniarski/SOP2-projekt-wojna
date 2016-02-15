#include "my_semafors.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>

void P(int semid, int semnum){
    buf.sem_num = semnum;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    int err = semop(semid, &buf, 1);
    if (err == -1){
        perror("P");
        exit(0);
    }
}

void V(int semid, int semnum){
    buf.sem_num = semnum;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    int err = semop(semid, &buf, 1);
    if (err == -1){
        perror("V");
        exit(0);
    }
}

void I(int semid, int semnum, int initVal){
    int err = semctl(semid,semnum,SETVAL,initVal);
    if (err == -1){
        perror("Setval");
        exit(0);
    }
}
