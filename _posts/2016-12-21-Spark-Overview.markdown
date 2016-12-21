---
layout: post
title:  "Spark Overview(翻译)"
date:   2016-12-21 21:00:00 +0800
categories: Spark
---

# Spark Overview(翻译)

Apache Spark是一个快速并且通用的集群计算系统。它提供了high level的Java, Scala, Python 和 R 语言的api，以及一个经过优化的支持通用图表的引擎。它也支持非常丰富的工具库，例如[Spark SQL](http://spark.apache.org/docs/latest/sql-programming-guide.html)用于sql和结构化数据的处理，[MLlib](http://spark.apache.org/docs/latest/ml-guide.html)用于机器学习，[GraphX](http://spark.apache.org/docs/latest/graphx-programming-guide.html)用于处理图，以及[Spark Streaming](http://spark.apache.org/docs/latest/streaming-programming-guide.html)

## 下载

从[下载页](http://spark.apache.org/downloads.html)获取Spark。这个文档对应于Spark 2.0.2. Spark使用Hadoop为HDFS和YARN做的客户端库。下载的压缩文件打包了一些流行版本的Hadoop。用户也可以下载Hadoop免费版的二进制文件，并通过[参数化Spark的classpath](http://spark.apache.org/docs/latest/hadoop-provided.html)来使用任意版本的Hadoop来运行Spark

如果你想编译Spark的源码，访问[Build Spark](http://spark.apache.org/docs/latest/building-spark.html)

Spark既可以运行在Windows, Unix, Mac OS等系统中。在本地运行Spark是非常简单的--你只需要安装了java，并JAVA_HOME环境即可。

Spark能够运行在Java 7+, Python 2.6+/3.4+ 以及 R 3.1+。Spark 2.0.2提供的Scala API需要使用 Scala 2.11。这意味着你需要一个兼容Scala 2.11的版本。

## 运行例子和shell

Spark 有一系列的样例程序。Scala, Java, Python 和 R的例子在examples/src/main文件夹下面。为了运行Java或者Scala的样例程序，在Spark一级目录下使用```bin/run-example <class> [params]```(在幕后，这条命令调用了其它通用的[spark-submit 脚本](http://spark.apache.org/docs/latest/submitting-applications.html)来登陆应用程序 )。例如，

```
./bin/run-example SparkPi 10
```

你也可以通过Scala Shell来运行Spark。这是学习框架非常好的方式。

```
./bin/spark-shell --master local[2]
```

```--master```选项确定了[cluster的master的URL](http://spark.apache.org/docs/latest/submitting-applications.html#master-urls)，local在本地运行一个线程，local[N]在本地运行N个线程。你可以使用local来启动测一下这点。使用--help选项可以获得所有的选项。

Spark也提供了Python API。要在Python接受器中运行Spark，使用```bin/pyspark```

```
./bin/pyspark --master local[2]
```

从1.4开始Spark也提供了实验性的[R API](http://spark.apache.org/docs/latest/sparkr.html)（只包含了DataFrames APIs）。要在R解释器中运行Spark，使用```bin/sparkR```:

```
./bin/sparkR --master local[2]
```

也提供了R语言的样例应用程序。例如：

```
./bin/spark-submit examples/src/main/r/dataframe.R
```

## 登陆cluster

[cluster mode overview](http://spark.apache.org/docs/latest/cluster-overview.html)解释了运行cluster的关键概念。Spark可以自己运行，也可以与几个已经存在的cluster manager一起运行。当前提供了几个发布的选项：

* [独立运行模式发布](http://spark.apache.org/docs/latest/spark-standalone.html)在私有cluster上发布Spark最简单的方式
* [Apache Mesos](http://spark.apache.org/docs/latest/running-on-mesos.html)
* [Hadoop YARN](http://spark.apache.org/docs/latest/running-on-yarn.html)

## 读完这篇文档接下来去哪里

### Programming Guides:

* [快速开始](http://spark.apache.org/docs/latest/quick-start.html): 简单介绍一下Spark API;
* [Spark编程指导](http://spark.apache.org/docs/latest/programming-guide.html): 各种语言(Scala, Java, Python, R)详细的overview
* 基于Spark的模块:
	* [Spark Streaming](http://spark.apache.org/docs/latest/streaming-programming-guide.html): 实时处理数据流
	* [Spark SQL, Datasets, and DataFrames](http://spark.apache.org/docs/latest/sql-programming-guide.html): 支持结构化数据和关系查询
	* [MLlib](http://spark.apache.org/docs/latest/ml-guide.html): 内置的机器学习的库
	* [GraphX](http://spark.apache.org/docs/latest/graphx-programming-guide.html): Spark图表处理的最新的API

### API Docs:

* [Spark Scala API (Scaladoc)](http://spark.apache.org/docs/latest/api/scala/index.html#org.apache.spark.package)
* [Spark Java API (Javadoc)](http://spark.apache.org/docs/latest/api/java/index.html)
* [Spark Python API (Sphinx)](http://spark.apache.org/docs/latest/api/python/index.html)
* [Spark R API (Roxygen2)](http://spark.apache.org/docs/latest/api/R/index.html)

### 发布指导:

* [Cluster Overview](http://spark.apache.org/docs/latest/cluster-overview.html): 介绍当运行cluster的时候，你需要了解的概念和组件
* [提交应用](http://spark.apache.org/docs/latest/submitting-applications.html): 介绍打包和发布应用
* 发布模式:
	* [Amazon EC2](https://github.com/amplab/spark-ec2): 介绍一些脚本帮你在5分钟内登陆运行在EC2上的cluster
	* [独立发布模式](http://spark.apache.org/docs/latest/spark-standalone.html): 介绍不使用第三方集群管理软件快速登陆一个独立的cluster
	* [Mesos](http://spark.apache.org/docs/latest/running-on-mesos.html): 介绍发布一个使用Apache Mesos的cluster
	* [YARN](http://spark.apache.org/docs/latest/running-on-yarn.html): 介绍在Hadoop NextGen (YARN)上发布Spark

### 其它文档:

* [配置](http://spark.apache.org/docs/latest/configuration.html)
* [监控](http://spark.apache.org/docs/latest/monitoring.html): 监控应用程序的行为
* [通信](http://spark.apache.org/docs/latest/tuning.html): 优化性能和内存占用的最佳实践
* [工作调度](http://spark.apache.org/docs/latest/job-scheduling.html): 在Spark应用之间或者应用内部调度资源
* [安全性](http://spark.apache.org/docs/latest/security.html)
* [硬件准备](http://spark.apache.org/docs/latest/hardware-provisioning.html): 推荐的集群硬件
* 与其它存储系统集成:
	* [OpenStack Swift](http://spark.apache.org/docs/latest/storage-openstack-swift.html)
* [编译Spark](http://spark.apache.org/docs/latest/building-spark.html): 使用Maven编译
* [Contributing to Spark](https://cwiki.apache.org/confluence/display/SPARK/Contributing+to+Spark)
* [第三方项目](https://cwiki.apache.org/confluence/display/SPARK/Third+Party+Projects): 相关的Spark项目

### 外部资源:

* [Spark Homepage](http://spark.apache.org/)
* [Spark Wiki](https://cwiki.apache.org/confluence/display/SPARK)
* [Spark社区](http://spark.apache.org/community.html) 资源, 以及本地会议
* [StackOverflow tag apache-spark](http://stackoverflow.com/questions/tagged/apache-spark)
* [邮件列表](http://spark.apache.org/mailing-lists.html): 提问关于Spark的问题
* [AMP Camps](http://ampcamp.berkeley.edu/): 一些列在美国伯克利的训练营，主要讨论关于Spark, Spark Streaming, Mesos等等的特性和训练.[视频](http://ampcamp.berkeley.edu/6/), [幻灯片](http://ampcamp.berkeley.edu/6/) 以及[联系](http://ampcamp.berkeley.edu/6/exercises/)在线上都是免费的。
* [Code Examples](http://spark.apache.org/examples.html): 更多样例在Spark的子文件夹```example```下面都是可用的 ([Scala](https://github.com/apache/spark/tree/master/examples/src/main/scala/org/apache/spark/examples), [Java](https://github.com/apache/spark/tree/master/examples/src/main/java/org/apache/spark/examples), [Python](https://github.com/apache/spark/tree/master/examples/src/main/python), [R](https://github.com/apache/spark/tree/master/examples/src/main/r))