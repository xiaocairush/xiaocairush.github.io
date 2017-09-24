---
layout: post
title:  "java.util.concurrent.locks源码剖析"
date:   2017-09-23 20:00:00 +0800
categories: MultiThread
---

# java.util.concurrent.locks源码剖析

## 参考

[英文文档](https://docs.oracle.com/javase/8/docs/api/index.html?java/util/concurrent/package-summary.html)<br/>
[英文 jenkov](http://tutorials.jenkov.com/java-util-concurrent/index.html)<br/>
[中文 defonds](http://blog.csdn.net/defonds/article/details/44021605/)<br/>


## Lock接口

接口定义：

```
package java.util.concurrent.locks;
import java.util.concurrent.TimeUnit;

public interface Lock {
    
    void lock();
    
    void lockInterruptibly() throws InterruptedException;
    
    boolean tryLock();
    
    boolean tryLock(long time, TimeUnit unit) throws InterruptedException;
    
    void unlock();

	// 依赖于Condition接口
    Condition newCondition();
}
```

## Condition接口

```
public interface Condition {

    void await() throws InterruptedException;

    void awaitUninterruptibly();
    
    long awaitNanos(long nanosTimeout) throws InterruptedException;
    
    boolean await(long time, TimeUnit unit) throws InterruptedException;
    
    boolean awaitUntil(Date deadline) throws InterruptedException;
    
    void signal();
    
    void signalAll();
}


```


## ReentrantLock

参考这篇[《ReentrantLock源码分析》](http://blog.csdn.net/tangyongzhe/article/details/44227593)

### 核心成员

ReentrantLock只有一个成员sync

```
	// 提供所有实现机制的同步器
    private final Sync sync;
```


### 核心方法

可以看出ReentrantLock只是Sync的一个代理

```
public class ReentrantLock implements Lock, java.io.Serializable {

	// 默认是非公平锁
	public ReentrantLock() {
        sync = new NonfairSync();
    }
	
	public void lock() {
        sync.lock();
    }

	public void unlock() {
        sync.release(1);
    }

	public boolean tryLock() {
        return sync.nonfairTryAcquire(1);
    }

	// 要求状态值减释放数，如果当前线程不是独自模式下的拥有者，那么它无权修改状态值，抛异常，可见ReentrantLock是独占锁
	protected final boolean tryRelease(int releases) {
			// 计算新的状态值
            int c = getState() - releases;
            // 如果线程不是独自模式下的拥有者，那么它就无权要求修改状态值，就抛出异常
            if (Thread.currentThread() != getExclusiveOwnerThread())
                throw new IllegalMonitorStateException();
            boolean free = false;
            if (c == 0) {
                free = true;
                setExclusiveOwnerThread(null);
            }
            setState(c);
            return free;
        }

	public Condition newCondition() {
        return sync.newCondition();
    }
}
```

### 数据结构

我们主要关心这些数据结构的成员以及核心方法

#### Sync

从定义可以看出Sync是一个抽象类，继承了AbstractQueuedSynchronizer，它的实现类有FairSync和NonfairSync.

```
 abstract static class Sync extends AbstractQueuedSynchronizer {
		// 没有定义成员

		// 子类需要去实现lock
        abstract void lock();
            
        final ConditionObject newCondition() {
	        // ConditionObject继承自AbstractQueuedSynchronizer
            return new ConditionObject();
        }
 }
```

#### NonfairSync

默认的同步机制，发现这个类非常简单，主要调用AbstractQueuedSynchronizer，因此，要了解ReentrantLock，最重要的是了解AbstractQueuedSynchronizer，同时注意对比FairSync和NonfairSync。 <br/>
非公平锁（Nonfair）：获取锁时不考虑排队等待问题，直接尝试获取锁，获取不到自动到队尾等待

```
    static final class NonfairSync extends Sync {

        final void lock() {
	        // 如果sync中的state没有被任何线程占有，则设定当前线程为锁的拥有者
            if (compareAndSetState(0, 1))
	            // AbstractOwnableSynchronizer中的方法，设置当前线程为锁的拥有者
                setExclusiveOwnerThread(Thread.currentThread());
            // 否则当前线程需要和sync队列中的其它线程竞争state的占有权
            else
                acquire(1);
        }
		
		  
        protected final boolean tryAcquire(int acquires) {
            return nonfairTryAcquire(acquires);
        }
		
		//非公平锁的获取方法，值得注意的是：这个方法实际是在NonfairSync的父类Sync中
	final boolean nonfairTryAcquire(int acquires) {
            final Thread current = Thread.currentThread();
            // 获取state的值
            int c = getState();
            // 如果state没被占有，就独占它
            if (c == 0) {
                if (compareAndSetState(0, acquires)) {
                    setExclusiveOwnerThread(current);
                    return true;
                }
            }
            // 如果state已经被占有，并且是被当前线程占有的
            else if (current == getExclusiveOwnerThread()) {
                int nextc = c + acquires;
                // 如果引用次数溢出，抛出异常而不是返回false
                if (nextc < 0) // overflow
                    throw new Error("Maximum lock count exceeded");
                // 更新引用次数
                setState(nextc);
                return true;
            }
            // 如果已被其他线程占有，那么不能修改state的值
            return false;
        }
        
    }
```

#### FairSync

公平锁（Fair）：加锁前检查是否有排队等待的线程，优先排队等待的线程，先来先得 

```
static final class FairSync extends Sync {
        private static final long serialVersionUID = -3000897897090466540L;

        final void lock() {
            acquire(1);
        }

        /**
         * Fair version of tryAcquire.  Don't grant access unless
         * recursive call or no waiters or is first.
         */
        protected final boolean tryAcquire(int acquires) {
            final Thread current = Thread.currentThread();
            int c = getState();
            // 没有没线程占有
            if (c == 0) {
	            // 必须是队列中的第一个线程，才能获取state的使用权，修改state的值
                if (!hasQueuedPredecessors() &&
                    compareAndSetState(0, acquires)) {
                    setExclusiveOwnerThread(current);
                    return true;
                }
            }
            // 当前线程已经获得state的使用权
            else if (current == getExclusiveOwnerThread()) {
                int nextc = c + acquires;
                if (nextc < 0)
                    throw new Error("Maximum lock count exceeded");
                setState(nextc);
                return true;
            }
            return false;
        }
    }
```


## AbstractQueuedSynchronizer

简称AQS，是一个非常核心的类，也是本文的重点关注对象。其实AQS主要就是维护了一个状态值，release对状态值做减法，acquire对状态值做加法。并且AQS提供了独占和共享两种模式。

### 内部类

AQS中有两个内部类：Node和ConditionObject。

#### Node

Node是等待队列中的节点类。
等待队列是["CLH" (Craig, Landin, and Hagersten)锁队列](https://people.csail.mit.edu/shanir/publications/CLH.pdf)的一种变体。CLH锁一般用作[自旋锁（spinlocks）](https://baike.baidu.com/item/%E8%87%AA%E6%97%8B%E9%94%81)。然而这里使用了相同的策略，把关于thread的控制信息保存在前一个节点中，只不过会阻塞。每个节点中的状态字段都记录了线程是否应该阻塞。节点会在前一个节点释放的时候收到信号被唤醒。因此队列中的每一个节点充当了一种特定通知风格的监视器，同时持有一个正在等待的线程。状态字段不控制线程是否授予锁等。如果线程在队列中的第一个的话，可能尝试获取锁。但是拍照第一个并不能保证获取锁成功；只是给予这个线程参与竞争的条件。所以当前被释放的竞争者可能还要等待。<br/>
为了让一个CLH锁入队，你可以让它自动拼接到新的tail中。离队只需要重新设置head字段。

```
      +------+  prev +-----+       +-----+
 head |      | <---- |     | <---- |     |  tail
      +------+       +-----+       +-----+
``` 

入队与离队分别是作用于tail和head的原子操作。然而，由于超时和中断可能导致线程被取消，node还需要确定它的下一个节点是谁。相比原来CLH锁的实现，增加了prev字段主要用来处理线程被取消这种情况。如果一个node中的线程被取消了，它的下一个节点就需要重新找一个没有被取消的节点来作为前继。想了解自旋锁的相似机制，可以看Scott和Scherer的[论文](http://www.cs.rochester.edu/u/scott/synchronization/)。<br>

```
    static final class Node {
         /** Marker to indicate a node is waiting in shared mode */
        static final Node SHARED = new Node();
        /** Marker to indicate a node is waiting in exclusive mode */
        static final Node EXCLUSIVE = null;

        /** waitStatus的值，表示thread被取消了 */
        static final int CANCELLED =  1;
        /** waitStatus的值，表示下一个节点的线程需要阻塞 */
        static final int SIGNAL    = -1;
        /** waitStatus的值，表示thread正在等待某个条件 */
        static final int CONDITION = -2;
        /**
         * waitStatus value to indicate the next acquireShared should
         * unconditionally propagate
         */
        static final int PROPAGATE = -3;
        
        volatile int waitStatus;

        volatile Node prev;
        
        volatile Node next;
        
        volatile Thread thread;

        Node nextWaiter;
    }
```

#### ConditionObject

仅列出重要的成员以及方法。

```
/*
 * 维护了一个条件队列，注意区别于AQS中的同步队列。
 * 条件队列用来记录未满足条件的线程，每当一个线程需要等待条件满足的时候，就加入条件队列进行等待；
 * 当条件被满足的时候，线程就会把等待队列中所有的线程按照顺序加入到同步队列，
 * 并与同步队列竞争state的使用权。
 */
public class ConditionObject implements Condition, java.io.Serializable {

         // 条件队列的第一个节点
         private transient Node firstWaiter;
        
         // 条件队列的最后一个节点
         private transient Node lastWaiter;

		 public ConditionObject() { }

		 public final void await() throws InterruptedException {
            if (Thread.interrupted())
                throw new InterruptedException();
            // 添加了一个当前线程节点到条件队列的尾部
            Node node = addConditionWaiter();
            // 完全释放当前线程对state的占有权，唤醒同步队列中第一个等待的线程，并记录当前线程占有的state的值
            int savedState = fullyRelease(node);
            int interruptMode = 0;
            
            // 等待另一个其它线程将当前线程从条件队列加入到同步队列中（调用sigal函数）
            while (!isOnSyncQueue(node)) {
	            // 当前线程被挂起，等待被唤醒
                LockSupport.park(this);
                // 如果是因为被中断而醒过来，就把当前线程直接加入到同步队列中
                // 自己将自己加入同步队列，需要抛异常，如果等待其它线程将自己加入同步队列，不需要抛异常
                if ((interruptMode = checkInterruptWhileWaiting(node)) != 0)
                    break;
                 // 也有可能是由于未知原因而醒过来，这时候interruptMode不是0，所以需要一个循环来确保当前线程被加入到同步队列中
            }
			
			// 当前线程与同步队列中其它的线程进行竞争，直到当前线程获取到state的使用权
            if (acquireQueued(node, savedState) && interruptMode != THROW_IE)
                interruptMode = REINTERRUPT;
            // 遍历条件队列，移除所有被取消的线程
            if (node.nextWaiter != null) // clean up if cancelled
                unlinkCancelledWaiters();
            // 如果被中断了，并且是通过其他线程将当前线程加入到同步队列中的
            if (interruptMode != 0)
                reportInterruptAfterWait(interruptMode);
        }

		
		private Node addConditionWaiter() {
            Node t = lastWaiter;
            // If lastWaiter is cancelled, clean out.
            if (t != null && t.waitStatus != Node.CONDITION) {
                unlinkCancelledWaiters();
                t = lastWaiter;
            }
            Node node = new Node(Thread.currentThread(), Node.CONDITION);
            if (t == null)
                firstWaiter = node;
            else
                t.nextWaiter = node;
            lastWaiter = node;
            return node;
        }

		// 遍历条件队列，移除所有被取消的线程
		 private void unlinkCancelledWaiters() {
            Node t = firstWaiter;
            Node trail = null;
            // 从前向后遍历条件队列
            while (t != null) {
                Node next = t.nextWaiter;
                // 在条件队列中，如果waitStatus != CONDITION，表示线程被取消
                if (t.waitStatus != Node.CONDITION) {
                    t.nextWaiter = null;
                    if (trail == null)
                        firstWaiter = next;
                    else
                        trail.nextWaiter = next;
                    if (next == null)
                        lastWaiter = trail;
                }
                else
                    trail = t;
                t = next;
            }
        }

		// 线程醒过来之后检查是否被中断，
		// 如果没有被中断，返回0；
		// 如果被中断了，且自己可以将当前线程加入到同步队列中，返回THROW_IE；
		// 如果被中断了，通过等待其它线程将当前线程加入到同步队列中，返回REINTERRUPT。
		private int checkInterruptWhileWaiting(Node node) {
            return Thread.interrupted() ?
                (transferAfterCancelledWait(node) ? THROW_IE : REINTERRUPT) :
                0;
        }

		// 将当前线程添加到同步队列中，返回是否自己可以将自己加入到同步队列中
        final boolean transferAfterCancelledWait(Node node) {
        if (compareAndSetWaitStatus(node, Node.CONDITION, 0)) {
            enq(node);
            return true;
        }
        /*
         * If we lost out to a signal(), then we can't proceed
         * until it finishes its enq().  Cancelling during an
         * incomplete transfer is both rare and transient, so just
         * spin.
         */
        // 等待其他线程通过signal把当前线程加入到同步队列中
        while (!isOnSyncQueue(node))
            Thread.yield();
        return false;
    }
    
	private void reportInterruptAfterWait(int interruptMode)
            throws InterruptedException {
            // 中断之后抛异常
            if (interruptMode == THROW_IE)
                throw new InterruptedException();
            // 中断之后当前线程重新进入中断状态
            else if (interruptMode == REINTERRUPT)
                selfInterrupt();
        }

		// 将条件队列中的所有线程加入到同步队列中
		public final void signal() {
            if (!isHeldExclusively())
                throw new IllegalMonitorStateException();
            Node first = firstWaiter;
            if (first != null)
	            // 将条件队列中的所有线程加入到同步队列中
                doSignal(first);
        }

		private void doSignal(Node first) {
            do {
                if ( (firstWaiter = first.nextWaiter) == null)
                    lastWaiter = null;
                first.nextWaiter = null;
            } while (!transferForSignal(first) &&
                     (first = firstWaiter) != null);
        }

		final boolean transferForSignal(Node node) {
        /*
         * If cannot change waitStatus, the node has been cancelled.
         */
         // 如果线程已被取消，就不放入同步队列中
        if (!compareAndSetWaitStatus(node, Node.CONDITION, 0))
            return false;

        /*
         * Splice onto queue and try to set waitStatus of predecessor to
         * indicate that thread is (probably) waiting. If cancelled or
         * attempt to set waitStatus fails, wake up to resync (in which
         * case the waitStatus can be transiently and harmlessly wrong).
         */
        Node p = enq(node);
        int ws = p.waitStatus;
        // 如果同步队列中前一个线程已被取消或者将前一个线程状态设置成signal失败，就唤醒该线程与同步队列其它线程竞争
        if (ws > 0 || !compareAndSetWaitStatus(p, ws, Node.SIGNAL))
            LockSupport.unpark(node.thread);
        return true;
    }
}
```

### 核心成员

```
	/**
     * 等待队列的头结点，它是一个虚拟节点，延迟初始化。
     * 如果head != null，可以保证head.waitStatus不是CANCELLED
     */
    private transient volatile Node head;

    // 等待队列尾节点，只能通过enq来加入新等待节点
    private transient volatile Node tail;

    // AQS对象的状态，初始值是0，表示state没有被任何线程占有
    // AQS最重要的成员，不同场景下具有不同的含义，一般指锁被引用的次数
    // AQS提供了竞争这个状态值占有权的框架
    private volatile int state;
```

### 核心方法

```
public abstract class AbstractQueuedSynchronizer
    extends AbstractOwnableSynchronizer
    implements java.io.Serializable {

	// 在独占模式下获取state的占有权，并使state加arg
	public final void acquire(int arg) {
		/**
		 * tryAcquire在FairSync和NonfairSync等AQS的子类中被实现。
		 * 
		 * 首先调用tryAcquire方法来尝试独占并修改state，
		 * tryAcquire如果返回false，就说明已经有thread获得了state的占有权，当前线程无权修改state，
		 * 这时候（执行acquireQueued方法）把当前节点入队并参与竞争state的占有权，当前节点变为首节点的时候获得state的占有权，state加arg。
		 * 如果当前线程在竞争过程中被中断过，则把当前线程恢复到中断状态。
		 */
        if (!tryAcquire(arg) &&
            acquireQueued(addWaiter(Node.EXCLUSIVE), arg))
            selfInterrupt();
    }

	// 添加当前线程节点到等待队列中
	private Node addWaiter(Node mode) {
		// 构造线程节点
        Node node = new Node(Thread.currentThread(), mode);
        // Try the fast path of enq; backup to full enq on failure
        Node pred = tail;
        if (pred != null) {
            node.prev = pred;
            // CAS操作，更新尾节点
            if (compareAndSetTail(pred, node)) {
                pred.next = node;
                return node;
            }
        }
        // pred是空或者CAS操作失败（尾节点已变），就入队
        enq(node);
        return node;
    }

	// 循环嵌套CAS，直到CAS成功为止
	// 将节点node加入到等待队列尾部
	 private Node enq(final Node node) {
        for (;;) {
            Node t = tail;
            // 队列为空
            if (t == null) { // Must initialize
	            // 注意这时候创建并设置虚拟的头结点，而不是在创建AQS对象的时候，属于延迟加载，创建完虚拟头结点仍然继续循环
                if (compareAndSetHead(new Node()))
                    tail = head;
            } else {
                node.prev = t;
                if (compareAndSetTail(t, node)) {
                    t.next = node;
                    return t;
                }
            }
        }
    }

     /*
      * 在独占不可中断模式下，当前线程与Sync队列中的其它线程竞争。
      * 当前线程成为队列中首节点的时，它获得state的占有权，并給state加arg；
      * 否则，Sync队列中存在有效（没有被取消）的线程，由于队列中前面的线程拥有更高的权利使用state，
      * 所以当前线程就需要阻塞，当前一个线程使用完state之后唤醒当前线程。
      * 线程处于阻塞状态时，也可以被中断而醒来，由于是不可中断模式，所以会记录并清除中断状态，
      * 将中断状态返回给调用方处理，例如acquire中会把当前线程恢复到中断状态。
      */
    final boolean acquireQueued(final Node node, int arg) {
        boolean failed = true;
        try {
            boolean interrupted = false;
            for (;;) {
	            // 等待队列是一个双向列表，这里p是node的前节点
                final Node p = node.predecessor();
                // 如果是队列中第一个线程，就获得state的占有权，并使用state
                if (p == head && tryAcquire(arg)) {
                    setHead(node);
                    p.next = null; // help GC
                    failed = false;
                    return interrupted;
                }
                // 如果不是队列中第一个线程，那么竞争失败，它无权使用state，
                // 检查队列中前面的线程是否有效（没有被取消），如果存在有效的线程，
                // 当前线程就需要阻塞，在线程醒过来之后检查是否被中断，如果被中断了，
                // 就清除中断标志位继续竞争state的使用权，但是要记录当前线程在竞争过程被中断过
                if (shouldParkAfterFailedAcquire(p, node) &&
                    parkAndCheckInterrupt())
                    interrupted = true;
            }
        } finally {
            if (failed)
                cancelAcquire(node);
        }
    }
	
	// 头结点出等待队列
	private void setHead(Node node) {
        head = node;
        node.thread = null;
        node.prev = null;
    }

     // 如果队列中存在有效的线程（没有被取消的）排在当前线程前面，那么当前线程就需要被阻塞，因为前面的线程等了更长的时间，拥有更高的使用权
    private static boolean shouldParkAfterFailedAcquire(Node pred, Node node) {
	    //waitStatus初始值为0
        int ws = pred.waitStatus;
        // 前一个线程未使用完state，当前线程就要被阻塞
        if (ws == Node.SIGNAL)
            // pred节点正在请求一个signal信号，所以它可以被安全挂起
            return true;
        // 在Sync队列中，如果大于0表示线程处于被取消的状态，
        // 被取消的线程没有必要获取state的使用权，所以直接从队列中删除取消的线程，不让它们参与竞争。暂时不挂起当前线程。
        if (ws > 0) {
             // 删除所有被取消的先继节点
            do {
                node.prev = pred = pred.prev;
            } while (pred.waitStatus > 0);
            pred.next = node;
        } else {
            /*
             * waitStatus must be 0 or PROPAGATE.  Indicate that we
             * need a signal, but don't park yet.  Caller will need to
             * retry to make sure it cannot acquire before parking.
             */
            // 前一个节点没有被取消，暂时不挂起当前线程，将前一个线程状态设置为需要sigal信号（未使用完state），
            // 下次竞争如果发现前一个线程仍没有使用完state再挂起
            compareAndSetWaitStatus(pred, ws, Node.SIGNAL);
        }
        return false;
    }

	  private final boolean parkAndCheckInterrupt() {
	  // 挂起当前线程，在被挂起的情况下，有三种情况会被唤醒，具体见park方法注释
        LockSupport.park(this);
        // 获取中断标记，如果中断标记是true，会清除中断标记
        return Thread.interrupted();
    }

	// 释放state的使用权
	final int fullyRelease(Node node) {
        boolean failed = true;
        try {
            int savedState = getState();
            if (release(savedState)) {
                failed = false;
                return savedState;
            } else {
                throw new IllegalMonitorStateException();
            }
        } finally {
            if (failed)
                node.waitStatus = Node.CANCELLED;
        }
    }

	public final boolean release(int arg) {
		// 如果当前线程释放state使用权成功（独占模式即有权利将state减arg成功），
		// 就唤醒队列中的等待的第一个线程。
        if (tryRelease(arg)) {
	        // head指向的是虚节点，没有记录线程id，head下一个节点才存了第一个线程
            Node h = head;
            if (h != null && h.waitStatus != 0)
                unparkSuccessor(h);
            return true;
        }
        return false;
    }

	// 唤醒node的下一个节点
	private void unparkSuccessor(Node node) {
        /*
         * If status is negative (i.e., possibly needing signal) try
         * to clear in anticipation of signalling.  It is OK if this
         * fails or if status is changed by waiting thread.
         */
        int ws = node.waitStatus;
        if (ws < 0)
            compareAndSetWaitStatus(node, ws, 0);

        // 找到下一个没有被取消的线程节点，在sync队列中，waitStatus > 0 表示当前节点中的线程被取消
        Node s = node.next;
        if (s == null || s.waitStatus > 0) {
            s = null;
            //从后向前遍历，找到最前面的一个没有被取消的线程
            for (Node t = tail; t != null && t != node; t = t.prev)
                if (t.waitStatus <= 0)
                    s = t;
        }
        if (s != null)
            LockSupport.unpark(s.thread);
    }
}

```

### AbstractOwnableSynchronizer

AQS继承了这个抽象类，这个类非常简单，所有成员及方法定义如下：

```
public abstract class AbstractOwnableSynchronizer
    implements java.io.Serializable {

    protected AbstractOwnableSynchronizer() { }

     // 独占独占模式下占有Synchronizer的线程对象
    private transient Thread exclusiveOwnerThread;

     // 赋予thread对锁的排他访问权限
    protected final void setExclusiveOwnerThread(Thread thread) {
        exclusiveOwnerThread = thread;
    }

    // 获取独占锁的线程对象
    protected final Thread getExclusiveOwnerThread() {
        return exclusiveOwnerThread;
    }
}

```
