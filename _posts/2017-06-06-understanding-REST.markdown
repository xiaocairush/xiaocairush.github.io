---
layout: post
title:  "understanding REST"
date:   2017-06-06 21:00:00 +0800
categories: REST
---

# 理解REST

翻译自：https://spring.io/understanding/REST <br/>
维基百科：https://zh.wikipedia.org/wiki/REST <br/>
REST (Representational State Transfer)是由[Roy Fielding](https://en.wikipedia.org/wiki/Roy_Fielding)2000年发表的[博士论文](https://www.ics.uci.edu/~fielding/pubs/dissertation/top.htm)提出的

。REST并非标准，而是有一系列约束组成，例如无状态，C/S以及风格一致的接口。REST与HTTP没有必然的联系。

## REST的原则

- **Resources**暴露容易理解的URI路径结构
- **Representations**把JSON或XML转换成对象或者属性
- **Messages**显示使用HTTP的方法（例如GET, POST, PUT, DELETE）
- **Stateless**交互不在服务器上存储请求的客户端上下文。状态的依赖会限制可扩展性，客户端保存会话状态。

## HTTP methods

使用HTTP methods来将CRUD (create, retrieve, update, delete) 操作映射到HTTP请求

## GET

获取信息。GET必须是安全和幂等的，就是说无论使用相同的参数GET多少次，结果都是相同的。GET可能会有负面影响，但不应该是用户所期望的，所以对于系统的操作来说并不严格。请求也可以是局部的或有条件的

。
获取一个ID是1的地址：

```
GET /addresses/1
```

## POST

请求位于URI的资源使用提供的entity做一些事情。通常用来创建entity，也可以用来更新entity。
创建一个新的地址

```
POST /addresses
```

## PUT

在URI除存储entity。PUT可以创建或者更新entity。PUT请求是幂等的。幂等是PUT和POST之间的主要区别。
更改ID是1的地址

```
PUT /addresses/1
```

> PUT替代存在的entity。如果只提供数据元素的子集，未提供的数据元素将用empty或者null来替代。

## PATCH

只更新处于URI位置的entity的特定的域。PATCH请求是幂等的，幂等是PATCH与POST的主要区别。

```
PATCH /addresses/1
```

## DELETE

请求移除资源；然而资源不必立即被移除，它可是是一个异步的或运行时间很长的请求。

删除ID是1的地址

```
DELETE /addresses/1
```

## HTTP 状态码

状态码代表了HTTP请求的结果

- 1XX - informational
- 2XX - success
- 3XX - redirection
- 4XX - client error
- 5XX - server error

## Media类型

**Accept** 和 **Content-Type** HTTP headers 可以用来描述HTTP请求中发送或者请求的内容。如果他请求的是JSON形式的response，客户端可能会将 **Accept** 设置成**application/json** . 相反, 在发送数

据的时候, 将 **Content-Type**设置成**application/xml**将表示客户端请求正在发送的数据是XML形式。