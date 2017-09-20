---
layout: post
title:  "ConcurrentHashMap(1.8版本)源码剖析"
date:   2017-09-20 20:00:00 +0800
categories: MultiThread
---

# ConcurrentHashMap(1.8版本)源码剖析

基于jdk1.8。

参考文章：
https://yq.aliyun.com/articles/36781 <br/>
http://blog.csdn.net/u012834750/article/details/71536618

## 数据结构

仅列出最重要的代码片段

### Node

```
static class Node<K,V> implements Map.Entry<K,V> {
        final int hash;
        final K key;
        volatile V val;
        volatile Node<K,V> next;

		/**
         * 子类中重写了这个方法，这里的find实现了在链表中查找hash值等于h且key等于k的节点
         */
        Node<K,V> find(int h, Object k) {
            Node<K,V> e = this;
            if (k != null) {
                do {
                    K ek;
                    if (e.hash == h &&
                        ((ek = e.key) == k || (ek != null && k.equals(ek))))
                        return e;
                } while ((e = e.next) != null);
            }
            return null;
        }
 }
```

### ForwardingNode

```
	 /**
     * A node inserted at head of bins during transfer operations.
     */
     // 并不是我们传统的包含key-value的节点，只是一个标志节点，并且指向nextTable，提供find方法而已。生命周期：仅存活于扩容操作且bin不为null时，一定会出现在每个bin的首位。
    static final class ForwardingNode<K,V> extends Node<K,V> {
        final Node<K,V>[] nextTable;
        ForwardingNode(Node<K,V>[] tab) {
            super(MOVED, null, null, null);
            this.nextTable = tab;
        }

        Node<K,V> find(int h, Object k) {
            // loop to avoid arbitrarily deep recursion on forwarding nodes
            outer: for (Node<K,V>[] tab = nextTable;;) {
                Node<K,V> e; int n;
                if (k == null || tab == null || (n = tab.length) == 0 ||
                    (e = tabAt(tab, (n - 1) & h)) == null)// 头结点存在e中
                    return null;
                for (;;) {
                // 检查头结点是否为要找的node
                    int eh; K ek;
                    if ((eh = e.hash) == h &&
                        ((ek = e.key) == k || (ek != null && k.equals(ek))))
                        return e;
                        // 如果头结点不是要找的节点
                    if (eh < 0) {
	                    // 头结点hash值小于0
	                    // 如果头结点是ForwardingNode，那么继续下一个ForwardingNode的find逻辑
                        if (e instanceof ForwardingNode) {
                            tab = ((ForwardingNode<K,V>)e).nextTable;
                            continue outer;
                        }
                        // 如果头结点不是ForwardingNode，就进行相应的find逻辑
                        else
                            return e.find(h, k);
                    }
                    // 查找到尾部仍然没有找到对应的node
                    if ((e = e.next) == null)
                        return null;
                }
            }
        }
    }
```

### TreeNode

红黑树中的节点类，值得注意的是：TreeNode可用于构造双向链表，Node包含next成员，同时，TreeNode加入了prev成员。

```
static final class TreeNode<K,V> extends Node<K,V> {
        TreeNode<K,V> parent;  // red-black tree links
        TreeNode<K,V> left;
        TreeNode<K,V> right;
        TreeNode<K,V> prev;    // needed to unlink next upon deletion
        boolean red;

        TreeNode(int hash, K key, V val, Node<K,V> next,
                 TreeNode<K,V> parent) {
            super(hash, key, val, next);
            this.parent = parent;
        }

        Node<K,V> find(int h, Object k) {
            return findTreeNode(h, k, null);
        }

        final TreeNode<K,V> findTreeNode(int h, Object k, Class<?> kc) {
            if (k != null) {
                TreeNode<K,V> p = this;
                do  {
                    int ph, dir; K pk; TreeNode<K,V> q;
                    TreeNode<K,V> pl = p.left, pr = p.right;
                    if ((ph = p.hash) > h)
                        p = pl;
                    else if (ph < h)
                        p = pr;
                    else if ((pk = p.key) == k || (pk != null && k.equals(pk)))
                        return p;
                    // hash值相等，key不等，左子树不存在，搜索右子树
                    else if (pl == null)
                        p = pr;
                    // hash值相等，key不等，右子树不存在，搜索左子树
                    else if (pr == null)
                        p = pl;
                   /*
                    * comparableClassFor的作用是:
                    * 如果k实现了Comparable接口，返回k的Class,
                    * 否则返回null。
                    * compareComparables的作用是：
                    * 将k与pk做比较
                    * 如果TreeNode的Key可以作比较，就可以继续在树中搜索
                    */
                    else if ((kc != null ||
                              (kc = comparableClassFor(k)) != null) &&
                             (dir = compareComparables(kc, k, pk)) != 0)
                        p = (dir < 0) ? pl : pr;
                    // 由于hash相等，key无法做比较，因此先在右子树中找
                    else if ((q = pr.findTreeNode(h, k, kc)) != null)
                        return q;
                    // 右子树没有找到，继续从当前的节点的左子树中找
                    else
                        p = pl;
                } while (p != null);
            }
            return null;
        }
    }

```

### TreeBin

TreeBin封装了红黑树的逻辑，有关红黑树, 可以参考的资料有[《Algorithm》网站](http://algs4.cs.princeton.edu/33balanced/) 以及 [中文翻译](http://www.cnblogs.com/yangecnu/p/Introduce-Red-Black-Tree.html)

也可以试玩[Red/Black Tree Visualization](https://www.cs.usfca.edu/~galles/visualization/RedBlack.html) 。

附文章中提到的红黑树旋转的动图与TreeBin中的rotateLeft、rotateRight代码片段帮助理解。

左旋：
![rotateLeft](http://img.blog.csdn.net/20170920113731723?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvZ3V5Y2FuZHk=/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/SouthEast)

对应代码
```
	static <K,V> TreeNode<K,V> rotateLeft(TreeNode<K,V> root,
                                              TreeNode<K,V> p) {
            TreeNode<K,V> r, pp, rl;
            // p是图中的E节点，r是图中的S节点
            if (p != null && (r = p.right) != null) {
                if ((rl = p.right = r.left) != null)
                    rl.parent = p;
                // p是根节点，则根节点需要变化
                if ((pp = r.parent = p.parent) == null)
                    (root = r).red = false;
                // p不是根节点，如果p是pp的左节点，就更新pp的left
                else if (pp.left == p)
                    pp.left = r;
                else
                    pp.right = r;
                // 把p放在左子树中
                r.left = p;
                p.parent = r;
            }
            return root;
        }
```


右旋：
![rotateRight](http://img.blog.csdn.net/20170920113827242?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvZ3V5Y2FuZHk=/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/SouthEast)

对应代码

```

        static <K,V> TreeNode<K,V> rotateRight(TreeNode<K,V> root,
                                               TreeNode<K,V> p) {
            TreeNode<K,V> l, pp, lr;
            // p是途中的S，l是图中的E
            if (p != null && (l = p.left) != null) {
                if ((lr = p.left = l.right) != null)
                    lr.parent = p;
                // p是根节点，则根节点需要变化
                if ((pp = l.parent = p.parent) == null)
                    (root = l).red = false;
                else if (pp.right == p)
                    pp.right = l;
                else
                    pp.left = l;
                l.right = p;
                p.parent = l;
            }
            return root;
        }
```

仅列出Treebin数据成员以及部分方法：

```
// 维护了一个红黑树
static final class TreeBin<K,V> extends Node<K,V> {
        TreeNode<K,V> root;
        // 链表头结点，每次都将新节点插入到链表的头部，成为新的头结点
        // 因此该链表中节点的顺序与插入顺序相反
        volatile TreeNode<K,V> first;
        volatile Thread waiter;
        volatile int lockState;
		
		 /**
         * 返回匹配的node或者没有匹配的就返回null. 在树中从根节点开始比较，
         * 当锁不可用的时候进行线性搜索
         */
        final Node<K,V> find(int h, Object k) {
            if (k != null) {
                for (Node<K,V> e = first; e != null; ) {
                    int s; K ek;
                    // 锁不可用，lockState包含了WAITER或者WRITER标志位
                    if (((s = lockState) & (WAITER|WRITER)) != 0) {
                        if (e.hash == h &&
                            ((ek = e.key) == k || (ek != null && k.equals(ek))))
                            return e;
                        e = e.next;
                    }
                    // 锁可用，当前对象设置为READER状态
                    else if (U.compareAndSwapInt(this, LOCKSTATE, s,
                                                 s + READER)) {
                        TreeNode<K,V> r, p;
                        try {
	                        // 在树中查找匹配的节点
                            p = ((r = root) == null ? null :
                                 r.findTreeNode(h, k, null));
                        } finally {
                            Thread w;
                            // 取消当前锁的READER状态
                            if (U.getAndAddInt(this, LOCKSTATE, -READER) ==
                                (READER|WAITER) && (w = waiter) != null)
                                LockSupport.unpark(w);
                        }
                        return p;
                    }
                }
            }
            return null;
        }

         // 寻找或者添加一个节点
        final TreeNode<K,V> putTreeVal(int h, K k, V v) {
            Class<?> kc = null;
            boolean searched = false;
            for (TreeNode<K,V> p = root;;) {
                int dir, ph; K pk;
                //  红黑树是空，直接插入到根节点
                if (p == null) {
                    first = root = new TreeNode<K,V>(h, k, v, null, null);
                    break;
                }
                // 根据hash值设置标记位
                else if ((ph = p.hash) > h)
                    dir = -1;
                else if (ph < h)
                    dir = 1;
                // hash值相同，并且k与pk相等（equals），直接返回
                else if ((pk = p.key) == k || (pk != null && k.equals(pk)))
                    return p;
                // hash相同，p与pk不equals，但是按照比较接口发现p与pk相等
                else if ((kc == null &&
                          (kc = comparableClassFor(k)) == null) ||
                         (dir = compareComparables(kc, k, pk)) == 0) {
                    if (!searched) {
                        TreeNode<K,V> q, ch;
                        searched = true;
                        if (((ch = p.left) != null &&
                             (q = ch.findTreeNode(h, k, kc)) != null) ||
                            ((ch = p.right) != null &&
                             (q = ch.findTreeNode(h, k, kc)) != null))
                            return q;
                    }
                    // 根据一种确定的规则来进行比较，至于规则本身具体是什么病不重要
                    dir = tieBreakOrder(k, pk);
                }
				
				// 程序运行到这里，说明当前节点不匹配，但子树中可能会有匹配的Node
                TreeNode<K,V> xp = p;
                // 根据大小关系移动p到左子树或者右子树
                // 如果满足p为null，则说明树中没有节点能与之匹配，应当在p位置插入新节点，然后维护红黑树的性质
                if ((p = (dir <= 0) ? p.left : p.right) == null) {
                    TreeNode<K,V> x, f = first;
                    first = x = new TreeNode<K,V>(h, k, v, f, xp);
                    if (f != null)
                        f.prev = x;
                    if (dir <= 0)
                        xp.left = x;
                    else
                        xp.right = x;
                    // 优先将新节点染为红色
                    if (!xp.red)
                        x.red = true;
                    else {
                        lockRoot();
                        try {
                            root = balanceInsertion(root, x);
                        } finally {
                            unlockRoot();
                        }
                    }
                    break;
                }
            }
            assert checkInvariants(root);
            return null;
        }
}

// 红黑树的平衡插入
static <K,V> TreeNode<K,V> balanceInsertion(TreeNode<K,V> root,
                                                    TreeNode<K,V> x) {
            x.red = true; // 将x染成红色
            for (TreeNode<K,V> xp, xpp, xppl, xppr;;) {
	            // 根节点必须是黑色
                if ((xp = x.parent) == null) {
                    x.red = false;
                    return x;
                }
                // 父节点是黑色或者父节点是根节点
                // 总之父节点是黑色，那么不会违反红黑树性质
                // 不需要调整结构，直接返回根节点即可
                else if (!xp.red || (xpp = xp.parent) == null)
                    return root;
                // 父节点是红色（需要调整），且在祖父节点的左子树中
                if (xp == (xppl = xpp.left)) {
	                // 因为父节点为红色，所以xppr必须是红色或空，不可能是黑色
	                // 祖父节点的右节点为红色
                    if ((xppr = xpp.right) != null && xppr.red) {
                    
                   /**
		             *     黑                  红
		             *    /  \    （染色后）    / \
		             *   红   红    ->        黑  黑
		             *  /                   /
		             * 红                  红
		             * 
		             * 可见通过调整颜色后，子树不需要旋转就可以满足红黑树的性质
		             * 但由于xpp变成了红色，有可能违反红黑树性质，仍然需要向上调整
                    */
                    
                        xppr.red = false;
                        xp.red = false;
                        xpp.red = true;
                        x = xpp;
                    }
                    // xppr是空
                    else {
	                   /**
	                    *      黑
	                    *     /
	                    *    红 
	                    *      \
	                    *       红
	                    */
                        if (x == xp.right) {
	                        /**
                                * 进行左旋操作，变为以下形式，
                                * 可以看出此时任然违反红黑树的性质，
                                * 然而x仍然指向了最下面冲突的红色节点，
                                * 此处仅仅调整了树的形状
                                *
                                *      黑
                                *     /
                                *    红
                                *   /
                                *  红
                                */
                            root = rotateLeft(root, x = xp);
                            xpp = (xp = x.parent) == null ? null : xp.parent;
                        }
                        /*
                         * 由于调整了树的形状，因此此时树一定长成这个样子
                         * 
                         *      黑
                         *     /
                         *    红
                         *   /
                         *  红
                         * 
                         * 在染色并右旋之后，变为
                         * 
                         *    黑
                         *   /  \
                         * 红     红
                         */
                        if (xp != null) {
                            xp.red = false;
                            if (xpp != null) {
                                xpp.red = true;
                               
                                root = rotateRight(root, xpp);
                            }
                        }
                    }
                }
                // x在祖父节点的右子树中，这种情况与x在祖父节点左子树中类似，因此不多作解释，不明白的话类比即可。
                else {
                /**
                *   黑                    红
                *  /  \     (染色后)      / \
                * 红    红   ->         黑   黑
                *        \                    \
                *         红                   红色
                */
                    if (xppl != null && xppl.red) {
                        xppl.red = false;
                        xp.red = false;
                        xpp.red = true;
                        x = xpp;
                    }
                    else {
                        if (x == xp.left) {
                            root = rotateRight(root, x = xp);
                            xpp = (xp = x.parent) == null ? null : xp.parent;
                        }
                        if (xp != null) {
                            xp.red = false;
                            if (xpp != null) {
                                xpp.red = true;
                                root = rotateLeft(root, xpp);
                            }
                        }
                    }
                }
            }
        }
```

## 核心成员

```
	// ForwardingNode的hash值都是-1
	static final int MOVED     = -1; 
    // Treebin的hash值是-1
    static final int TREEBIN   = -2; 
    
	/**
     * 在第一次insert的时候才进行初始化(延迟初始化)
     * Size总是2的幂. 直接通过迭代器访问.
     */
    transient volatile Node<K,V>[] table;

    // nextTable的用途：只有在扩容时是非空的
    private transient volatile Node<K,V>[] nextTable;

    /**
     * Base counter value, used mainly when there is no contention,
     * but also as a fallback during table initialization
     * races. Updated via CAS.
     */
    private transient volatile long baseCount;

    /**
     * sizeCtl是控制标识符，不同的值表示不同的意义。
	 * -1代表正在初始化； 
	 * -(1+有效扩容线程的数量)，比如，-N 表示有N-1个线程正在进行扩容操作；
	 * 0 表示还未进行初始化
	 * 正数代表初始化或下一次进行扩容的大小，类似于扩容阈值。它的值始终是当前ConcurrentHashMap容量的0.75倍，这与loadfactor是对应的。实际容量>=sizeCtl，则扩容。
     */
    private transient volatile int sizeCtl;
    

     // 扩容的时候，next数组下标+1
    private transient volatile int transferIndex;

    /**
     * Spinlock (locked via CAS) used when resizing and/or creating CounterCells.
     */
    private transient volatile int cellsBusy;

    /**
     * Table of counter cells. When non-null, size is a power of 2.
     */
    private transient volatile CounterCell[] counterCells;

    // 视图
    private transient KeySetView<K,V> keySet;
    private transient ValuesView<K,V> values;
    private transient EntrySetView<K,V> entrySet;
```

## 核心函数

### ConcurrentHashMap(int initialCapacity)

之所以列出这个函数，是因为这个函数初始化了sizeCtl，并且可以看出table在这里并没有被初始化，而是在插入元素的时候进行延迟初始化。
我们要注意的是table的长度始终是2的幂，sizeCtl的值为正数时表示扩容的最小阀值。

```
 // 需要注意的是，构造了一个能够容纳initialCapacity个元素的对象，
 // 但实际table的大小比1.5倍的initialCapacity还多
 public ConcurrentHashMap(int initialCapacity) {
        if (initialCapacity < 0)
            throw new IllegalArgumentException();
        // 保证cap是2的幂，其中tableSizeFor返回大于入参的最小的2的幂
        int cap = ((initialCapacity >= (MAXIMUM_CAPACITY >>> 1)) ?
                   MAXIMUM_CAPACITY :
                   tableSizeFor(initialCapacity + (initialCapacity >>> 1) + 1));
        this.sizeCtl = cap;
    }
```

### initTable

```
     // 初始化table，使用sizeCtl记录table的容量
     // 为了保证并发访问不会出现冲突，使用了Unsafe的CAS操作
    private final Node<K,V>[] initTable() {
        Node<K,V>[] tab; int sc;
        // tab是空的
        while ((tab = table) == null || tab.length == 0) {
	        // 如果已经初始化过
            if ((sc = sizeCtl) < 0)
                Thread.yield(); // 退出初始化数组的竞争; just spin
            // 如果没有线程在初始化，将sizeCtl设置为-1，表示正在初始化
            // CAS操作，由此可见sizeCtl维护table的并发访问
            else if (U.compareAndSwapInt(this, SIZECTL, sc, -1)) {
                try {
	                // 再次检查table是否为空
                    if ((tab = table) == null || tab.length == 0) {
	                    // 计算分配多少个Node
	                    // sc大于0的时候表示要分配的大小
	                    // 否则默认分配16个node
                        int n = (sc > 0) ? sc : DEFAULT_CAPACITY;
                        @SuppressWarnings("unchecked")
                        Node<K,V>[] nt = (Node<K,V>[])new Node<?,?>[n];
                        table = tab = nt;
                        // 下次扩容的最小阀值0.75*n
                        // 注意0.75 * n < n，而且它很可能不是2的幂，
                        // 例如n = 16， 则sc = 12；
                        // 因此这个阀值在后续扩容情况下实际上不会成为数组的容量值，但它可以用来能保证用户提供了容量大小时，能够容纳用户要求数目的元素。
                        sc = n - (n >>> 2);
                    }
                } finally {
                    sizeCtl = sc;
                }
                break;
            }
        }
        return tab;
    }
```

### put

put过程的描述：

为表述方便，用符号i 来表示 (n - 1) & hash，用newNode表示使用key,value创建的节点

```
loop:
{
	if table == null
	{
		初始化一个默认长度为16的数组
	}
	else table[i] == null
	{	
		table[i] = newNode
	}
	else hash == -1，table[i]是ForwardingNode
	{
		进行整合表的操作
	}
	else
	{
		if hash >= 0，table[i]不是特殊Node(链表中的Node)
		{
			将newNode插入到链表中
		}
		else table[i]是TreeBin
		{
			 newNode插入到TreeNode中
		}
	}
	addCount(1L, binCount);
}

```

通过研读代码，发现Doug Lea使用了一种有效且高效的技巧：
在循环里面嵌套使用CAS操作。这种技巧把临界区变得很小，因此比较高效。

put源码如下：

```
	public V put(K key, V value) {
        return putVal(key, value, false);
    }

/** put和putIfAbsent都是通过调用putVal方法来实现的*/
    final V putVal(K key, V value, boolean onlyIfAbsent) {
	    // ConcurrentHashMap不支持key和value是null
        if (key == null || value == null) throw new NullPointerException();
        // 获取hash值
        int hash = spread(key.hashCode());
        int binCount = 0;
        for (Node<K,V>[] tab = table;;) {
            Node<K,V> f; int n, i, fh;
            // case 1：tab为null，需要初始化tab
            if (tab == null || (n = tab.length) == 0)
                tab = initTable();
            // case 2: 没有任何节点hash值与当前要插入的节点相同
            else if ((f = tabAt(tab, i = (n - 1) & hash)) == null) {
                if (casTabAt(tab, i, null,
                             new Node<K,V>(hash, key, value, null)))
                    break;                   // no lock when adding to empty bin
            }
            // case 3: 当遇到表连接点时，需要进行整合表的操作
            // 需要注意的是，遇到连接点的时候，并没有插入新节点，仅仅帮助扩容，因为当前线程迫切需要尽快插入新节点，只能等待扩容完毕才有可能插入新节点
            else if ((fh = f.hash) == MOVED)
                tab = helpTransfer(tab, f);
            // case 4: 找到对应于hash值的链表首节点，且该节点不是连接节点
            else {
                V oldVal = null;
                synchronized (f) {
                    if (tabAt(tab, i) == f) {
                        if (fh >= 0) {
                            binCount = 1;
                            for (Node<K,V> e = f;; ++binCount) {
                                K ek;
                                // 如果找到相同key的node，根据onlyIfAbsent来更新node的值
                                if (e.hash == hash &&
                                    ((ek = e.key) == key ||
                                     (ek != null && key.equals(ek)))) {
                                    oldVal = e.val;
                                    if (!onlyIfAbsent)
                                        e.val = value;
                                    break;
                                }
                                // 如果一直到链表的尾部都没有找到任何node的key与key相同，就插入到链表的尾部
                                Node<K,V> pred = e;
                                if ((e = e.next) == null) {
                                    pred.next = new Node<K,V>(hash, key,
                                                              value, null);
                                    break;
                                }
                            }
                        }
                        // 如果该节点是TreeBin，就插入到TreeBin中
                        else if (f instanceof TreeBin) {
                            Node<K,V> p;
                            binCount = 2;
                            // 当存在相同的key时，putTreeVal不会修改那个TreeNode，而是返回给p，由onlyIfAbsent决定是否修改p.val
                            if ((p = ((TreeBin<K,V>)f).putTreeVal(hash, key,
                                                           value)) != null) {
                                oldVal = p.val;
                                if (!onlyIfAbsent)
                                    p.val = value;
                            }
                        }
                    }
                }
                // 若链表长度不低于8，就将链表转换为树
                if (binCount != 0) {
                    if (binCount >= TREEIFY_THRESHOLD)
                        treeifyBin(tab, i);
                    if (oldVal != null)
                        return oldVal;
                    break;
                }
            }
        }
        // 添加计数，如有需要，扩容
        addCount(1L, binCount);
        return null;
    }

	// 给tab[i]赋值
	// 如果tab[i]等于c,就将tab[i]与v交换数值
	static final <K,V> boolean casTabAt(Node<K,V>[] tab, int i,
                                        Node<K,V> c, Node<K,V> v) {
        return U.compareAndSwapObject(tab, ((long)i << ASHIFT) + ABASE, c, v);
    }
    
    /**
    * 协助扩容方法。
    * 多线程下，当前线程检测到其他线程正进行扩容操作，则协助其一起扩容；
    *（只有这种情况会被调用）从某种程度上说，其“优先级”很高，
    * 只要检测到扩容，就会放下其他工作，先扩容。
	* 调用之前，nextTable一定已存在。
	*/
	final Node<K,V>[] helpTransfer(Node<K,V>[] tab, Node<K,V> f) {
        Node<K,V>[] nextTab; int sc;
        // 如果f是tab中的连接节点，并且它所连接的table非空
        if (tab != null && (f instanceof ForwardingNode) &&
            (nextTab = ((ForwardingNode<K,V>)f).nextTable) != null) {
            // 标志位
            int rs = resizeStamp(tab.length);
            // 当正在扩容时，帮助扩容
            while (nextTab == nextTable && table == tab &&
                   (sc = sizeCtl) < 0) {
                if ((sc >>> RESIZE_STAMP_SHIFT) != rs || sc == rs + 1 ||
                    sc == rs + MAX_RESIZERS || transferIndex <= 0)
                    break;
                if (U.compareAndSwapInt(this, SIZECTL, sc, sc + 1)) {
                    transfer(tab, nextTab);
                    break;
                }
            }
            return nextTab;
        }
        return table;
    }

```

### get

get方法比较简单，没有使用锁，而是用Unsafe来保证获取的头结点是volatile的

```
 public V get(Object key) {
        Node<K,V>[] tab; Node<K,V> e, p; int n, eh; K ek;
        // 获取hash值h
        int h = spread(key.hashCode());
        // tab只是保存了hash值相同的头结点
        if ((tab = table) != null && (n = tab.length) > 0 && // table里面有元素
            (e = tabAt(tab, (n - 1) & h)) != null) {// 根据h来获取头结点e
            // hash值相同，如果找到key，直接返回
            if ((eh = e.hash) == h) {
                if ((ek = e.key) == key || (ek != null && key.equals(ek)))
                    return e.val;
            }
            // todo：看一下hash值什么时候小于0
            else if (eh < 0)
                return (p = e.find(h, key)) != null ? p.val : null;
            while ((e = e.next) != null) {
                if (e.hash == h &&
                    ((ek = e.key) == key || (ek != null && key.equals(ek))))
                    return e.val;
            }
        }
        return null;
    }
    
//tableAt方法使用了Unsafe对象来获取数组中下标为i的对象
static final <K,V> Node<K,V> tabAt(Node<K,V>[] tab, int i) {
		// 第i个元素实际地址i * (2^ASHIFT) + ABASE
        return (Node<K,V>)U.getObjectVolatile(tab, ((long)i << ASHIFT) + ABASE);
    }
```

### treeifyBin

```
     // 如果tab的长度很小，小于64个，就尝试进行扩容为两倍，
     // 否则就将以tab[index]开头的链表转换为Treebin
    private final void treeifyBin(Node<K,V>[] tab, int index) {
        Node<K,V> b; int n, sc;
        if (tab != null) {
	        // tab的长度小于64，就尝试进行扩容
            if ((n = tab.length) < MIN_TREEIFY_CAPACITY)
                tryPresize(n << 1);
            else if ((b = tabAt(tab, index)) != null && b.hash >= 0) {
                synchronized (b) {
                    if (tabAt(tab, index) == b) {
                        TreeNode<K,V> hd = null, tl = null;
                        // 这个循环建立了TreeNode中的双向链表，hd保存了双向链表的头结点
                        for (Node<K,V> e = b; e != null; e = e.next) {
                            TreeNode<K,V> p =
                                new TreeNode<K,V>(e.hash, e.key, e.val,
                                                  null, null);
                            if ((p.prev = tl) == null)
                                hd = p;
                            else
                                tl.next = p;
                            tl = p;
                        }
                        setTabAt(tab, index, new TreeBin<K,V>(hd));
                    }
                }
            }
        }
    }
```

### tryPresize

有关扩容，可以参考[深入分析 ConcurrentHashMap 1.8 的扩容实现](http://www.jianshu.com/p/f6730d5784ad) 这篇文章。

```
	// 尝试扩容使它能放size个元素
    private final void tryPresize(int size) {
	    // 计算扩容后的数量
        int c = (size >= (MAXIMUM_CAPACITY >>> 1)) ? MAXIMUM_CAPACITY :
            tableSizeFor(size + (size >>> 1) + 1);
        int sc;
        while ((sc = sizeCtl) >= 0) {
            Node<K,V>[] tab = table; int n;
            // 如果tab是空的，直接扩容
            if (tab == null || (n = tab.length) == 0) {
	            // 计算扩容后的容量
                n = (sc > c) ? sc : c;
                if (U.compareAndSwapInt(this, SIZECTL, sc, -1)) {
                    try {
                        if (table == tab) {
                            @SuppressWarnings("unchecked")
                            Node<K,V>[] nt = (Node<K,V>[])new Node<?,?>[n];
                            table = nt;
                            // 下次扩容的容量阀值是0.75 * n
                            sc = n - (n >>> 2);
                        }
                    } finally {
                        sizeCtl = sc;
                    }
                }
            }
            // 容量已经够用，不需要进行扩容；或者容量太大，无法进行扩容。
            else if (c <= sc || n >= MAXIMUM_CAPACITY)
                break;
            // 仍然需要扩容
            else if (tab == table) {
                int rs = resizeStamp(n);
                // todo：不是很懂为什么会出现 sc < 0 ？先看一下transfer的实现
                if (sc < 0) {
                    Node<K,V>[] nt;
                    if ((sc >>> RESIZE_STAMP_SHIFT) != rs || sc == rs + 1 ||
                        sc == rs + MAX_RESIZERS || (nt = nextTable) == null ||
                        transferIndex <= 0)
                        break;
                    if (U.compareAndSwapInt(this, SIZECTL, sc, sc + 1))
                        transfer(tab, nt);
                }
                else if (U.compareAndSwapInt(this, SIZECTL, sc,
                                             (rs << RESIZE_STAMP_SHIFT) + 2))
                    transfer(tab, null);
            }
        }
    }
```

### transfer

```
伪代码：

n = table.length

nextTable = new Node[2 * n]

forwardingNode = new ForwardingNode

forwardingNode.nextTable = nextTable;

for(table[i] : table)
{
	for(p = table[i]; p != null ; p = p.next)
	{
		if(p.hash & n == 0)
			将p放入nextTable[i]的数据集合中
		else
			将p放入nextTable[i+n]的数据集合中
	}
	table[i] = forwardingNode;
}

table = nextTable;

nextTable = null;
```



数学公式：
> 已知：n = 2 ^ k ， hash & (n-1) = i，显而易见：
> （1）若 hash & n = 0， 则 hash &(2*n - 1) = i ；
> （2）若 hash & n != 0,  则 hash&(2*n - 1) = i + n。

源代码在此：

```
     // 把table中所有的Node放入新的table中
    private final void transfer(Node<K,V>[] tab, Node<K,V>[] nextTab) {
        int n = tab.length, stride;
        if ((stride = (NCPU > 1) ? (n >>> 3) / NCPU : n) < MIN_TRANSFER_STRIDE)
            stride = MIN_TRANSFER_STRIDE; // subdivide range
        if (nextTab == null) {            // initiating
            try {
                @SuppressWarnings("unchecked")
                Node<K,V>[] nt = (Node<K,V>[])new Node<?,?>[n << 1];
                nextTab = nt;
            } catch (Throwable ex) {      // try to cope with OOME
                sizeCtl = Integer.MAX_VALUE;
                return;
            }
            nextTable = nextTab;
            transferIndex = n;
        }
        int nextn = nextTab.length;
        ForwardingNode<K,V> fwd = new ForwardingNode<K,V>(nextTab);
        boolean advance = true;
        boolean finishing = false; // to ensure sweep before committing nextTab
        for (int i = 0, bound = 0;;) {
            Node<K,V> f; int fh;
            while (advance) {
                int nextIndex, nextBound;
                if (--i >= bound || finishing)
                    advance = false;
                else if ((nextIndex = transferIndex) <= 0) {
                    i = -1;
                    advance = false;
                }
                else if (U.compareAndSwapInt
                         (this, TRANSFERINDEX, nextIndex,
                          nextBound = (nextIndex > stride ?
                                       nextIndex - stride : 0))) {
                    bound = nextBound;
                    i = nextIndex - 1;
                    advance = false;
                }
            }
            if (i < 0 || i >= n || i + n >= nextn) {
                int sc;
                if (finishing) {
                    nextTable = null;
                    table = nextTab;
                    sizeCtl = (n << 1) - (n >>> 1);
                    return;
                }
                if (U.compareAndSwapInt(this, SIZECTL, sc = sizeCtl, sc - 1)) {
                    if ((sc - 2) != resizeStamp(n) << RESIZE_STAMP_SHIFT)
                        return;
                    finishing = advance = true;
                    i = n; // recheck before commit
                }
            }
            else if ((f = tabAt(tab, i)) == null)
                advance = casTabAt(tab, i, null, fwd);
            else if ((fh = f.hash) == MOVED)
                advance = true; // already processed
            else {
                synchronized (f) {
                    if (tabAt(tab, i) == f) {
                        Node<K,V> ln, hn;
                        if (fh >= 0) {
	                
                            int runBit = fh & n;
                            Node<K,V> lastRun = f;
                            for (Node<K,V> p = f.next; p != null; p = p.next) {
                                int b = p.hash & n;
                                if (b != runBit) {
                                    runBit = b;
                                    lastRun = p;
                                }
                            }
                            if (runBit == 0) {
                                ln = lastRun;
                                hn = null;
                            }
                            else {
                                hn = lastRun;
                                ln = null;
                            }
                            for (Node<K,V> p = f; p != lastRun; p = p.next) {
                                int ph = p.hash; K pk = p.key; V pv = p.val;
                                if ((ph & n) == 0)
                                    ln = new Node<K,V>(ph, pk, pv, ln);
                                else
                                    hn = new Node<K,V>(ph, pk, pv, hn);
                            }
                            setTabAt(nextTab, i, ln);
                            setTabAt(nextTab, i + n, hn);
                            setTabAt(tab, i, fwd);
                            advance = true;
                        }
                        else if (f instanceof TreeBin) {
                            TreeBin<K,V> t = (TreeBin<K,V>)f;
                            TreeNode<K,V> lo = null, loTail = null;
                            TreeNode<K,V> hi = null, hiTail = null;
                            int lc = 0, hc = 0;
                            for (Node<K,V> e = t.first; e != null; e = e.next) {
                                int h = e.hash;
                                TreeNode<K,V> p = new TreeNode<K,V>
                                    (h, e.key, e.val, null, null);
                                if ((h & n) == 0) {
                                    if ((p.prev = loTail) == null)
                                        lo = p;
                                    else
                                        loTail.next = p;
                                    loTail = p;
                                    ++lc;
                                }
                                else {
                                    if ((p.prev = hiTail) == null)
                                        hi = p;
                                    else
                                        hiTail.next = p;
                                    hiTail = p;
                                    ++hc;
                                }
                            }
                            ln = (lc <= UNTREEIFY_THRESHOLD) ? untreeify(lo) :
                                (hc != 0) ? new TreeBin<K,V>(lo) : t;
                            hn = (hc <= UNTREEIFY_THRESHOLD) ? untreeify(hi) :
                                (lc != 0) ? new TreeBin<K,V>(hi) : t;
                            setTabAt(nextTab, i, ln);
                            setTabAt(nextTab, i + n, hn);
                            setTabAt(tab, i, fwd);
                            advance = true;
                        }
                    }
                }
            }
        }
    }
```

### addCount

```
	/**
     * Adds to count, and if table is too small and not already
     * resizing, initiates transfer. If already resizing, helps
     * perform transfer if work is available.  Rechecks occupancy
     * after a transfer to see if another resize is already needed
     * because resizings are lagging additions.
     *
     * @param x the count to add
     * @param check if <0, don't check resize, if <= 1 only check if uncontended
     */
    // 添加计数，如果table太小且table没有在扩容，就进行扩容
    private final void addCount(long x, int check) {
        CounterCell[] as; long b, s;
        // 利用CAS快速更新baseCount的值
        if ((as = counterCells) != null ||
            !U.compareAndSwapLong(this, BASECOUNT, b = baseCount, s = b + x)) {
            CounterCell a; long v; int m;
            boolean uncontended = true;
            if (as == null || (m = as.length - 1) < 0 ||
                (a = as[ThreadLocalRandom.getProbe() & m]) == null ||
                !(uncontended =
                  U.compareAndSwapLong(a, CELLVALUE, v = a.value, v + x))) {
                fullAddCount(x, uncontended);
                return;
            }
            if (check <= 1)
                return;
            s = sumCount();
        }
        
        // 当之前检查的节点个数大于等于0时，才考虑扩容
        if (check >= 0) {
            Node<K,V>[] tab, nt; int n, sc;
            while (s >= (long)(sc = sizeCtl) && (tab = table) != null &&
                   (n = tab.length) < MAXIMUM_CAPACITY) {
                // 为当前的n保留一个数，不同的数组n（这里n=2^k）得到的结果必然不同，可类比时间戳
                int rs = resizeStamp(n);
                // 如果有线程正在扩容，就帮助其扩容
                if (sc < 0) {
                    if ((sc >>> RESIZE_STAMP_SHIFT) != rs || sc == rs + 1 ||
                        sc == rs + MAX_RESIZERS || (nt = nextTable) == null ||
                        transferIndex <= 0)
                        break;
                    if (U.compareAndSwapInt(this, SIZECTL, sc, sc + 1))
                        transfer(tab, nt);
                }
                // 没有线程在扩容，直接扩容
                else if (U.compareAndSwapInt(this, SIZECTL, sc,
                                             (rs << RESIZE_STAMP_SHIFT) + 2))
                    transfer(tab, null);
                s = sumCount();
            }
        }
    }
```
