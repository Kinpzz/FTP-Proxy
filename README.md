# FTP Proxy

## 运行方法

```sh
$ make clean
$ make
$ ./proxy <ProxyIP> <ProxyPort> <Rate>
```
例如: `./proxy 127.0.0.1 8000 100` 可以将下载和上传的速度限制为100KB/sec. 若需要更换限制的速度，则`control+c`终止程序之后，使用新的`Rate`重新运行proxy。

Filezilla 通过被动模式连接上`<ProxyIP>`和`<ProxyPort>`即可，连接所需要的账号密码是原FTP server所需要的账号密码。注意需要选择不加密的plain FTP，以及被动模式。通过在filezilla客户端点击`control+s`可以进入站点管理，在`Encryption` 选择`Use plain FTP`在`Transfer Settings`设置为`Passive`

![](https://ws4.sinaimg.cn/large/6177e8b1gw1fbq608h9h0j20ym0rugrb.jpg)

![](https://ws4.sinaimg.cn/large/6177e8b1gw1fbq62i34cyj20yc0s2af3.jpg)

## 运行环境

适用于linux系统，本次测试环境使用ubuntu 14.04

所依赖的库有linux下的`<sys/socket.h>`和`<pthread.h>`等。

注意：使用macOS进行测试可能会出现问题，因为socket的window size在macOS下使用setsockopt无法限制住，详细可以参考我所记录的issue[#2](https://github.com/Kinpzz/FTP-Proxy/issues/2)

## Proxy 理解

![](https://ws3.sinaimg.cn/large/6177e8b1gw1fbq5xoh7voj211i0abgmi.jpg)

### Proxy 作用

FTP proxy 所起的作用在FTP client与FTP server之间进行数据的转送。本次的Proxy所起的作用就是从FTP server下载数据，以及从FTP client上传数据到FTP server之间进行速度的限制。其实，proxy的作用还有很多，比如可以制作成应用层的防火墙，在封包传到server/client之前，对封包的每一层进行检查，是一种安全级别非常高的防火墙，但是速度相对比较慢。也可以通过proxy对封包数据进行过滤，速度进行限制，放置server接受太多的数据无法处理过来或者造成网络拥塞。也可以当做一种一种代理上网的方式。但是FTP proxy的缺点就是，只能识别FTP协议，所能识别的协议比较有限。此处的FTP proxy只是针对未加密的FTP 被动模式所涉及。

### 程序基础架构

整体的FTP proxy内部架构如下。可以允许同时有多个client与proxy进行连接。proxy的主程序会开启一个socket不断进行listen，每次有client与proxy进行连接的时候，都会创建一个子进程来处理。每一个子进程都会与client建立一条socket连线，再与server建立一条socket连线。

此处的client是广义而言的，并不是单指一个主机，一个进程都可以作为一个client。在Filezilla中，每新建一个下载或上传进程，都会向proxy主程序发起一个新的连接请求。所以每一个任务都可以被视作是一个新的client。

Proxy面向client就相当于server一样，面向server就相当于client一样。所以Proxy其实既要实现client的功能也要实现server的功能。

![](https://ws2.sinaimg.cn/large/6177e8b1gw1fbq5xzwad4j21330jbmzh.jpg)

### FTP 被动模式

在FTP 被动模式下client与server上传数据的过程中使用的是建立连线所使用的`Port`，而需要从FTP下载数据时，client会开放一个特定的`Port`然后将端口的信息以及被动模式信息发送给Server，server接受到消息会开放一个端口来与client进行连接，并且回应client并且与client的这个端口建立连线。被动模式主要是为了避免因为cilent的端口没有开放，而server发送了该端口的连接请求而被client的防火墙所阻挡而设计。

所以，在FTP的被动模式下每个client会有两个连线，数据通路和命令通路。

* 数据通路，负责从server下载信息，如文件、目录信息等。
* 命令通路，该通路负责从client上传控制信息或者数据到server，以及server返回一些相应的控制信息。

也正因为这两个通路是分离开来的，以至于在过程中会出现两个通路的信息不同步等一些非常tricky的问题。在后面的问题解决，我有通过github的issue比较详细地记录下来。例如，从server下载完数据到prxoy，server会通过命令通路发送信息给proxy 226传送完毕的信息，proxy直接将信息转传给client，client便认为数据已经传送完毕，但是实际因为限速原因，数据还留在proxy没有传送完毕。亦或者是从client上传数据到proxy，client上传完毕，但是proxy数据还没有转送完毕，此时client过长时间没有收到proxy返回的226信息，自动断开连接等问题。

### FTP 常见命令 & server 响应码

在进行调试程序的时候，经常要通过判断server的response code和client的命令来进行判断。而且proxy也要有识别server response code的能力，比如在被动模式下建立数据通路就要识别server 返回的227信息。所以有一些常见的命令和响应吗需要了解。

| Server Code | Explanation                              |
| :---------- | ---------------------------------------- |
| `226`       | Closing data connection. Requested file action successful (for example, file transfer or file abort). |
| `227`       | Entering Passive Mode (h1,h2,h3,h4,p1,p2). |
| `150`       | File status okay; about to open data connection. |

| 命令   | 解释                             |
| ---- | ------------------------------ |
| RETR | 传输文件副本                         |
| STOR | 接收数据并且在服务器站点保存为文件              |
| LIST | 如果指定了文件或目录，返回其信息；否则返回当前工作目录的信息 |

详细地可以参考wikipedia:

* [FTP命令列表](https://zh.wikipedia.org/wiki/FTP%E5%91%BD%E4%BB%A4%E5%88%97%E8%A1%A8),
* [List of FTP server return codes](https://en.wikipedia.org/wiki/List_of_FTP_server_return_codes)

### Linux Socket

此次proxy所使用的基础就是linux下的socket，使用它，我们不必去实现一些复杂的TCP协议的功能。只要通过其提供的API就能实现向读写文件一样控制TCP流了。

Linux Socket为网络的低层协议提供了API(接口)。Socket 实现了IP的接口，在socket的基础之上，我们可以建立网络层的协议。比如建立TCP,建立UDP。通过Socket可以很容易地帮助我们实现这一点。而且它提供了read/write这种功能的函数，让我们可以像是处理文件读写一样来处理数据的传输。

#### 通过Socket实现TCP

这里简单地介绍一下，如果通过Socket实现TCP，由于Proxy同时面向Client和Server所以，在其内部会有client的功能也会有server的功能。

##### Server部分

在proxy内部，通过要与client进行连接，就要模拟server的功能。

1. 通过socket()函数创建一个socket
2. 将这个server地址端口进行初始化，使用结构体sockaddr_in
3. 将socket于server的地址端口进行绑定，使用bind()
4. 对这个socket进行listen
5. 通过accpet来接受client的connect请求(此处在proxy的主程序中循环等待accept)
6. 连接完毕通过close关闭socket

如下方所示，这就是在proxy中创建一个TCP连接server的server部分。

```c
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
```

##### client部分

同样地，在proxy部分，要与server进行连接，就要模拟client的功能。

1. 创建一个新的socket
2. 初始化server的地址信息
3. 通过connect连接到服务器
4. 连接完毕通过close关闭socket

```c
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
```

在连线建立完成之后就可以通过，server部分通过操作client的socket，client部分通过操作server的socket部分，就可以用read/write函数像文件读写一样来进行操作了。



## 速度限制功能

此处任务的核心目的，就是在proxy对上传和下载速度进行控制。下面将介绍，用来进行限速的算法token bucket 令牌桶算法的基本原理，以及如何在proxy中实现这一算法。

### Token bucket(令牌桶算法)

#### 介绍

令牌桶算法是网络流量整形(Traffic Shaping)和速度限制(Rate Limiting)中所常用的一种算法。它有两个主要的特点，一个就是能够限制速度，控制封包发送的数量，另一个就是允许你突发的数据传输。相比于leaky bucket算法，它能更好地满足QoS服务品质标准，允许一段时间发送较多的封包，而之后不发送封包，但是保证封包发送的数目是在限制范围之内的，既能够保证限制这段时间内发送封包的平均速度，又能允许比较大的封包到来。而leaky bucket就是不能允许突发的流量的。

#### 原理

下面用一张图来解释一下算法的具体原理:

![](https://ws1.sinaimg.cn/large/6177e8b1gw1fbq7qvqigoj20zx0kk41k.jpg)

令牌桶算法主要有三个主要部分:

1. token generation(令牌产生)：每个`1/Rate`时间往令牌桶中放入一个令牌（相当于速度单位的最小单位）
2. 封包队列:建立一个队列来暂时存储封包，当封包进入proxy之后，队列未满就暂存下来，若队列已经满了则不进行接收。
3. 消耗令牌，发送封包：每次从封包队列的front取出封包，如果令牌桶中令牌的数量大于等于封包大小（取决于速度单位）则消耗相应的令牌，发送该封包，若没有则继续等待。



### Linux下令牌桶具体实现方法

#### 使用socket缓存区

##### 核心

在`ftp_proxy.c`的`int proxy_func(int ser_port, int clifd, int rate)`就是在建立好两边的连线之后进行内部读写和限速功能的核心函数了，也相当于删除结构图中的每一个单独处理的子进程。

![](https://ws3.sinaimg.cn/large/6177e8b1jw1fbq9aoqescj217c0fggmw.jpg)

##### 缓存区（充当封包队列）

如图所示中间部分，就是proxy的每一个子进程所起的作用。每个socket部分，在建立完每TCP连线之后都会有两个缓存区，一个是发送缓存区，一个是接受缓存区。也就是说proxy子进程上会有4个缓存区

* client socket的缓存区
  * 接受缓存区，read client socket 从client读的数据，就是先缓存到这里再从这里面读取数据的
  * 发送缓存区，write client socket 发送到client的数据，就是先缓存到这里再进行发送的
* server socket的缓存区
  * 接受缓存区，read server socket 从server读的数据，就是先缓存到这里再从这里面读取数据的
  * 发送缓存区，write server socket 发送到server的数据，就是先缓存到这里再进行发送的

这里缓存区的大小就是所建立的TCP连线的window size的大小。TCP的拥塞协议根据，网络状况丢包情况，会进行动态地调整。具体TCP的协议，在socket内部已经实现好了，了解了缓存区基础已经足够我们在这里进行编程了，在这里不需要自己去写raw socket，来进行ACK等回复。只要把TCP连线当成stream(流)来处理，类似如文件流。

此处的缓存区，就可以充当我们令牌桶算法中**封包队列**的职责了。所以之后，我们在实现令牌桶算法的时候，就只要从一个缓存区的接受缓存区读取数据->消耗令牌->发送到另一个缓存区的发送缓存区即可。



#### 令牌结构体

 由于令牌的产生需要与其他任务运行是同步运行的，所以这里就考虑使用Linux下的pthread多线程来进行设计。一个proxy的子进程创建了一个线程，负责产生令牌，且创建多线程与fork一个子进程相比不会再次复制父进程的空间，因为那些数据大多数在这里是多余的。进程与线程共享一个共同的数据token。此处的token，因为在多线程里共享多个变量，需要使用结构体。所以我就定义了一个结构体token。在`token.h`可以详细看到。

```c
struct token {
    pthread_mutex_t mutex;
    int count;
    int rate;
    int token_per_time;
    double t; // delay time of generating and consuming token
};
```

这个结构体里，包含了令牌的数目，当前所限制的速率。以及一个互斥锁，因为在访问多线程临界区资源的时候，需要通过呼互斥锁来保证数据的正确性和完整性。

![](https://ws1.sinaimg.cn/large/6177e8b1gw1fbqeps6iwzj20ug0gc0th.jpg)

创建线程方法如下，proxy_func中创建

```c
    // tokens not to tell upload and download
    struct token *proxy_token = (struct token *)malloc(sizeof(struct token));
    proxy_token->rate = rate*1024;//Kbytes to bytes
    proxy_token->count = 0;
    // to balance consume and generate token frequency
    proxy_token->token_per_time = proxy_token->rate >= MAXSIZE*100 ? MAXSIZE : proxy_token->rate / 100;
    proxy_token->t = ((double)(proxy_token->token_per_time)/proxy_token->rate);

    // mutex closer to critical source
    pthread_mutex_init(&proxy_token->mutex,NULL);
    // create a pthread to generate token
    pthread_t ptid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    // thread token generator
    pthread_create(&ptid, &attr, token_generator, (void*)proxy_token);
    // char is 1 byte
```



#### 产生令牌

这里所用来产生令牌的函数，与token bucket算法描述上做了一些修改。主要的修改就是不再是`1/rate`就产生一个token，而是在一定时间内产生一些token，通过`token_per_time`我每次最少增加的token，以及产生这么多token需要的时间`t`，此处我所采用每次产生token的数目为

```c
proxy_token->token_per_time = proxy_token->rate >= MAXSIZE*100 ? MAXSIZE : proxy_token->rate / 100;
```

大约是每秒所能传送字节数的1/100。之所以这样子做，是为了避免反复读取临界区变量，获取互斥锁所需过于频繁，导致系统开销过大，以及多余时间的花费。同时，在这里我定义了计时函数，会把获取互斥锁所花费的时间，通过相应token的量补回去。这样子修改之后，就可以降低访问临界区资源的次数，而且根据速度来设置每次放token的量，也会使传输更均匀，封包不用等很长时间才可以送一次。

需要主要的一点，在使用多线程的时候，需要将线程与线程进行手动的分离使用`pthread_detach`函数，因为默认进程产生了一个线程与原始线程是阻塞关系的。

```c
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
```

#### 消耗令牌

在消耗令牌的过程中，我把它包装成了一个阻塞式的write function，当能够拿到足够的token的时候就进行write，拿不到的时候就进行等待。等待的时间，大致与产生足量的token的时间差不多。这样子也避免了长时间占有互斥锁的问题。原先，在设计的过程中，我没有加入等待的时间，所以容易出现一个问题，如果有一方访问互斥锁的速度过快，另一方就很难拿到互斥锁，会导致阻塞而影响了准确的限速。

```c
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
```

### 应用令牌桶

在设计完成令牌桶之后，就可以在上传和下载的过程中应用上这个算法了。

这里所采用的方法是`selcect`函数，使用这个函数来复用I/O接口。当哪个client socket活跃就从那读取数据，当server socket活跃就从那里读取数据。

初始化

```c
// initialize select vars
FD_ZERO(&allset);
FD_SET(clifd, &allset);
FD_SET(serfd, &allset);
rset = allset;
maxfdp1 = max(clifd, serfd) + 1;

// select descriptor,I/O multiplexer
nready = select(maxfdp1, &rset, NULL, NULL, NULL);
```

下面以从client读取数据到server为例：

```c
if (FD_ISSET(clifd, &rset)) {
    memset(buffer, '\0', MAXSIZE);
    // read from TCP recv buffer 
    if ((byte_num = read(clifd, buffer, proxy_token->token_per_time)) <= 0) {
        printf("[!] Client terminated the connection.\n");
        break;
    }

    // blocking write to TCP send buffer
    if (rate_control_write(proxy_token, serfd, buffer, byte_num) != 0) {
        printf("[x] Write fail server in rate controling write\n");
        break;
    }
}
```

只要调用所设计的token.h中，消耗token的write funciton既可以像读写文件一样来进行处理了。使得框架的设计变得更加简单。

## 框架修改

* 在原先基础的框架之上，我修复了一个原有的bug。详细信息在github的[issure#1](https://github.com/Kinpzz/FTP-Proxy/issues/1)可以看到分析过程和解决过程。主要解决了在通过fork创建数据通路的时候，由于原先框架会使得的连线未建立好就发送命令给client，client发起连接请求而导致错误。我在这里重新调整了发送的顺序，成功解决了这个问题。
* 增加多线程方式，如上述所描述，增加了多线程的方式来运作。
* 延迟发送226信息，更改了Proxy转送226的时间，在传输完毕之后再传送226，即修改相应速度，避免了数据在proxy还未传完就发送226给client。[issure#2](https://github.com/Kinpzz/FTP-Proxy/issues/2)

## 问题发现及解决

在这次实做的过程中，主要遇到了几大困难。

第一点，就是令牌桶多线程产生的问题，由于临界区资源访问速度不同，容易导致一方饥饿，一直无法消费令牌，或者产生令牌，就无从达到传输的目的了。解决办法就是通过将他们的速度修改到相近。

第二点，就是缓存区队列设计问题，原先只是把socket当成TCP的API来使用，没有了解它其实是有自己的缓存区域的，通过自己再设计一个队列来暂存这些数据，其实遇到了很大的问题，储存之后又如何消费令牌，涉及到了许多共享资源的问题。后来在了解了socket的设计之后发现，其实本身就自带缓存区，解决了很多共享资源设计上的复杂度。

第三点，

* 如[issure#1](https://github.com/Kinpzz/FTP-Proxy/issues/1)所描述的，框架本身存在的bug，导致客户端连接失败，需要进程重连的问题。也得到了修复。详细可进issue查看解决过程，篇幅较长，这里就不再一一赘述。
* 如[issure#2](https://github.com/Kinpzz/FTP-Proxy/issues/2)所描述的，以25KB/sec上传存在的响应超时的问题，以及如何通过`setsockopt`来进行调整缓存区的大小来解决这个问题。
* 如[issure#3](https://github.com/Kinpzz/FTP-Proxy/issues/3)所描述的，以过快的速度下载一个过小的文件，所存在的平均速度的问题。这主要是由于数据通路和命令通路不同步，以及如何通过延迟226 response code来解决这个问题。

## 测试结果

* 150KB/sec 上传测试

![](https://ws2.sinaimg.cn/large/6177e8b1gw1fbqfvngx5kj20uu05641o.jpg)

* 100KB/sec 上传测试

![](https://ws3.sinaimg.cn/large/6177e8b1gw1fbqgerumfsj20v803ywh5.jpg)

* 100KB/sec 下载测试

![](https://ws1.sinaimg.cn/large/6177e8b1gw1fbqgeijwiij20zo04o41o.jpg)

* 75KB/sec 下载测试

![](https://ws4.sinaimg.cn/large/6177e8b1gw1fbqfw00zfpj20za03yju5.jpg)

* 25KB/sec 上传测试

![](https://ws2.sinaimg.cn/large/6177e8b1gw1fbqgdxyu0lj20vo03wtbf.jpg)

