
FTP Proxy
==
Running
==
```sh
$ make clean
$ make
$ ./proxy <ProxyIP> <ProxyPort> <Rate>
```
Please note that your proxy should control the transmission rate by given `<Rate>`.  
You can access the parameter `Rate` through `argv[3]`.

