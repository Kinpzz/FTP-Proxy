#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "token.h"

void *token_generator(void *data) {
    // detach thread from process
    pthread_detach(pthread_self());
    struct token* proxy_token;
    proxy_token = (struct token*)data;
    // two cancle point to avoid block by thread lock
    clock_t start, end;
    double dur = 0;
    int token_per_time = proxy_token->token_per_time;
    int usleep_time = (int)1000000*(proxy_token->t);

    while (1) {
        // delay
        usleep(usleep_time);
        // dur can not count sleep time into it
        start = clock();
        pthread_testcancel(); // cancle point
        pthread_mutex_lock(&proxy_token->mutex);
        if (proxy_token->count < 2*token_per_time) {
            proxy_token->count += token_per_time + (int)(dur * proxy_token->rate);
            //printf("generate %d tokens\n", proxy_token->count);
        }
        // unlock
        pthread_mutex_unlock(&proxy_token->mutex);
        pthread_testcancel();
        end = clock();
        dur = (double)(end-start)/CLOCKS_PER_SEC;
    }
    return ((void*)0);
}

int rate_control_write(struct token* proxy_token, int fd, char* buffer, int byte_num) {
    // seperate packet to get more fluent data flow
    int usleep_time = (int)1000000*(proxy_token->t);
    pthread_mutex_lock(&proxy_token->mutex);
    while (proxy_token->count < byte_num) {
        pthread_mutex_unlock(&proxy_token->mutex);
        usleep(usleep_time);
        pthread_mutex_lock(&proxy_token->mutex);
    }
    //printf("consume %d tokens %d\n", proxy_token->count,byte_num);
    proxy_token->count -= byte_num;
    pthread_mutex_unlock(&proxy_token->mutex);
    
    // write
    if (write(fd, buffer, byte_num) < 0) {
        printf("[x] Write to server failed.\n");
        return -1;
    }
    return 0;
}