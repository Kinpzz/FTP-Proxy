/**
 * Simple FTP Proxy
 * Author: z58085111 @ HSNL
 * Author: kinpzz
 * 2016/12
 * **/
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <string>
#include <queue>
#include <iostream>

#define MAXSIZE 2048
#define FTP_PORT 8740
#define FTP_PASV_CODE 227
#define FTP_ADDR "140.114.71.159"
#define max(X,Y) ((X) > (Y) ? (X) : (Y))

int proxy_IP[4];
struct token {
    pthread_mutex_t mutex;;
    pthread_cond_t cond;
    int count;
    int rate;
};
struct buffer_unit {
    std::string buffer_str;
    int byte_num;
};

int connect_FTP(int ser_port, int clifd);
int proxy_func(int ser_port, int clifd, int rate);
int create_server(int port);
void rate_control();
void * token_generator(void* data);

int main (int argc, char **argv) {
    int ctrlfd, connfd, port, rate;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    if (argc < 4) {
        printf("[v] Usage: ./executableFile <ProxyIP> <ProxyPort> <Rate>\n");
        return -1;
    }

    sscanf(argv[1], " %d.%d.%d.%d", &proxy_IP[0], &proxy_IP[1], &proxy_IP[2], &proxy_IP[3]);
    port = atoi(argv[2]);
    rate = atoi(argv[3]);

    ctrlfd = create_server(port);
    clilen = sizeof(struct sockaddr_in);
    for (;;) {
        connfd = accept(ctrlfd, (struct sockaddr *)&cliaddr, &clilen);
        if (connfd < 0) {
            printf("[x] Accept failed\n");
            return 0;
        }

        printf("[v] Client: %s:%d connect!\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));
        if ((childpid = fork()) == 0) {
            close(ctrlfd);
            proxy_func(FTP_PORT, connfd, rate);
            close(connfd);
            printf("[v] Client: %s:%d terminated!\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));
            exit(0);
        }

        close(connfd);
    }
    return 0;
}

int connect_FTP(int ser_port, int clifd) {
    int sockfd;
    char addr[] = FTP_ADDR;
    int byte_num;
    char buffer[MAXSIZE];
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("[x] Create socket error");
        return -1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(ser_port);

    if (inet_pton(AF_INET, addr, &servaddr.sin_addr) <= 0) {
        printf("[v] Inet_pton error for %s", addr);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        printf("[x] Connect error");
        return -1;
    }

    printf("[v] Connect to FTP server\n");
    if (ser_port == FTP_PORT) {
        if ((byte_num = read(sockfd, buffer, MAXSIZE)) <= 0) {
            printf("[x] Connection establish failed.\n");
        }

        if (write(clifd, buffer, byte_num) < 0) {
            printf("[x] Write to client failed.\n");
            return -1;
        }
    }

    return sockfd;
}

int proxy_func(int ser_port, int clifd, int rate) {
    char buffer[MAXSIZE];
    int serfd = -1, datafd = -1, connfd;
    int data_port;
    int byte_num;
    int status, pasv[7];
    int childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    std::queue<struct buffer_unit> upload_buffer_queue;
    std::queue<struct buffer_unit> download_buffer_queue;
    struct buffer_unit* queue_front;
    bool can_write = false;

    // select vars
    int maxfdp1;
    int i, nready = 0;
    fd_set rset, allset;

    // connect to FTP server
    if ((serfd = connect_FTP(ser_port, clifd)) < 0) {
        printf("[x] Connect to FTP server failed.\n");
        return -1;
    }

    datafd = serfd;

    // initialize select vars
    FD_ZERO(&allset);
    FD_SET(clifd, &allset);
    FD_SET(serfd, &allset);
    // tokens not to tell upload and download
    struct token *proxy_token = (struct token *)malloc(sizeof(struct token));
    proxy_token->rate = rate;
    proxy_token->count = 0;
    pthread_mutex_init(&proxy_token->mutex,NULL);
    pthread_cond_init(&proxy_token->cond,NULL);
    // create a pthread to generate token
    pthread_t ptid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&ptid, &attr, token_generator, (void*)proxy_token);
    // char is 1 byte
    // selecting
    for (;;) {
        // reset select vars
        rset = allset;
        maxfdp1 = max(clifd, serfd) + 1;
        // reset can write
        can_write = false;

        // select descriptor,I/O multiplexer
        nready = select(maxfdp1 + 1, &rset, NULL, NULL, NULL);
        if (nready > 0) {
            // check FTP client socket fd
            // upload
            if (FD_ISSET(clifd, &rset)) {
                memset(buffer, '\0', MAXSIZE);
                if ((byte_num = read(clifd, buffer, MAXSIZE)) <= 0) {
                    printf("[!] Client terminated the connection.\n");
                    break;
                }
                printf("client : %s\n", buffer);
                // push_back
                struct buffer_unit read_buffer_unit;
                read_buffer_unit.buffer_str = buffer;
                read_buffer_unit.byte_num = byte_num;
                upload_buffer_queue.push(read_buffer_unit);
                // front
                queue_front = &upload_buffer_queue.front();
                byte_num = queue_front->byte_num;
                // get token
                pthread_mutex_lock(&proxy_token->mutex);
                if (proxy_token->count >= byte_num) {
                    // comsume token
                    proxy_token->count -= byte_num;
                    can_write = true;
                }
                pthread_mutex_unlock(&proxy_token->mutex);
                // write
                if (can_write) {
                    if (write(serfd, queue_front->buffer_str.c_str(), byte_num) < 0) {
                        printf("[x] Write to server failed.\n");
                        break;
                    }
                    upload_buffer_queue.pop();
                }
            }

            // check FTP server socket fd
            // download
            if (FD_ISSET(serfd, &rset)) {
                memset(buffer, '\0', MAXSIZE);
                if ((byte_num = read(serfd, buffer, MAXSIZE)) <= 0) {
                    printf("[!] Server terminated the connection.\n");
                    break;
                }
                if(ser_port == FTP_PORT)
                  buffer[byte_num] = '\0';
                                // push_back
                struct buffer_unit read_buffer_unit;
                read_buffer_unit.buffer_str = buffer;
                read_buffer_unit.byte_num = byte_num;
                download_buffer_queue.push(read_buffer_unit);
                // front
                queue_front = &download_buffer_queue.front();
                byte_num = queue_front->byte_num;
                // get token

                pthread_mutex_lock(&proxy_token->mutex);
                printf("token count: %d\n", proxy_token->count);
                if (proxy_token->count >= byte_num) {
                    // comsume token
                    proxy_token->count -= byte_num;
                    can_write = true;
                }
                pthread_mutex_unlock(&proxy_token->mutex);
                pthread_cond_signal(&proxy_token->cond);
                if (can_write) {
                    status = atoi(queue_front->buffer_str.c_str());
                    if (status > 99) printf("server : %d %d\n", status, ser_port);
                    if (status == FTP_PASV_CODE && ser_port == FTP_PORT) {

                        sscanf(buffer, "%d Entering Passive Mode (%d,%d,%d,%d,%d,%d)",&pasv[0],&pasv[1],&pasv[2],&pasv[3],&pasv[4],&pasv[5],&pasv[6]);
                        memset(buffer, '\0', MAXSIZE);
                        // force data connection to proxy, set to issue1
                        sprintf(buffer, "%d Entering Passive Mode (%d,%d,%d,%d,%d,%d)\n", status, proxy_IP[0], proxy_IP[1], proxy_IP[2], proxy_IP[3], pasv[5], pasv[6]);

                        if ((childpid = fork()) == 0) {
                            data_port = pasv[5] * 256 + pasv[6];
                            datafd = create_server(data_port);
                            if (write(clifd, buffer, byte_num) < 0) {
                                printf("[x] Write to client failed.\n");
                                break;
                            }
                            printf("[-] Waiting for data connection!\n");
                            clilen = sizeof(struct sockaddr_in);
                            connfd = accept(datafd, (struct sockaddr *)&cliaddr, &clilen);
                            if (connfd < 0) {
                                printf("[x] Accept failed\n");
                                return 0;
                            }

                            printf("[v] Data connection from: %s:%d connect.\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));
                            proxy_func(data_port, connfd, rate);
                            close(connfd);
                            printf("[!] End of data connection!\n");
                            exit(0);
                        }
                    } else {  // parent
                        if (write(clifd, queue_front->buffer_str.c_str(), byte_num) < 0) {
                            printf("[x] Write to client failed.\n");
                            break;
                        }
                        download_buffer_queue.pop();
                    }
                }
            }
        } else {
            printf("[x] Select() returns -1. ERROR!\n");
            return -1;
        }
    }
    // cancle token generator
    // free memory
    if (pthread_cancel(ptid) == 0) {
        free(proxy_token);
        pthread_mutex_destroy(&proxy_token->mutex);
    } else {
        printf("Cancle token_generator pthread fail\n");
    }
    return 0;
}

int create_server(int port) {
    int listenfd;
    struct sockaddr_in servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *)&servaddr , sizeof(servaddr)) < 0) {
        //print the error message
        perror("bind failed. Error");
        return -1;
    }

    listen(listenfd, 3);
    return listenfd;
}

void rate_control() {
    /**
     * Implement your main logic of rate control here.
     * Add return variable or parameters you need.
     * **/
}

void *token_generator(void *data) {
    // detach thread from process
    pthread_detach(pthread_self());
    struct token* proxy_token;
    proxy_token = (struct token*)data;
    // two cancle point to avoid block by thread lock
    clock_t start, end;
    double dur = 0, sleep_time;
    int token_per_time;
    int usleep_time;
    while (1) {
        start = clock();
        pthread_testcancel();
        token_per_time = 500;
        // block lock
        pthread_mutex_lock(&proxy_token->mutex);
        proxy_token->count += token_per_time;
        printf("generate %d tokens\n", proxy_token->count);
        // unlock
        pthread_mutex_unlock(&proxy_token->mutex);
        sleep_time = ((double)token_per_time/proxy_token->rate)-dur;
        usleep_time = (int)1000000*(sleep_time);
        if (usleep_time > 0) {
            usleep(usleep_time);
        } else {
            token_per_time++;
        }
        // cancle point
        pthread_testcancel();
        end = clock();
        dur = (double)(end-start)/CLOCKS_PER_SEC;
    }
    return ((void*)0);
}