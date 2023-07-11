---
layout: post
title:  "BlockingQueue源码剖析"
date:   2017-09-20 20:00:00 +0800
categories: MultiThread
---

[toc]

# BlockingQueue源码剖析

## 参考

[英文文档](https://docs.oracle.com/javase/8/docs/api/index.html?java/util/concurrent/package-summary.html)<br/>
[英文 jenkov](http://tutorials.jenkov.com/java-util-concurrent/index.html)<br/>
[中文 defonds](http://blog.csdn.net/defonds/article/details/44021605/)<br/>

```
package java.util.concurrent;
import java.util.Collection;
import java.util.Queue;

public interface BlockingQueue<E> extends Queue<E> {
    
    boolean add(E e);
    
    boolean offer(E e);
    
    void put(E e) throws InterruptedException;

    
    boolean offer(E e, long timeout, TimeUnit unit)
        throws InterruptedException;

    
    E take() throws InterruptedException;

    
    E poll(long timeout, TimeUnit unit)
        throws InterruptedException;

    int remainingCapacity();
    
    boolean remove(Object o);
    
    public boolean contains(Object o);
    
    //从队列中移除所有可用的元素，并将移除的元素加入到给定的集合中。当你需要重复poll的时候这个操作比较有效。
    int drainTo(Collection<? super E> c);

    int drainTo(Collection<? super E> c, int maxElements);
}

```

BlockingQueue的实现类有

* ArrayBlockingQueue
* DelayQueue
* LinkedBlockingQueue
* PriorityBlockingQueue
* SynchronousQueue

## ArrayBlockingQueue

[转载地址](http://www.jianshu.com/p/9a652250e0d1)

### 核心组成：

```
	/** 底层维护队列元素的数组 */
    final Object[] items;

    /**  当读取元素时数组的下标(这里称为读下标) */
    int takeIndex;

    /** 添加元素时数组的下标 (这里称为写小标)*/
    int putIndex;

    /** 队列中的元素个数 */
    int count;

    /** 用于并发控制的工具类**/
    final ReentrantLock lock;

	/** 保证所有访问的主要的锁*/
    final ReentrantLock lock;

    /** 控制take操作时是否让线程等待 */
    private final Condition notEmpty;

    /** 控制put操作时是否让线程等待 */
    private final Condition notFull;
```

### 取元素逻辑

在队列是空的时候需要等待到它不是空，同时take需要保证线程安全性。
```
public E take() throws InterruptedException {
        final ReentrantLock lock = this.lock;
        /*
            尝试获取锁之前，检测当前线程是否被中断，如果已被中断就抛出InterruptedException
         */
        lock.lockInterruptibly();
        try {
            /*
          如果此时队列中的元素个数为0,那么就让当前线程wait,并且释放锁。
          注意:这里使用了while进行重复检查，是为了防止当前线程可能由于其他未知的原因被唤醒。
          (通常这种情况被称为"spurious wakeup")
            */
        while (count == 0)
		        // 使当前线程一直处于等待状态，直到notEmpty发出signal信号
                notEmpty.await();
            //如果队列不为空，则从队列的头部取元素
            return extract();
        } finally {
             //完成锁的释放
            lock.unlock();
        }
    }

/*
      根据takeIndex来获取当前的元素,然后通知其他等待的线程。
      Call only when holding lock.(只有当前线程已经持有了锁之后，它才能调用该方法)
     */
    private E extract() {
        final Object[] items = this.items;

        //根据takeIndex获取元素,因为元素是一个Object类型的数组,因此它通过cast方法将其转换成泛型。
        E x = this.<E>cast(items[takeIndex]);

        //将当前位置的元素设置为null
        items[takeIndex] = null;

        //并且将takeIndex++,注意：这里因为已经使用了锁，因此inc方法中没有使用到原子操作
        takeIndex = inc(takeIndex);

        //将队列中的总的元素减1
        --count;
        //唤醒其他等待的线程
        notFull.signal();
        return x;
    }
```

### 存元素逻辑

```
public void put(E e) throws InterruptedException {
        checkNotNull(e);
        final ReentrantLock lock = this.lock;
        //进行锁的抢占
        lock.lockInterruptibly();
        try {
            /*当队列的长度等于数组的长度,此时说明队列已经满了,这里同样
              使用了while来方式当前线程被"伪唤醒"。*/
            while (count == items.length)
                //则让当前线程处于等待状态
                notFull.await();
            //一旦获取到锁并且队列还未满时，则执行insert操作。
            insert(e);
        } finally {
            //完成锁的释放
            lock.unlock();
        }
    }

     //该方法的逻辑非常简单
    private void insert(E x) {
        //将当前元素设置到putIndex位置   
        items[putIndex] = x;
        //让putIndex++
        putIndex = inc(putIndex);
        //将队列的大小加1
        ++count;
        //唤醒其他正在处于等待状态的线程
        notEmpty.signal();
    }
```

### inc

在extract和insert的时候都用到了inc方法，可以看出数组空间被循环利用了，因此ArrayBlockingQueue其实是一个循环队列

```
final int inc(int i) {
        //当takeIndex的值等于数组的长度时,就会重新置为0，这个一个循环递增的过程
        return (++i == items.length) ? 0 : i;
    }
```

## DelayQueue


进入DelayQueue的元素必须实现 java.util.concurrent.Delayed 接口：

```
public interface Delayed extends Comparable<Delayed< {  
  
 public long getDelay(TimeUnit timeUnit);  
  
}  
```

当getDelay方法返回延迟的是 0 或者负值时，将被认为过期，该元素将会在 DelayQueue 的下一次take被调用的时候被释放掉。


[原理实现参考文档](http://www.jianshu.com/p/e0bcc9eae0ae)

### 核心组成

```
	private final transient ReentrantLock lock = new ReentrantLock();
	// 用于根据delay时间排序的优先级队列
    private final PriorityQueue<E> q = new PriorityQueue<E>();

	// 这个变量用来保存工作线程的引用，通过减少线程切换
    private Thread leader = null;

    
    private final Condition available = lock.newCondition();
```
### leader-follower模式

参考[这篇文章](http://blog.csdn.net/goldlevi/article/details/7705180)

### take

```
// 获取队列中首元素，如果该元素未过期就需要等待该元素过期，然后取出该元素
public E take() throws InterruptedException {
        final ReentrantLock lock = this.lock;
        lock.lockInterruptibly();
        try {
            for (;;) {
	            // 从优先队列中取出但不删除第一个元素
                E first = q.peek();
                if (first == null)
	                // 阻塞队列为空的状态下需要等待
                    available.await();
                else { // 阻塞队列非空，且leader为null
                
	                // 检测first是否过期，如果过期就取出first并返回
                    long delay = first.getDelay(NANOSECONDS);
                    if (delay <= 0)
                        return q.poll();
                    
                    // first没有过期，需要继续等待
                    first = null; // 在等待期间first不持有引用，是因为first有可能被其它线程拿走了，且其它线程用完之后需要GC回收掉
                    // 当前线程需要竞选leader
                    if (leader != null)
                        available.await();
                    else {
	                    // 当前线程竞选leader成功
                        Thread thisThread = Thread.currentThread();
                        leader = thisThread;
                        try {
	                        // 等待首元素过期，可减少线程切换时间，从而提高效率
                            available.awaitNanos(delay);
                        } finally {
                            if (leader == thisThread)
                                leader = null;
                        }
                    }
                }
            }
        } finally {
	        // 如果阻塞队列非空且没有在work的leader线程，此时队列可用
            if (leader == null && q.peek() != null)
                available.signal();
            lock.unlock();
        }
    }
```

思考：采用leader/follower设计模式有什么好处？

不采用L/F模式，即直接将for循环里面first = null;之后的代码替换为available.awaitNanos(delay)，无法避免频繁切换线程上下文的开销，比如以下场景：
线程A和B同时在awaitNanos，A等待了100ms，B等待了200ms，假设等待时限为300ms，线程B被唤醒，并在100ms内完成take()操作，100ms之后线程A被唤醒，发现首元素没有过期，因为之前过期的元素已经被线程B take出去了，就会继续等待。可见，线程A没有必要被唤醒，因为它付出了切换线程A上下文的代价。

采用L/F模式，可以避免这种开销，因为follower线程一直都在等待，不存在线程切入与切出上下文的问题。

### put

```
	public void put(E e) {
        offer(e);
    }

	// 向队列中放一个元素，不满足放入条件时阻塞
	public boolean offer(E e) {
        final ReentrantLock lock = this.lock;
        lock.lock();
        try {
            q.offer(e);
            if (q.peek() == e) {
                leader = null;
                available.signal();
            }
            return true;
        } finally {
            lock.unlock();
        }
    }
```


## LinkedBlockingQueue

可参考[这篇文章](http://www.jianshu.com/p/057e94b71df9)

### 核心成员

```
//链表中节点的定义
 static class Node<E> {
		// 放入队列的元素值
        E item;

        /**
         * 指向下一个节点
         * 指向this, 意思是下一个节点是head.next
         * null, 没有实际的下一个节点
         */
        Node<E> next;

        Node(E x) { item = x; }
    }

    /** 队列的容量, 不提供容量就是Integer.MAX_VALUE*/
    private final int capacity;

    /**队列中元素的容量*/
    private final AtomicInteger count = new AtomicInteger();

    /**
     * 链表头结点引用.
     * 恒等式: head.item == null
     * 头结点不用于存储实际的元素
     */
    transient Node<E> head;

    /**
     * 链表尾节点
     * 恒等式: last.next == null
     */
    private transient Node<E> last;

    /** 保护take, poll等获取元素操作的锁*/
    private final ReentrantLock takeLock = new ReentrantLock();

    private final Condition notEmpty = takeLock.newCondition();

    /** 保护put, offer等存入元素操作的锁 */
    private final ReentrantLock putLock = new ReentrantLock();

    private final Condition notFull = putLock.newCondition();

```

### take

// 当队列非空的时候，取出链表首元素
```
public E take() throws InterruptedException {
        E x;
        int c = -1;
        final AtomicInteger count = this.count;
        final ReentrantLock takeLock = this.takeLock;
        takeLock.lockInterruptibly();
        try {
            while (count.get() == 0) {
                notEmpty.await();
            }
            x = dequeue();
            c = count.getAndDecrement();
            if (c > 1)
	            // 在检查take的时候检查队列是否非空，并发出非空的信号，这挺贪心的。目的是提高效率，并非必须检查。
                notEmpty.signal();
        } finally {
            takeLock.unlock();
        }
        if (c == capacity)
            signalNotFull();
        return x;
    }

private E dequeue() {
        Node<E> h = head;
        Node<E> first = h.next;
        // todo: 为什么要这样写？
        // stackoverflow: https://stackoverflow.com/questions/10106191/openjdks-linkedblockingqueue-implementation-node-class-and-gc
        // commit: http://gee.cs.oswego.edu/cgi-bin/viewcvs.cgi/jsr166/src/main/java/util/concurrent/LinkedBlockingQueue.java?r1=1.50&r2=1.51
        h.next = h; // help GC
        head = first;
        E x = first.item;
        first.item = null;
        return x;
    }
```

### put 

```
public void put(E e) throws InterruptedException {
        if (e == null) throw new NullPointerException();
        // 注意: 约定在put/take等操作中都要预设本地变量
        // 如果没有给c负值，就是-1，用来判断是否操作失败
        int c = -1;
        Node<E> node = new Node<E>(e);
        final ReentrantLock putLock = this.putLock;
        final AtomicInteger count = this.count;
        putLock.lockInterruptibly();
        try {
	        // 检查队列是否已满，如果已满需要等待队列变为未满的状态
            while (count.get() == capacity) {
                notFull.await();
            }
            enqueue(node);
            c = count.getAndIncrement();
            if (c + 1 < capacity)
                notFull.signal();
        } finally {
            putLock.unlock();
        }
        // 发出非空信号
        if (c == 0)
            signalNotEmpty();
    }

private void enqueue(Node<E> node) {
        // assert putLock.isHeldByCurrentThread();
        // assert last.next == null;
        last = last.next = node;
    }
    
private void signalNotEmpty() {
        final ReentrantLock takeLock = this.takeLock;
        takeLock.lock();
        try {
            notEmpty.signal();
        } finally {
            takeLock.unlock();
        }
    }
```

## PriorityBlockingQueue

### 核心成员

```

    /**
     * 用来表示优先级队列的平衡的二进制堆（最小堆）：queue[n]的两个儿子是queue[2*n+1]和 queue[2*(n+1)]。  
     * 优先级队列根据comparator排序, 如果comparator是null的话，就根据元素自然顺序排序。
     */
    private transient Object[] queue;

    //元素的个数
    private transient int size;

     // 用于确定元素顺序的比较器
    private transient Comparator<? super E> comparator;

     // 用于保护所有操作的锁
    private final ReentrantLock lock;

    private final Condition notEmpty;

     // 用来保护扩容的锁（Spinlock），需要通过CAS来实现
    private transient volatile int allocationSpinLock;

     // 仅用于序列化，只有在序列化和反序列话的时候才是非空的
    private PriorityQueue<E> q;
```

### take

```
public E take() throws InterruptedException {
        final ReentrantLock lock = this.lock;
        lock.lockInterruptibly();
        E result;
        try {
            while ( (result = dequeue()) == null)
                notEmpty.await();
        } finally {
            lock.unlock();
        }
        return result;
    }
private E dequeue() {
		// 指向最后一个元素
        int n = size - 1;
        if (n < 0)
            return null;
        else {
            Object[] array = queue;
            E result = (E) array[0];
            E x = (E) array[n];
            array[n] = null;
            Comparator<? super E> cmp = comparator;
            if (cmp == null)
                siftDownComparable(0, x, array, n);
            else
	            // 继续维护最小堆
                siftDownUsingComparator(0, x, array, n, cmp);
            size = n;
            return result;
        }
    }
```

### put

```
  public void put(E e) {
        offer(e); // never need to block
    }
    
    public boolean offer(E e) {
        if (e == null)
            throw new NullPointerException();
        final ReentrantLock lock = this.lock;
        lock.lock();
        int n, cap;
        Object[] array;
        while ((n = size) >= (cap = (array = queue).length))
	        // 如果数组空间不够，需要重新分配空间，offer比较简单，因此可以主要关注一下数组空间是怎样增长的即可
            tryGrow(array, cap);
        try {
            Comparator<? super E> cmp = comparator;
            if (cmp == null)
                siftUpComparable(n, e, array);
            else
                siftUpUsingComparator(n, e, array, cmp);
            size = n + 1;
            notEmpty.signal();
        } finally {
            lock.unlock();
        }
        return true;

    private void tryGrow(Object[] array, int oldCap) {
        lock.unlock(); // 必须释放并在函数尾重新获取锁，这样take就不会被阻塞住了，效率有提升
        Object[] newArray = null;
        
        if (allocationSpinLock == 0 &&
        // 如果allocationSpinLock为0就把1赋值给它
            UNSAFE.compareAndSwapInt(this, allocationSpinLockOffset,
                                     0, 1)) {
            try {
            // 如果当前size小于64，就增长为2*size+2; 否则增长为1.5 * size
            // 这种策略可以使size小的时候增长快，size大的时候增长不会过快
                int newCap = oldCap + ((oldCap < 64) ?
                                       (oldCap + 2) : // grow faster if small
                                       (oldCap >> 1));
                if (newCap - MAX_ARRAY_SIZE > 0) {    // 可能会溢出
                    int minCap = oldCap + 1; // 溢出的情况下size增加1
                    if (minCap < 0 || minCap > MAX_ARRAY_SIZE)
                        throw new OutOfMemoryError();
                    newCap = MAX_ARRAY_SIZE;
                }

				// 如果queue != array，说明之前有其它线程分配了新的空间
                if (newCap > oldCap && queue == array)
                    newArray = new Object[newCap];
            } finally {
                allocationSpinLock = 0;
            }
        }
        if (newArray == null) // 如果其它线程正在尝试重新分配空间，暗示cpu优先调度其它线程
            Thread.yield();
        lock.lock();
        if (newArray != null && queue == array) {
            queue = newArray;
            System.arraycopy(array, 0, newArray, 0, oldCap);
        }
    }
```


## SynchronousQueue

SynchronousQueue 是一个特殊的队列，它的内部同时只能够容纳单个元素。<br/>
如果该队列已有一元素的话，试图向队列中插入一个新元素的线程将会阻塞，直到另一个线程将该元素从队列中抽走。<br/>
同样，如果该队列为空，试图向队列中抽取一个元素的线程将会阻塞，直到另一个线程向队列中插入了一条新的元素。
