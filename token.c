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
    double dur = 0, sleep_time;
    int token_per_time = proxy_token->rate / 100;
    token_per_time = token_per_time >= 2048 ? 2048 : token_per_time;
    int usleep_time;
    while (1) {
        start = clock();
        sleep_time = ((double)(token_per_time)/proxy_token->rate);
        usleep_time = (int)1000000*(sleep_time);
        if (usleep_time > 0) {
            usleep(usleep_time);
        }
        pthread_testcancel(); // cancle point
        pthread_mutex_lock(&proxy_token->mutex);
        if (proxy_token->count < 2*token_per_time) {
            proxy_token->count += token_per_time + dur * proxy_token->rate;
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
    /*
        seperate packet to get more fluent data flow
    */
    int div = proxy_token->rate >= 102400 ? 2048 : proxy_token->rate / 50;
    char sub_buffer[div];
    int offset = 0;
    int length = div;
    double sleep_time;
    int usleep_time;
    sleep_time = ((double)div)/proxy_token->rate;
    usleep_time = (int)1050*(sleep_time);
    while (byte_num > 0) {
        length = byte_num >= div ? div : byte_num;
        byte_num -= div;
        pthread_mutex_lock(&proxy_token->mutex);
        while (proxy_token->count < length) {
            pthread_mutex_unlock(&proxy_token->mutex);
            usleep(usleep_time);
            pthread_mutex_lock(&proxy_token->mutex);
        }
        //printf("consume %d tokens %d\n", proxy_token->count,length);
        proxy_token->count -= length;
        pthread_mutex_unlock(&proxy_token->mutex);
        
        // write
        strncpy(sub_buffer,buffer+offset,length);
        if (write(fd, sub_buffer, length) < 0) {
            printf("[x] Write to server failed.\n");
            return -1;
        }
        offset += length + 1;
    }
    return 0;
}