# Final Project Specification
## Description of this project

In this project, what your team are going to do is to implement a FTP proxy with the ability to **control transmission rate**. Your team should brainstorm together to come up with a approach (You could think about the approaches teacher metioned in class).

Your approach will be **graded by the ranking performance** of your own FTP proxy compared to other teams’s performance. Therefore, if you want to get higher score, try to improve the performance of your FTP proxy to the best as possible as you can.


## Architecture and Environment
Three main components:
- **FTP server**
  - We provide a FTP server where you can upload and download files.
- **FTP proxy**
  - Control download and upload rate between client and server.
- **FTP client**
  - Client will be any kind of FTP clients such as Filezilla, CuteFTP. It depends on you.

<br>

![](https://raw.githubusercontent.com/HSNL-TAs/2016-ITCN-Final-Spec/master/img/ftp_proxy_scenario.png)

### FTP Server
Information about our FTP server as below
- IP address and port: `140.114.71.159:8740`
- User/Password: `lab/lab`
- Please use **Passive mode** to connect to the server.
- **FTP without TLS** (no encryption).


### FTP Proxy
TAs will provide an incomplete sample code of FTP proxy. You must trace the code, try to figure out how the proxy works and **finish the incomplete part**. You can run **localhost (127.0.0.1) proxy** on your own Unix-like or Linux machine.

Template code of proxy: [FTP-Proxy](https://github.com/HSNL-TAs/2016-ITCN-FTP-Proxy-Template)

### FTP Client
If you have run up a proxy on localhost with port 8888 (`127.0.0.1:8888`) and want to use Filezilla to connect to your proxy, following are setting examples:

![](https://raw.githubusercontent.com/HSNL-TAs/2016-ITCN-Final-Spec/master/img/ftp_setting.png)
![](https://raw.githubusercontent.com/HSNL-TAs/2016-ITCN-Final-Spec/master/img/pasv_mode.png)

## FTP Proxy Specfication
- FTP Proxy must be written in C/C++. Any other languages will not be accecpted.
- Following configurations can be specify with the command line interface when the proxy begins.
  - Proxy IP address and port.
  - Downloading rate.
  - Uploading rate.
- FTP proxy can transfer requests from a cilent to specificed FTP server and from specificed FTP server to a client
- FTP proxy can control the rate of downloading and uploading as close as what users expect.

## Grading
- **25%** -  Your FTP proxy can meet all requrements of  FTP proxy specfication.
- **40%** - Ranking: The performance of your FTP proxy will be ranked compared to other teams. **If your proxy cannot meet all requrements of FTP proxy specfication, your will get zero in this part.**
- **35%** - Report
- **20%** - Bonus

### Ranking and Performance Defintion (40%)

#### Performance Defintion

**`Performance = | Actual average transmission rate via proxy - expected transmission rate |`**

#### Testcases

- The first 2 testcases be released before demo time. One case is for downloading rate, and another one is for uploading rate.
  - First testcase:
    - Expected **downloading** rate: **200 KBtyes/s.**
    - File: You can download the testcase file from the **download** folder in FTP.
  - Second testcase:
    - Expected **uploading** rate: **100 KBytes/s.**
    - File: Upload the testcase file (You can download it from download folder) to the **upload** folder.

- There are also 2 hidden test cases, which will be released on demo time.

- For each test case, your team have only two chance to run your proxy, TA will chose the one with the best performance. Then, your rank will be mapped to a score with following table.

  |  Rank | 1 - 5 | 6 - 10| 11 - 15 | 16 - 20 | 21 - 25 |
  | ----- | ----- | ----- | ------- | ------- | ------- |
  | Score |  100  |  90   |  80     |  70     |  60     |

Then your score in this part will be
**`40% * (average score of the first 2 testcases ) + 60% * (average score of 2 hidden testcases)`**

### Report (35%)
Your report must includes the following contents.

1. Architectire of the project. (ex: Flow of program)
1. The understanding of the proxy (ex: How the proxy works, How the proxy commuicates with the FTP server and the FTP client ...etc). Try your best to express your understanding and thought to get higher score.
1. How do you implement the approach of controling transmission rate?
1. Problems you confront and how do you solve them?
1. How to run your code and show some experimental results
1. What is the responsibility of each member?

### Extra Bonus (up to 20%)
It is possible to improve this proxy. You can re-write your own FTP proxy with the ability to control transmission rate. Also, Please describe how you implement your own proxy (architecture, flow, significant code snippets ...etc) in the report.

## Deadline and Submission
- Submission deadline: **2017/1/15 23:59.** Late submission is not accepted.
- Please compress all your code file as a **ZIP** file and upload to ilms. Also upload the report in **PDF** separately.
  - Name of the zip file: `<team_number>.zip`
  - Name of the report: `<team_number>.pdf`
- Demo time: **2017/1/16 - 2017/1/17**. We will annouce the detailed time and place later.
- **DO NOT** copy other’s code, or your team will get zero point.
- Please do some researches and start this project early.
- E-mail to TAs if you have questions about spec of final project.

## Reference and Hint
You may read the following material about FTP protocol and you may notice that there are data channels and signal channels in FTP transmission.
- https://www.ietf.org/rfc/rfc959.txt
- http://www.linuxhowtos.org/Misc/ftpmodes.htm
- http://blogs.msdn.com/b/webtopics/archive/2014/09/06/revisiting-ftp-basics.aspx


You may use a function called select() to transmit requests from a cilent to a specificed FTP server and from a specificed FTP server to a client concurrently. Some resources are listed below.
- http://www.gnu.org/software/libc/manual/html_node/Server-Example.html
- https://www.gnu.org/software/libc/manual/html_node/Waiting-for-I_002fO.html


You may use forking child processes to handle muitiple clients and handling data channels and signal channels concurently.
- http://www.chemie.fu-berlin.de/chemnet/use/info/libc/libc_23.html
