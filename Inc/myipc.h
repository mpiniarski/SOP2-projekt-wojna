#pragma once
#include <sys/types.h>

void exitSave(int val);
int msggetSave(key_t key);
int shmgetSave(key_t key, size_t size);
