# cpp-tbox

#### 介绍
cpp-tbox，全称: C++ Treasure Box，C++开发百宝箱，是基于事件的服务型应用开发库。

#### 适用环境

- Linux 环境，主要是针对服务型应用的；
- C++11 以上。

#### 模块介绍

- base，基础库，含日志打印、常用工具等；
- util，工具库，在业务代码中可能会用到的库，含`PidFile`,`Serializer`,`ArgumentParser`,`StateMachine`,`TimeCounter`等；
- event，事件库，实现Fd,Timer,Signal三种事件驱动；
- log，日志输出库，实现了终端、syslog、文件形式的日志输出；
- eventx，事件扩展库，含 ThreadPool 线程池模块，专用于处理阻塞性事务；TimerPool 定时器池模块；
- network，网络库，实现了串口、终端、UDP、TCP 通信模块；
- http，HTTP库，在network的基础上实现了HTTP的Server与Client模块；
- coroutine，协程库，众所周知，异步框架不方便处理顺序性业务，协程弥补之；
- mqtt，MQTT客户端库；
- terminal, 终端，类似shell的命令终端，可实现运行时与程序进行命令交互；
- main，应用程序框架，实现了完备的程序启动流程与框架，让开发者只需关心业务代码；
- alarm，闹钟模块，实现了4种常用的闹钟：CRON闹钟、单次闹钟、星期循环闹钟、工作日闹钟；
- flow，动作模块，解决异步模式下的顺序性动作执行困难问题；

#### 外部库依赖

| 库名 | 依赖模块 | 说明 | 安装方法 |
|:----:|:--------:|:----:|:--------:|
| googletest | 所有模块 | 单元测试用，如果不进行单元测试可忽略 | sudo apt install google-mock |
| libevent | event | 默认不依赖，在event/config.mk中开启了WITH\_LIBEVENT时依赖 | sudo apt install libevent-dev |
| libev | event | 默认不依赖，在event/config.mk中开启了WITH\_LIBEV时依赖 | sudo apt install libev-dev |
| mosquitto | mqtt | MQTT库 | sudo apt install libmosquitto-dev |
| nlohmann/json | main | 作为配置数据用 | 下载[json\_fwd.hpp](https://raw.githubusercontent.com/nlohmann/json/v3.10.4/include/nlohmann/json_fwd.hpp)与[json.hpp](https://raw.githubusercontent.com/nlohmann/json/v3.10.4/single_include/nlohmann/json.hpp)到头文件目录，如：/usr/local/include/nlohmann/ |

**安装命令**

| 系统 | 安装命令 |
|:----:|:------:|
| Ubuntu/Debian | `apt install -y g++ make google-mock libevent-dev libev-dev libmosquitto-dev` |

**如果构建**

进入到cpp-tbox的顶层目录，执行命令:  
```
STAGING_DIR=$HOME/.local make modules RELEASE=1
```

完成之后，所有的头文件导出到 `$HOME/.local/include/`，所有的库文件输出到 `$HOME/.local/lib/`。
如果你没有指定 `STAGING_DIR` 参数，它默认为 `.staging`。

在你自己工程的 Makefile 中，你需要添加以下的内容:
```
CXXFLAGS += -I$(HOME)/.local/include
LDFLAGS += -L$(HOME)/.local/lib -ltbox_xxxxx
```
然后就可以使用tbox的组件了。

#### 模块间依赖

![](http://assets.processon.com/chart_image/6227efafe401fd18bcfc83e8.png)

#### 模块裁减

打开 config.mk 文件，将不需要模块对应 `app_y += xxx` 屏蔽即可，但要注意模块间的依赖性。

#### 未来规划

- 实现IPv6
- 实现TLS的支持

