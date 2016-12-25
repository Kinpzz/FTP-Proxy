FTP Proxy Template
==
NTHU ITCN Final Project Code exmaple

Simple Guildline
==

- Read this code, know how it does and then write what you learn on your report
- Add delay function to control download and upload rate. User can assign upload or download rate when a proxy begins running
- You must not share what you do with other teams, or you will get zero.
- If you can improve this code and write what you do in report, you will get bonus.

Download code
==

**With simple browser**

You can click `Download ZIP` to download a zip file directly

**With Git**

1. install Git on your system.
2. open command prompt
3. input `git clone https://github.com/HSNL-TAs/2016-ITCN-FTP-Proxy-Template.git`

Running
==
```sh
$ gcc ftp_proxy.c -o proxy
$ ./proxy <ProxyIP> <ProxyPort> <Rate>
```
Please note that your proxy should control the transmission rate by given `<Rate>`.    
You can access the parameter `Rate` through `argv[3]`.

Question?
==
Feel free to ask questions [here](https://github.com/HSNL-TAs/2016-ITCN-FTP-Proxy-Template/issues) or on the ilms forum

Contributor
==
[z58085111](https://github.com/z58085111) and  [lockys](https://github.com/lockys)
