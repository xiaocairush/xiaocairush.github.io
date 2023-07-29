# JDK源码剖析系列之AQS

## 概览

`AbstractQueuedSynchronizer`简称AQS，采用了模板模式，是实现锁和各种同步器的模板类，提供了独占和共享两种模式。

## 类的主要成员
1. `volatile int state`。acquire和release方法实际就是修改这个状态变量的值。一般acquire对状态值做加法，release对状态值做减法。
2. 同步队列。实际就是一个双向链表，又叫CLH队列，一个线程获取不到共享资源的访问权限，就将这个线程打包成节点并放入这个同步队列，然后线程进入休眠。
3. 独占线程。当前拥有共享资源访问权限的线程，主要用于独占模式判断当前线程是否拥有共享资源的访问权限。

## 类的api
1. `acquire` 独占模式下获取共享资源的访问权
2. `acquireShared` 共享模式下获取共享资源的访问权
3. `release` 独占模式下释放共享资源的访问权
4. `releaseShared` 共享模式下释放共享资源的访问权

这些方法都是final修饰的，符合里氏代换原则，不能被子类覆盖。在这些方法内部会调用`tryAcquire` 、`tryRelease` 、`tryAcquireShared` 、`tryReleaseShared`这些模板方法，AQS中没有实现这些`tryXXX`方法，因此AQS的子类必须至少实现其中的一组方法(`tryAcquire` 和`tryRelease`是一组，`tryAcquireShared` 和`tryReleaseShared`是一组)

## acquire和release流程

![](https://fastly.jsdelivr.net/gh/filess/img3@main/2023/07/29/1690623620866-df67f183-4c0e-4574-bf98-61e1ab9cacde.png)


## 等待队列流程
AQS除了提供了同步队列，还提供了等待队列。
用法举例：
```java
  class BoundedBuffer<E> {
    final Lock lock = new ReentrantLock();
    final Condition notFull  = lock.newCondition(); 
    final Condition notEmpty = lock.newCondition(); 
 
    final Object[] items = new Object[100];
    int putptr, takeptr, count;
 
    public void put(E x) throws InterruptedException {
      lock.lock();
      try {
        while (count == items.length)
          notFull.await();
        items[putptr] = x;
        if (++putptr == items.length) putptr = 0;
        ++count;
        notEmpty.signal();
      } finally {
        lock.unlock();
      }
    }
 
    public E take() throws InterruptedException {
      lock.lock();
      try {
        while (count == 0)
          notEmpty.await();
        E x = (E) items[takeptr];
        if (++takeptr == items.length) takeptr = 0;
        --count;
        notFull.signal();
        return x;
      } finally {
        lock.unlock();
      }
    }
  }
  
```
下图解释了`notFull.await()`和`notFull.signal()`的内部流程：

![](https://fastly.jsdelivr.net/gh/filess/img4@main/2023/07/29/1690624044320-a095334d-f832-46f4-9240-83f91de1f0f7.png)



---

欢迎关注我的公众号“**窗外月明**”，原创技术文章第一时间推送。

<center>
    <img src="https://open.weixin.qq.com/qr/code?username=gh_c36a67dfc3b3" style="width: 100px;">
</center>
