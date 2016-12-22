---
layout: post
title:  "4. Resources"
date:   2016-12-21 23:00:00 +0800
categories: Spring Framework Reference Documentation
---

## 4.资源

### 4.1 介绍

java提供了标准类```java.net.URL```和标准handler用来处理不同的URL前缀。不幸的是，并非所有对low-level resource的访问它们都能够胜任。例如，当资源需要从classpath或者有关系的```ServletContext```中获取时，你找不到```URL```标准化的实现。尽管对于特定的```URL```前缀注册新的handler是可能的（例如为```http:```这个前缀注册一个handler），但是这通常是复杂的，并且```URL```接口仍然缺少一些需要的功能，例如用于检测指向资源是否存在的方法。

### 4.2 Resource 接口

Spring的```Resource ```接口意图成为更加强大的接口，它抽象了对于low-level资源的访问。

```
public interface Resource extends InputStreamSource {

    boolean exists();

    boolean isOpen();

    URL getURL() throws IOException;

    File getFile() throws IOException;

    Resource createRelative(String relativePath) throws IOException;

    String getFilename();

    String getDescription();

}
```

```
public interface InputStreamSource {

    InputStream getInputStream() throws IOException;

}
```

注意```Resource ```接口一些重要的方法：

* ```getInputStream()```:找到并打开资源，返回```InputStream```用于读取资源。这个方法期望每次被调用都返回一个新的```InputStream```。调用方需要承担关闭stream的职责。
* ```exists()```: 返回boolean值，表示资源是否以物理形式存在。
* ```isOpen()```: 返回boolean值，表示此资源是否在处理一个打开的stream。如果是true，```InputStream```不能被多次读取，并且必须只能被读取一次然后被关闭以避免资源的泄露。一般而言resource的实现将会返回false，除了```InputStreamResource```。
* ```getDescription()```: 返回对这个资源的描述，用于在这个资源工作的时候打印错误信息。描述通常是文件的全名或者资源的实际URL

其它方法允许你获取实际代表资源的URL或者File对象（如果基本的实现是兼容的并且支持对于的方法）

Resource在Spring中被广泛使用，当需要的时候会作为函数前面中的参数类型。Spring APIs中的其它方法（例如ApplicationContext不同实现的构造函数），使用像String这样没有经过修饰或简单的类型来创建适合于context实现的Resource，或者通过String path的特定前缀，允许调用者确保一个特定Resource实现必须被创建或使用。

当Resource接口在Spring中或者被Spring广泛使用的同时，它实际上作为实体类在你自己的代码中访问资源是非常有用的，尽管你并不关心Spring的其它部分。尽管它使你的代码与Spring耦合在一起，但它实际上只耦合了自己和一小部分实体类，它用来充当URL的更强大的替代品，并且可以被认为与你处于这个目的（替代URL）的其它库是等价的。

值得注意的是Resource这个抽象并没有替代功能：只要可能它就能包装。例如，UrlResource包装了一个URL，并使用被包装的URL做实际工作。

### 4.3 内置的Resource实现

在Spring中有一些列Resource接口的开箱即用的实现。

#### 4.3.1 UrlResource

UrlResource包装了java.net.URL，并且可以用来访问任何通过URL访问的对象，例如文件，http target， ftp target等等。所有的URLs都有一个标准的String来表示，例如恰当的标准的前缀用来区分URL的类别。例如file：表示访问文件系统路径，http：表示通过http协议访问资源，ftp：表示通过ftp协议访问资源吗，等等。

UrlResource对象通过UrlResource的构造函数来显式地创建，但是当你调用使用String参数代表path的API函数的时候通常被隐式创建。对于后者，一个叫做PropertyEditor的JavaBeans最终决定创建那种类型的Resource。如果path字符串包含众所周知的前缀，例如classpath：，PropertyEditor将会创建适合于那个前缀的特定的Resource。然而，如果它不认识那个前缀，它将会假设该字符串是一个标准的URL字符串，并会创建UrlResource。


#### 4.3.2 ClassPathResource

这个class表示资源应该从classpath中获取。这个类使用thread context class loader，或者给定class loader，或者给定的class来加载资源。

如果类路径在文件系统中存在的话，Resource的这个实现也支持解析java.io.File，但是如果calsspath resource在文件系统中是无法展开的例如在jar包或者在Servlet Engine中这个类就无法解析了。为了解决这个问题，Resource的不同实现总是支持将资源解析为java.net.URL。


ClassPathResource使用构造函数显示的创建，但是当你调用API函数时，这个函数使用String参数，ClassPathResource经常被隐式的创建。对于后者PropertyEditor将会识别String path的classpath:前缀，然后创建ClassPathResource对象。


#### 4.3.3 FileSystemResource

这个类是处理java.io.File的Resource接口实现。它明显支持解析File和URL。

#### 4.3.4 ServletContextResource

这个类是ServletContext资源的Resource实现，解析相对应web应用程序根路径的相对路径。

它支持访问Stream和URL，但是只有在web应用程序文件被展开并且资源在文件系统物理存在时才允许访问java.io.File。web应用程序文件是否展开还是直接从jar
文件或者其它地方如DB访问实际上依赖于Sevlet容器。

#### 4.3.5 InputStreamResource

这个类是相对于InputStream的Resource实现。这个类只在没有Resource特定适用的实现的情况下才应该被使用。特别的，推荐使用ByteArrayResource或者任何可能的基于文件的Resource实现。

相对于Resource的其它实现，这个类是对于一个已经打开资源的描述符。因此isOpen()返回true。如果你需要在某些地方持有资源描述符或者多次读取stream，不要使用这个类。

#### 4.3.6 ByteArrayResource

这个类是byte array资源的Resource实现。它为给定的byte array资源创建ByteArrayInputStream对象。

这个类适用于从给定的byte array中加载内容，而不需要借助于InputStreamResource。