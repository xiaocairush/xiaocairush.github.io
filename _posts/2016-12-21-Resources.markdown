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

### 4.4 ResourceLoader

实现了ResourceLoader的对象可以返回Resource实例

```
public interface ResourceLoader {

    Resource getResource(String location);

}
```

所有的应用上下文都实现了ResourceLoader接口，因此应用上下文可以用来获取Resource实例。

当你调用特定的应用上下文的getResource()方法时，如果location path没有特定的前缀，你将会获得一个适合于某个应用上下文的Resource类型。例如，假设下面的代码段在执行```ClassPathXmlApplicationContext```这个实例

```
Resource template = ctx.getResource("some/resource/path/myTemplate.txt");
```

这段代码将会获取一个ClassPathResource；如果相同的函数在执行FileSystemXmlApplicationContext实例，你将会获得FileSystemResource；执行WebApplicationContext，你将会获得ServletContextResource，以此类推。

另一方面，你可能想强制使用ClassPathResource，不依赖于应用上下文的类型，只需要确保以特定的classpath:作为前缀即可。

```
Resource template = ctx.getResource("classpath:some/resource/path/myTemplate.txt");
```

相似的，通过确定标准形式的ava.net.URL可以强制获取UrlResource 

```
Resource template = ctx.getResource("file:///some/resource/path/myTemplate.txt");
```

```
Resource template = ctx.getResource("http://myhost.com/resource/path/myTemplate.txt");
```

下面的表格总结了将各种String转化为Resource的策略

| 前缀       | 例子                           | 解释                     |
|------------|--------------------------------|--------------------------|
| classpath: | classpath:com/myapp/config.xml | 从classpath中加载        |
| file:      | file:///data/config.xml        | 从文件系统中加载为URL    |
| http:      | http://myserver/logo.png       | 加载为URL                |
| (none)     | /data/config.xml               | 依赖于ApplicationContext |


### 4.5 ResourceLoaderAware接口

ResourceLoaderAware是一个特定的标记接口，标识对象需要提供一个ResourceLoader引用。

```
public interface ResourceLoaderAware {

    void setResourceLoader(ResourceLoader resourceLoader);
}

```

当一个类实现了ResourceLoaderAware接口并且被发布到应用上下文时（作为Spring管理的bean），它被应用上下文识别为ResourceLoaderAware。应用上下文将会调用setResourceLoader(ResourceLoader)，并使用自身作为参数（注意，Spring的所有的应用上下文都实现了ResourceLoader接口）。


当然，因为ApplicationContext是一个ResourceLoader，实现了ApplicationContextAware接口的bean也可以直接使用应用上下文来加载资源，但是通常来说，最好使用特定的ResourceLoader接口如果正是它只需要应用上下文来加载Resource。这样做会使代码耦合于ResourceLoader，而不是耦合于Spring的ApplicationContext接口，从这个角度来看可以认为这是个实用的接口。

自从Spring 2.5起，你可以使用ResourceLoader的自动注入作为实现ResourceLoaderAware接口的替代方式。传统的contructor和byType自动注入的模式现在能够提供Resource类型的依赖作为相应的构造函数参数或者setter函数参数。如果想更灵活些（包括能够自动注入字段和多参数函数），可以考虑使用新的基于注解的自动注入特性。在那种情况下，ResourceLoader将会用来注入字段，构造函数参数，函数参数，只要在字段，构造函数，函数上加上@Autowired注解。

## 4.6 使用资源作为依赖

bean本身要通过某种动态处理来确定和应用资源的path，使用ResourceLoader接口来加载资源可能对bean来说是有意义的。以加载某种（下面代码中的）template为例，这种情况下需要的资源取决于用户的角色。如果资源是静态的，不使用ResourceLoader也可以，只需要bean暴露出来它需要的Resource属性并注入即可。

有个细节的地方是在注入属性上，所有的应用上下文都register和使用了一个叫做PropertyEditor的特殊的JavaBean，这个bean可以把String路径装换成Resource对象。所以如果myBean有一个Resource类型的template属性，就可以通过配置一个简单的string来获取资源，如下：

```
<bean id="myBean" class="...">
    <property name="template" value="some/resource/path/myTemplate.txt"/>
</bean>
```

需要注意的是资源路径没有前缀，之所以这样是因为应用上下文本身被当做ResourceLoader来使用，资源被加载为ClassPathResource, FileSystemResource 还是 ServletContextResource取决于上下文的类型。

如果想强制使用某种类型的Resource，加前缀就可以。下面的例子显示了怎样强制加载为ClassPathResource和UrlResource （后者用于访问文件系统的文件）

```
<property name="template" value="classpath:some/resource/path/myTemplate.txt">
<property name="template" value="file:///some/resource/path/myTemplate.txt"/>
```

## 4.7 应用上下文和资源路径

### 4.7.1 构造应用上下文

应用上下文构造函数把String或者String数组看成Resource(s)的path(s)，例如xml文件组成了context的定义。

尽管这样的path没有一个前缀，由那个path来构建的类型和用来加载的bean的定义，取决和适用于某种应用上下文。例如，如果你像下面这样创建ClassPathXmlApplicationContext：

```
ApplicationContext ctx = new ClassPathXmlApplicationContext("conf/appContext.xml");
```

bean的定义将从classpath中加载，作为ClassPathResource来使用。但是如果你像下面这样创建FileSystemXmlApplicationContext：

```
ApplicationContext ctx = new FileSystemXmlApplicationContext("conf/appContext.xml");
```

bean定义将从文件系统中位置加载，在这种情况下将相对于当前工作路径来寻找文件位置。

注意使用classpath前缀作为标准的URL前缀将会重载Resource的默认类型来加载定义。所以下面这个FileSystemXmlApplicationContext

```
ApplicationContext ctx = new FileSystemXmlApplicationContext("classpath:conf/appContext.xml");
```

将会从classpath来加载bean的定义。然而，它仍然是一个FileSystemXmlApplicationContext。如果后面作为ResourceLoader来使用，任何没有前缀的path将会被认为是文件系统的path。

#### 构造ClassPathXmlApplicationContext实例

ClassPathXmlApplicationContext暴露了一些列的构造函数使实例化更方便。基本的想法是只是提供一个String数组，这个数组只是包含了xml文件的文件名（没有前缀信息），同时提供一个class；ClassPathXmlApplicationContext将会根据提供的class衍生出路径信息。

一个例子可以解释清楚。路径布局是下面这样：

```
| com/
|-- foo/
|   |-- services.xml
|   |-- daos.xml
|-- MessengerService.class
```

由services.xml和daos.xm定义的bean来组成ClassPathXmlApplicationContext：

```
ApplicationContext ctx = new ClassPathXmlApplicationContext(new String[] {"services.xml", "daos.xml"}, MessengerService.class);
```

想更详细的了解不同的构造器请参考ClassPathXmlApplicationContext的javadocs。

### 4.7.2 应用上下文构造器resource path中的通配符

Resource path在应用上下文构造器中的值可以是一个简单的path（像上面一样）一一对应于目标Resource，或者包含特殊的“classpath*:”前缀或者内部Ant风格的正则表达式（使用Spring的PathMatcher来匹配）。后面两种都是有效的通配符。

这种机制的一种用处是组装组装风格的应用。所有的组件可以将上下文定义片段发布到一个已知的path位置，当使用classpath*:作为path的前缀来创建最终的应用上下文时，所有的组件片段可以被自动选择。

注意这种通配符是应用上下文构造器中Resource path的特殊用法（或者说直接使用了PathMatcher），在构造的时候解析完成。它与Resource类型本身没有关系。不能使用classpath*：前缀来构造一个实际的Resource，因为一个Resource一次只能指向一个资源。

#### Ant风格的Pattern

当位置的path包含了Ant风格的pattern的时候，例如

```
/WEB-INF/*-context.xml
|-- com/mycompany/**/applicationContext.xml
|-- file:C:/some/path/*-context.xml
|-- classpath:com/mycompany/**/applicationContext.xml
```

解析器遵循一种更加复杂但是定义的步骤来解析通配符。它为最后没有通配符的path产生一个Resource并且从中获取一个URL。如果这个URL不是jar:类型的URL或者特殊容器的变体（例如WebLogic中的zip：，WebSphere中的wsjar），就可以从中获取一个java.io.File并通过遍历文件系统来解析通配符。如果是一个jar的URL，解析器从中获取一个java.net.JarURLConnection或者人工转换jar URL然后遍历jar文件中的内容来解析通配符。

#### 蕴含的可移植性

如果某一个path已经是一个文件的URL（或者显式的或者是隐式的）因为ResourceLoader是基于文件系统的，可以保证可移植风格的通配符是有效的。

如果某一个path是classpath中的位置，解析器就可以通过调用Classloader.getResource()来获取最后的没有通配符的URL路径。因为它只是path中的一个节点（而不是在最后的文件名）实际上（在ClassLoader javadocs）中没有定义这种情况下返回哪种类型的URL。实际上，总是使用java.io.File来表示目录，不论classpath resource解析到文件系统位置或者某种jar的URL或者jar的位置。同时它考虑了操作的可移植性。

如果jar的URL从最后的没有通配符的片段获取，这个解析器就一定能从中获取java.net.JarURLConnection，或者人工转换jar的URL，一定能遍历jar的内容并解析通配符。这将在大多数的环境下有效，但是也会失败，强烈推荐在你依赖这种方式前在自己的环境中测一下能不能解析来自jar的资源。

#### classpath*:前缀的可移植性

当构造基于xml的应用上下文时，表示位置的string可能使用classpath*:前缀：

```
ApplicationContext ctx = new ClassPathXmlApplicationContext("classpath*:conf/appContext.xml");
```

这种特殊的前缀确定所有必须获取的classpath资源（在内部，本质上调用ClassLoader.getResources(…​)），然后合并成最后的应用上下文定义。
