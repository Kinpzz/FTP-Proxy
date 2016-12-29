#ifndef TOKEN_H
#define TOKEN_H
#include <pthread.h>
#endif

struct token {
    pthread_mutex_t mutex;
    int count;
    int rate;
};

void *token_generator(void *data);
//void* token_consumer(void* data);
