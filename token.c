#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "token.h"

void *token_generator(void *data) {
    // detach thread from process
    pthread_detach(pthread_self());
    struct token* proxy_token;
    proxy_token = (struct token*)data;
    // two cancle point to avoid block by thread lock
    clock_t start, end;
    double dur = 0, sleep_time;
    int token_per_time = 1024;
    int usleep_time;
    while (1) {
        start = clock();
        sleep_time = ((double)(token_per_time)/proxy_token->rate)-dur;
        // why double time?
        usleep_time = (int)2000000*(sleep_time);
        if (usleep_time > 0) {
            usleep(usleep_time);
        } else if (usleep_time < 0){
            token_per_time += 100;
        }
        pthread_testcancel(); // cancle point

        if (proxy_token->count < token_per_time*2) {
            pthread_mutex_lock(&proxy_token->mutex);
            proxy_token->count += token_per_time;
            //printf("generate %d tokens\n", proxy_token->count);
            // unlock
            pthread_mutex_unlock(&proxy_token->mutex);
        }
        pthread_testcancel();
        end = clock();
        dur = (double)(end-start)/CLOCKS_PER_SEC;
    }
    return ((void*)0);
}
