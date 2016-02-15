#pragma once

struct sembuf buf;

void P(int semid, int semnum);
void V(int semid, int semnum);
void I(int semid, int semnum, int initVal);
