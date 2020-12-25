# MyWebServer

Linux环境C++并发Web服务器, 通过实际操作学习网络编程, 贯穿后端开发相关知识.

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

对代码进行了一些重构, 添加epoller类, 封装addfd等操作, 修改webserver和http_conn的实现. 目前htpp_conn中的modfd存在耦合问题, 难以移除.

# 参考文献
```
@book{游双2013Linux,
  title={Linux高性能服务器编程},
  author={游双},
  publisher={机械工业出版社},
  year={2013},
}
```
