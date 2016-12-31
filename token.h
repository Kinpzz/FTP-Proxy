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
int rate_control_write(struct token * proxy_token, int fd, char* buffer, int byte_num);