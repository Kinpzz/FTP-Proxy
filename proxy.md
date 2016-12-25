线程和进程十分相似，不同的只是线程比进程小。首先，线程采用了多个线程可共享资源的设计思想；例如，它们的操作大部分都是在同一地址空间进行的。其次，从一个线程切换到另一线程所花费的代价比进程低。再次，进程本身的信息在内存中占用的空间比线程大，因此线程更能允分地利用内存。

https://www.ibm.com/developerworks/cn/linux/l-pthred/

使用fork来创建子进程，子进程会复制一份父进程的变量而使用pthread的话不会复制

fork的时候只复制当前线程到子进程

连接上FTP会创建一个client,长连接 

上传文件会创建一个client，停止上传之后这个client会终止

同时上传两个两件就会创建两个client一个client对应一个或2个socket

下载一个文件对应了一个client，这个client会创建两个端口一个命令端口，一个数据端口

下载文件会创建一个client

如果使用被动模式，通常服务器端会返回一个端口号。客户端需要用另开一个 Socket 来连接这个端口，然后我们可根据操作来发送命令，数据会通过新开的一个端口传输。

https://www.ibm.com/developerworks/cn/linux/l-cn-socketftp/



已知bug：

1. 切换目录发送PASV导致bug，只能等待重连恢复 
   * 上下文：出现在开着目录重连，还有第一次切换目录的时候，多次下东	
   * 原因：filezilla 发送的连接请求被拒绝
   * 降低下载频率，或等待重连
   * socket没有关闭，下载中断之后

成功：server : 227 8740
[-] Waiting for data connection!
client : RETR 10MB_testcase

失败：server : 227 8740
client : RETR 10MB_testcase

[-] Waiting for data connection!

难定位：有时候出现，有时候不出现

原因：可能是server还没有建好，fork后的父子进程的顺序是随机的

父进程，和子进程执行的顺序不一定。

如果父进程先执行了，在server还没有建好的时候client就发送了请求，导致了错误

解决在子进程里创建完server再发送给client

2.PASV

传完一个文件之后socket没有关闭

proxy_func后面加上close connfd