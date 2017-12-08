---
layout: post
title:  "Redis-CheatSheet"
date:   2017-12-07 21:00:00 +0800
categories: redis
---

# Redis CheatSheet

## 简介

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
* 每隔2秒，每隔哨兵节点会向数据节点的__sentinel__:hello频道发送该哨兵节点对主节点的判断，该频道也用于哨兵节点交换主节点的状态，作为客观选举和领导者选举的依据
* 每隔1秒，每个哨兵节点会向主节点，从节点，哨兵节点发送一条ping命令做一次心跳检测，来确认这些节点是否可达。


#### 主观下线与客观下线

#### 哨兵主节点选举

Raft算法简述：

1. 每个哨兵节点都想成为领导者，会向其它哨兵节点发送sentinel is-master-down-by-addr 命令，要求将自己设置为领导者
2. 每个收到sentinel is-master-down-by-addr命令的哨兵节点，如果没有收到过其它节点的sentinel is-master-down-by-addr命令，将同意其成为领导者，否则拒绝
3. 如果该哨兵节点发现字节的票数大于max(quorum, num(sentinel)/2+1)， 那么它将成为领导者
4. 如果此过程没有选举出领导者，将进入下一次选举。

## 集群

### 数据分区理论

1. 节点取余
2. 一致性哈希分区
3. 虚拟槽分区

### redis数据分区

slot = CRC16(key) & 16383

### 节点通信

#### Gossip消息

* ping
* pong
* meet
* fail

#### 故障转移

##### 主观下线

##### 客观下线

##### 资格审查

检查最后与主节点断线时间，判断是否有资格成为替换故障的主节点。<br/>

#### 选举投票

#### 替换主节点
