---
layout: post
title:  "Redis-CheatSheet"
date:   2017-12-07 21:00:00 +0800
categories: redis
---

# Redis CheatSheet

## 简介

清单来自[《Redis开发与运维》](https://yuedu.163.com/source/e99b02fd21a44345bee98821e64b2af6_4)

### 特点

* 快；
* KV存储；
* 功能丰富：键过期，发布订阅，Lua，事务，PipeLine
* 单线程
* 持久化
* 主从复制
* 高可用  (Sentinel)
* 分布式（Cluster, version > 3.0）

### 使用场景

可以做：
* 缓存
* 计数
* 共享session
* 限速
* 标签（集合）
* 排行榜（有序集合）

由于放在内存中，数据量不能太大

## API

```
// 全局命令
keys *
dbsize
exists key
del key
expire key seconds
type key
object encoding key
select dbInx

// 字符串
strlen key

// 哈希
hset key field value
hget key field

// 列表
rpush key value
linsert key before|after pivot value
lrange key start_inx end_inx
blpop

// 集合
sadd key e1 e2 ...
srem key e1	//删除操作
sinter key1 key2 ... // 多个集合相交

// 有序集合
zadd key score member [score member...]

// pipeline
mset

// 事务
multi
...
exec

// 脚本管理
script load
script flush
script kill

// Bitmaps
setbit key offset value
getbit key offset

// HyperLoglog
pfadd key e1 [e2 ...]
pfcount key1 [key2 ...]

// 发布订阅
publish channel message
subscribe channel [channel2 ...]

// CEO
geoadd key longitude latitude member
geopos key member // 获取member的经纬度

// 统计与排查问题
slowlog get {n} // 获取最近n条慢查询
redis-cli -h {ip} -p {port} bigkeys // 获取大对象 
redis-cli -h {ip} -p {port} --stat // 统计redis使用情况
```

### 数据结构与编码

1. 字符串:

* int 8字节
* embstr  < 39字节
* raw 

2. 哈希：

* ziplist 元素个数 < 512,  所有值 < 64字节，元素连续存储，省空间
* hashtable

3. 列表：

* ziplist entry个数<512个, 所有value < 64字节
* linkedlist

4. 集合：

* intset 整数集合，元素个数<512
* hashtable

5. 有序集合

* ziplist 元素个数<128个, 所有value<64字节
* skiplist

## 实用功能

### 慢查询分析

slowlog-log-slower-than 单位是微秒， 10 毫秒
slowlog-max-len 慢查询条数上限， 1000以上

### Bitmaps

一亿用户，当日是否访问过网站

### HyperLoglog

有可能有误差

### 发布订阅

### CEO

地理位置信息范围计算

## 持久化

### RDB

## 触发机制

save
bgsave

运作流程略。

优点：

* 使用LZF算法压缩，紧凑，适合做备份，每XX小时备份一次，用于灾难恢复
* 加载速度快于AOF

缺点：

* 无法实时持久化/秒级持久化，因为fork属于重量级操作
* 兼容性问题

### AOF

运作流程：

#### 命令写入(append)

* 写入aof_buf，减小磁盘负载
* 使文本协议，因为兼容性好，aof一般是追加操作，可读性好，方便直接修改和管理

#### 文件同步(sync)

同步策略：

* always 每条命令写入缓冲区，立即fsync。
* everysec 命令写入缓冲区后write。单独线程每秒fsync
* no 写入aof_buf后调用write, 不对aof_buf进行fsync

常用everysec, 用于平衡性能和数据安全性，在磁盘繁忙的时候，定时fsync的线程会造成主线程阻塞，阻塞流程如下：

* 主线程写入AOF缓冲区
* AOF线程每秒执行一次fsync
* 主线程对比上次AOF fsync成功的时间，如果距上次成功时间在2秒内就不阻塞，否则将阻塞直到同步操作完成。（因此everysec最多可能丢失2秒数据）

#### 文件重写(rewrite)

手动触发：bgrewriteaof
自动触发：auto-aof-rewrite-min-size,  auto-aof-rewrite-percentage（当前文件体积/上一次重写后体积）

#### 重启加载(load)

开启aof的情况下，优先加载aof

## 复制

### 配置

建立复制 slaveof {masterHost}
断开复制 slaveof no one

### 拓扑结构

* 一主一从（简单，不安全）
* 一主多从（可利用多个节点实现读写分离，加重主节点负载，影响稳定性）
* 树状主从（有效降低主节点负载和需要传送给从节点的数据量）

### 数据同步

psync 命令用于同步，分为全量复制和部分复制，可以用info replication来查看复制偏移量以及复制积压缓冲区


### 心跳机制

主从节点建立复制后，维护长连接并彼此发送心跳命令：

* 主节点连接状态为flag=M, 从节点连接状态为flag=S
* 主节点每隔10秒发送ping，用于判断从节点是否存活
* 从节点每秒发送replconf ack {offset} 命令上报当前复制偏移量


## 哨兵(Sentinel)

### 基本概念

用于监控节点，故障转移

两种启动方法：

```
redis-sentinel XX.conf
redis-server XX.conf --sentinel
```

### 实现原理

#### 三个定时任务

* 每隔10秒，每个哨兵节点都向主节点执行info命令，获取最新的拓扑结构
* 每隔2秒，每隔哨兵节点会向数据节点的__sentinel__:hello频道发送该哨兵节点对主节点的判断，该频道也用于哨兵节点交换主节点的状态，作为客观下线和领导者选举的依据
* 每隔1秒，每个哨兵节点会向主节点，从节点，哨兵节点发送一条ping命令做一次心跳检测，来确认这些节点是否可达。


#### 主观下线与客观下线

#### 哨兵主节点选举

Raft算法简述：

1. 每个哨兵节点都想成为领导者，会向其它哨兵节点发送sentinel is-master-down-by-addr 命令，要求将自己设置为领导者
2. 每个收到sentinel is-master-down-by-addr命令的哨兵节点，如果没有收到过其它节点的sentinel is-master-down-by-addr命令，将同意其成为领导者，否则拒绝
3. 如果该哨兵节点发现字节的票数大于max(quorum, num(sentinel)/2+1)， 那么它将成为领导者
4. 如果此过程没有选举出领导者，将进入下一次选举。

## 集群

### 数据分布

#### 数据分区理论

1. 节点取余
2. 一致性哈希分区(常用于缓存场景)
3. 虚拟槽分区

#### redis数据分区

slot = CRC16(key) & 16383

#### 功能限制

1. 批量操作如mset，mget只支持具有相同slot值的key执行操作
2. 只支持多key在同一节点上的事务操作
3. key作为数据分区的最小粒度，因此不能将一个大的键值对象如hash，list映射到不同的节点
4. 不支持多数据库空间。单机最多可使用16个数据库，集群模式只能使用一个数据库空间，即db 0
5. 复制结构只支持一层，不支持嵌套树状复制结构

### 搭建集群

### 准备节点

配置：

```
port 6379
# 开启集群模式
cluster-enabled yes
# 节点超时时间，单位毫秒
cluster-node-timeout 15000
# 集群内部配置文件
cluster-config-file "nodes-6379.conf"
```

启动节点：

```
redis-server conf/redis-6379.conf
```

### 节点握手

```
cluster meet 127.0.0.1 6381
cluster nodes
cluster info
```

### 分配槽

```
redis-cli -h 127.0.0.1 -p 6379 cluster addslots {0...5461}
cluster replicate {node run id}
```

### 节点通信

#### Gossip消息

* ping
* pong
* meet
* fail

消息头：clusterMsg ; 消息体：clusterMsgData

#### 节点选择

> 每秒随机选取5个节点，向其中最久没有通信的节点发送ping消息。每100毫秒扫描本地节点列表，向最近一次接收pong消息的时间大于cluster_node_timeout/2的节点发送ping消息。

注意: cluster_node_timeout参数对消息发送的节点数影响非常大。该参数会影响消息交换的频率从而影响故障转移，槽信息更新，新节点发现速度。因此需要根据业务容忍程度和资源消耗进行平衡。

#### 消息数据量

> 消息头主要占用空间的字段是myslots[cluster_slots/8]，占用2KB;
消息体会携带一定数量的其它节点信息用于消息交换，
消息体中其它节点的数量=max(3, floor(cluster_node_size/10))。

#### 故障转移

##### 主观下线

每个节点定期向其它节点发送ping消息，若在cluster_node_timeout之内没收到pong消息，则把该节点标记为主观下线状态。

每个节点内的clusterState结构都需要保存其它节点的信息。

##### 客观下线

当某个节点判断另一个节点主观下线后，相应的节点状态信息会在对集群内传播。ping/pong消息体会携带集群1/10的其它节点状态信息，当接受节点发现消息体中含有主观下线的节点时，会在本地找到故障节点的ClusterNode结构，保存到该结构中下线报告链表中。<br/>

当半数以上持有槽点主节点都标记某个节点是主观下线时，触发客观下线流程。要求半数以上是为了应对网络分区等原因造成的集群分隔情况，被分割的小集群因为无法完成从主观下线到客观下线这一过程，从而防止小集群完成故障转移后继续对外提供服务。

##### 资格审查

如果下线节点是持有槽的主节点则需要在它的从节点中挑选出一个替换它。<br/>
检查最后与主节点断线时间，判断是否有资格成为替换故障的主节点。<br/>
如果从节点与主节点断线时间超过cluster-node-time * cluser-slave-validity-factor，则当前从节点不具备故障转移资格。

#### 准备选举时间

 当前从节点具备资格后，更新触发故障选举的时间，只有到达改时间后才能执行后续流程。(延迟触发机制，可以通过对多个从节点使用不同的延迟选举时间来支持优先级问题。复制偏移量越大优先级越高，延迟时间越小。)

#### 发起选举

通过配置纪元（configEpoch）来保证每个从节点在一个配置纪元内只能发起一次选举消息。该选举消息消息内容如同ping消息，只是type类型为FAILOVER_AUTH_REQUEST。同时标记该节点在该配置纪元内已发送过消息状态。

#### 选举投票

持有槽点主节点在一个配置纪元内只有唯一的一张选票, 当接到第一个请求投票的从节点消息时回复FAILOVER_AUTH_REQUEST消息作为投票，之后相同纪元内其它从节点的选举消息将忽略。

#### 替换主节点

1. 当前从节点取消复制变为
2. 撤销故障节点复制的槽，并将这些槽委派给自己
3. 向集群内广播pong消息, 通知其它节点自己已成为主节点


### 资料

[作者 付磊博客](http://carlosfu.iteye.com) <br/>
[作者 张益军博客](http://hot66hot.iteye.com)
