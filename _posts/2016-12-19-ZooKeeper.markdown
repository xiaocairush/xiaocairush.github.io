---
layout: post
title:  "ZooKeeper"
date:   2016-12-19 23:59:59 +0800
categories: ZooKeeper
---

# ZooKeeper: 分布式应用的分布式协调服务
ZooKeeper是一个分布式的，开源的，协调分布式应用的service. 它暴露了一组简单的原语，分布式应用可以基于这些原语可以构建并实现high level的同步服务，配置管理，分组和命名。它易编程，使用的数据模型风格与我们所熟悉的文件系统树结构非常相似。它使用java来运行并且对java和c都有良好的支持。

众所周知，要使得Coordination services（协调服务）正确是非常困难的。Coordination services有很多非常容易出错的地方，例如边界条件和死锁。ZooKeeper的动机就是减少分布式应用的复杂性，不必自己从头实现coordination services的职责。
# 设计目标
## 简单
通过一个共享的分层次的命名空间，Zookeeper允许分布式进程能够互相协调。这个命名空间由登记的数据节点组成，用zookeeper的说法，它们非常相似于文件和文件夹的关系。与典型的文件系统不同，文件系统被用来存储数据，zookeeper的数据是放在内存中的，这意味着zookeeper能够实现高吞吐和低延迟。

zookeeper的实现着重考虑并保证高效性，高可靠性，严格的顺序访问。ZooKeeper的高效意味着它可以被用在大规模的分布式系统中。可靠性能保证它不会因为一个节点的失败而失败。严格的顺序性意味着复杂的同步原语能够在客户端被非常简单的实现。
## 复杂
与zookeeper所服务的不同线程一样，zookeeper意图自身被复制在一系列的叫做ensemble的主机上。
![Zookeeper Service](https://zookeeper.apache.org/doc/trunk/images/zkservice.jpg)

图中所有的server组成了zookeeper服务，它们必须互相知道彼此。它们在内存中维护了一个镜像状态，这个镜像与transaction logs和snapshots一起被持久化。只要大多数的server是可用的，zookeeper就会是可用的。

Client 连接到一个单独的zookeeper server上。这个client维护了tcp连接，这个连接可以用来发送request，获取response，获取watch events，以及发送heart beats. 如果连到server的tcp连接断开，client会连到其它的server.
## 有序
ZooKeeper为每次update操作记录一个数字，这个数字代表了ZooKeeper事物的顺序。后续的操作可以使用这个顺序来实现high level的抽象，例如同步的语义。
## 快速
在读的工作负载为主的时候，zookeeper非常快。ZooKeeper应用运行在数以千计的机器上，在读写比例为10:1的时候它的表现是最好的。
# 数据模型和层次性的命名空间
ZooKeeper提供的命名空间有点像标准的文件系统。name由一系列被```/```分开的路径元素组成。

zookeeper中的每个node由一个path来标识。
![ZooKeeper's Hierarchical Namespace](https://zookeeper.apache.org/doc/trunk/images/zknamespace.jpg)
# 节点和临时性节点(ephemeral nodes)
与标准的文件系统不同的是，ZooKeeper命名空间中的每个节点既可以有数据也可以有子节点。它就像一个允许文件也可以同时是路径的文件系统。（zookeeper被设计用来存储协调的数据有：状态信息，配置，地址等等，所以存储在每个node中的数据是非常小的，在KB范围之内。）为了使表达清晰，当我们讨论zookeeper数据节点的时候我们用znode来表示。

znode维护了一个统计结构，为了保证cache的有效和协调update，里面包含了数据改变的版本号，访问控制列表（ACL）改变的版本号，时间戳（timestamp）的版本号。每次znode中的数据改变的时候，版本号将会增加。例如，client获取数据的时候它也会收到数据的版本号。

znode命名空间中存储的数据的读写是自动完成的。读操作获取znode的数据，写将会替代znode中所有的数据。每个节点都有一个访问控制列表（ACL）用来限制谁可以做什么。

ZooKeeper也有临时性节点(ephemeral nodes)的概念。只要创建这些znode的session是active的，这些znode就存在。当session结束的时候znode也被删除了。当你想要实现[tbd]的时候Ephemeral nodes是非常有用的。(这里的[tbd]是to be discussion的缩写，有待讨论的意思)
# Conditional updates和watches
zookeeper支持watch的概念。client可以在znode上设置一个watch。当znode改变的时候watch将会被触发（trigger）和删除。在watch被触发之后，client会收到一个数据包告诉它znode已经变了。如果client和zookeeper中某一个server的连接断开了，client将会收到一个本地的通知。这将有待讨论。
# 保证
zookeeper是非常快速和简单的。尽管它的目标是成为构建更为复杂service的基础，例如同步，它也提供了一系列的保证。它们是：
* 顺序一致性
* 原子性
* 单系统镜像 - 无论client连接到哪个server它们看到的视图都是相同的
* 高可用 - 一旦update被apply了，它将会从那个时间点持久化直到client重写了update
* 及时性 - 保证client看到的系统的视图在一定时间内被更新
更多方面有待讨论
# 简单的api
ZooKeeper的一个设计目标是提供简化的编程接口。它支持以下操作
```
create
	在树种的一个节点创建node
delete
	删除一个node
exists
	检查某一个位置是否存在一个node
get data
	从node中读取数据
set data
	向node中写入数据
get children
	获取一个节点所有的子节点
sync
	等待数据复制完成
```
对于在这些方面的深度讨论，以及如何实现high level的操作，仍旧有待讨论。
# 实现
[zookeeper组件](https://zookeeper.apache.org/doc/trunk/zookeeperOver.html#fg_zkComponents)				 显示了high level的zookeeper服务的组件。除了请求处理器（request processor）之外，每个组成zookeeper service的server都复制每一份组件自己使用。
![zookeeper组件](https://zookeeper.apache.org/doc/trunk/images/zkcomponents.jpg)

replicated database是一个维护整个数据树的内存数据库。为了可恢复，update被记录在磁盘上，write在被应用到内存数据库之前被序列化到磁盘上。

每个zookeeper server 服务多个client。client只连接到一个要提交request的server上。read请求从每个server内存数据库中的副本中拿数据。改变service状态的请求以及写请求经过协商协议来处理。

作为协商协议的一部分，所有来自client的write请求都被转发到一个单独的叫做leader的server上。zookeeper中剩余的server都叫做follower，他们只能接受leader的调配。消息层负责替代失败的leader和同步follower与leader一致。

zookeeper使用结点的自动消息协议。因为消息层是自动的，zookeeper能够保证本地备份不会产生误差。当leader接收到write请求之后，它计算系统的状态是什么，什么时候应用这个写请求将它转换成事务来捕捉这个新的状态。
# 用法
ZooKeeper的编程接口有意设计得很简单。使用它你可以实现high level的命令操作，例如同步的语义，组关系，权限，等等。Some distributed applications have used it to: [tbd: add uses from white paper and video presentation.] For more information, see [tbd]
# 效率
zookeeper被设计得很高效。但确实如此吗？Yahoo!研究中心的zookeeper开发团队说是的。当应用程序读的数量远超过写操作的数量时，它是非常高效的，因为写操作会同步所有server的状态。（在coordination service中这种case是非常典型的）
![zookeeper吞吐率随着读写比例的变化](https://zookeeper.apache.org/doc/trunk/images/zkperfRW-3.2.jpg)

这是ZooKeeper release 3.2的一张吞吐率的图，它运行的server是dual 2Ghz Xeon 和two SATA 15K RPM drives。一个驱动器被用作专用zookeeper记录设备。zookeeper的快照被写到系统的驱动器中。1k read和write请求。Servers表明了ensemble的大小，也就是zookeeper中server的数量。大约有30台其它的服务器来模仿client。zookeeper ensemble被配置成不允许来自client的连接。

> 与3.1相比3.2版本中读写的performance提高了两倍
	
基准点也表明它是可信的。下面显示了怎样针对不同的失败做发布。图中标记的事件是：
1. follower的失败和恢复
2. 不同的follower的失败和恢复
3. leader的恢复
4. 两个follower的失败和恢复
5. 另一个leader的失败
# 可靠
为了显示系统的行为，在我们运行7台机器组成的zookeeper service的时候失败被注入进来。我们使用与之前相同的基准点运行，但是这次我们使write的百分比固定在30%，这个比率是我们期望负载商定的比例。
![error时的可信度](https://zookeeper.apache.org/doc/trunk/images/zkperfreliability.jpg)

这张图中有一些重要的注意点。首先，如果follower失败并立即恢复，zookeeper仍能够维护高吞吐。但可能更重要的是，leader选举算法能够允许系统足够快地恢复，这能够阻止吞吐量的降低。我们可以看到，zookeeper使用少于200ms的时间选举一个新的leader。第三点，当follower恢复的时候，zookeeper能够再次提高处理请求的吞吐量。
# zookeeper项目
zookeeper被成功的用在许多的工业实践中。Yahoo!使用它作为Yahoo! Message Broker的协调和失败恢复服务，Yahoo! Message Broker是一个大规模的发布-订阅系统，管理着上千的topic备份和数据传输。Fetching Service for Yahoo! crawler也使用它，也是拿来管理失败恢复。Yahoo!的一些广告系统也使用zookeeper来实现高可靠的service。

欢迎任何用户和开发者加入到社区贡献你们的专业知识。从http://zookeeper.apache.org/能够获得更多信息。
