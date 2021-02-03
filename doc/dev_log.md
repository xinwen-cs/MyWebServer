# 开发日志
2020.12.25

利用书中15章源码搭建代码原型, 使用C++简单封装. 添加clang-format配置文件格式化代码. 修复了书中ET模式下accept函数的不正确写法(会导致epoll_wait阻塞), accept时需要用while处理所有同时到来的连接请求. 注释了原本代码中socket的linger选项(使用WebBench短连接测试时不正常). 压测结果如下(2核2G1M云服务器):
```sh
./webbench http://wenxin.site/index.html -t 10 -c 4000 -2
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET /index.html HTTP/1.1
User-Agent: WebBench 1.5
Host: wenxin.site
Connection: close


Runing info: 4000 clients, running 10 sec.

Speed=69660 pages/min, 118066 bytes/sec.
Requests: 11610 susceed, 0 failed.
```

2020.12.25

对代码进行了一些重构, 添加epoller类, 封装addfd等操作, 修改webserver和http_conn的实现. 目前http_conn中的modfd存在耦合问题, 难以移除.

2020.12.28

对http类进行了重构, 新建http_request和http_response两个类, 前者仍有bug, modfd依然耦合.

2020.12.29

修复http_response的bug, 每次GET/BAD_REQUEST后需要将m_checked_idx和m_check_state复位. 将listenfd设置为重用, 方便调试.

2020.12.30

修复了http_conn::write传输大文件时的bug.

2020.12.31

修复了keep-alive模式不能正常工作的bug.

2020.12.31

修复了编译时会产生warning的语句

2020.12.31

将m_epollfd更改为epoller, 更加简洁

2021.1.1

添加升序链表定时器处理非活动连接, 代码需要进一步封装简化

2020.1.2

添加阻塞队列, 异步日志, 修改条件变量封装类

2020.1.29

对定时器模块进行部分重构

2020.2.1

新建doc目录, 将开发日志迁移到doc/dev_log.md中

2020.2.1

优化Epoller类, 使用vector封装事件

2020.2.1

使用智能指针优化WebServer类

2020.2.1

将http_conn::m_user_count设置为atomic<int>

2020.2.1

优化http_response结构

2020.2.2

开了一个8C8G的云服务器进行本地测试, 关闭Log, 添加TCP_NODELAY功能
```sh
./webbench -c 1000 -t 60 http://127.0.0.1:80/
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET / HTTP/1.0
User-Agent: WebBench 1.5
Host: 127.0.0.1


Runing info: 1000 clients, running 60 sec.

Speed=2255092 pages/min, 1127546 bytes/sec.
Requests: 2255092 susceed, 0 failed.
```

2020.2.2

优化Log性能

2020.2.3
添加buffer类, 并使用它优化http类结构
