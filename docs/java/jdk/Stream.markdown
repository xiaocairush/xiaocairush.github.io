# JDK 源码剖析系列之 Stream

> 在读本文之前您必须知道 Stream 的基础用法。

## Stream 是什么

### 组成

一个 Stream 由三部分组成：

- source。也就是 Stream 数据的来源，可以是集合，数组，生成函数，I/O channel 等
- 任意数量的中间操作。例如 filter， map，flatmap 等操作
- 终止操作。分为两种，一种是 count 这种聚合操作可以产生一个结果，另一种是 forEach(Consumer)操作，产生某种副作用

### 特性

- 惰性计算（如果没有终止操作，则计算过程不会被执行，其实就是只生成 StatelessOp 这类代表计算过程的对象，真正的计算过程只有遇到终止操作才会被执行）
- 支持串行（单线程）和并行（多线程）两种方式进行聚合操作

## 主要流程

这里我们主要看下以下代码的流程：

```java
collection.stream().filter(xxx).count()
```

在看完`count()`之后，我们会看一下`collect()`，因为 collect 我们也经常用，有必要看一下。我们在看源码的过程中只关注主要流程，不会深入不重要的细节。

### 2.0 概述

Stream 使用了双向链表来实现，链表中的每一个节点（节点类型是`ReferencePipeline`）都对应了一个操作，`collection.stream()`方法其实就是创建了链表的头节点，`filter()`就是向链表中插入了一个中间节点，`count()`方法就是向链表中插入了名为 ReduceOp 的尾节点，尾节点负责创建 sink 对象（提供`makeSink()`方法），sink 对象就是用来接收 Stream 中流过来的元素的，它在`accept`元素的时候就可以完成统计行为。链表中除了头节点和尾节点之外的所有节点都重写了`opWrapSink(sink)`方法，这个方法可以对 sink 进行装饰来修改它的 accept()行为，例如装饰后的 sink 拒绝接收不满足条件的元素，这样就可以实现过滤功能了。

### 2.1`collection.stream()`的实现

```java

Collection类：
    default Stream<E> stream() {
        return StreamSupport.stream(spliterator(), false);
    }

    @Override
    default Spliterator<E> spliterator() {
        return Spliterators.spliterator(this, 0);
    }

 StreamSupport类：
     public static <T> Stream<T> stream(Spliterator<T> spliterator, boolean parallel) {
        Objects.requireNonNull(spliterator);
        return new ReferencePipeline.Head<>(spliterator,
                                            StreamOpFlag.fromCharacteristics(spliterator),
                                            parallel);
    }
```

`Spliterators.spliterator(this, 0)` 这个方法实际上创建了一个`IteratorSpliterator`对象，目前仅需关注该对象持有了 Stream 源头的 Collection 对象，它有`tryAdvance()`和`trySplit()`这两个方法，后续会讲解这两个方法的细节。

另外我们注意到`StreamSupport.stream()`只是创建了` ReferencePipeline.Head`这样一个对象，这个对象实际上是`ReferencePipeline`的子类，而`ReferencePipeline`维护了一个双向链表，` ReferencePipeline.Head`则是这个链表的头节点。

### 2.2 `filter(xxx)`的实现

上面我们讲到了`ReferencePipeline`这个类，其实这个类实现了 Stream 接口，它是 Stream 的核心实现类。`filter()`
就是在这个类中实现的。

```java
   @Override
    public final Stream<P_OUT> filter(Predicate<? super P_OUT> predicate) {
        return new StatelessOp<P_OUT, P_OUT>(this, StreamShape.REFERENCE,
                                     StreamOpFlag.NOT_SIZED) {
            @Override
            Sink<P_OUT> opWrapSink(int flags, Sink<P_OUT> sink) {
                return new Sink.ChainedReference<P_OUT, P_OUT>(sink) {
                    @Override
                    public void begin(long size) {
                        downstream.begin(-1);
                    }

                    @Override
                    public void accept(P_OUT u) {
                        if (predicate.test(u))
                            downstream.accept(u);
                    }
                };
            }
        };
    }

```

这里我们仅需关注`filter()`只是创建了一个`StatelessOp`对象，这个`StatelessOp`实际上就是`ReferencePipeline`的一个子类。

因此：Stream 的方法例如 `filter()`不会真正进行计算，只是为这个计算过程创建了一个对象。Stream 实际上就是一个 pipeline，而这个 pipeline 实际上就是一个双向链表，`StatelessOp`的构造函数，实际上就是在链表中创建了一个节点，并拼接到双向链表中去。

```java
StatelessOp类：
    abstract static class StatelessOp<E_IN, E_OUT>
            extends ReferencePipeline<E_IN, E_OUT> {

        StatelessOp(AbstractPipeline<?, E_IN, ?> upstream,
                    StreamShape inputShape,
                    int opFlags) {
            super(upstream, opFlags);
            assert upstream.getOutputShape() == inputShape;
        }
    }

ReferencePipeline类：
abstract class ReferencePipeline<P_IN, P_OUT>
        extends AbstractPipeline<P_IN, P_OUT, Stream<P_OUT>>
        implements Stream<P_OUT>  {
    ReferencePipeline(AbstractPipeline<?, P_IN, ?> upstream, int opFlags) {
        super(upstream, opFlags);
    }
}

AbstractPipeline类：
    AbstractPipeline(AbstractPipeline<?, E_IN, ?> previousStage, int opFlags) {
        previousStage.nextStage = this;
        this.previousStage = previousStage;
        this.sourceStage = previousStage.sourceStage;
        this.depth = previousStage.depth + 1;
    }
```

### 2.3 count()的实现

```java
ReferencePipeline类：
    @Override
    public final long count() {
        return evaluate(ReduceOps.makeRefCounting());
    }

   final <R> R evaluate(TerminalOp<E_OUT, R> terminalOp) {
        return isParallel()
               ? terminalOp.evaluateParallel(this, sourceSpliterator(terminalOp.getOpFlags()))
               : terminalOp.evaluateSequential(this, sourceSpliterator(terminalOp.getOpFlags()));
    }`
```

目前我们只需要关注`terminalOp.evaluateSequential()`这个方法即可，明白这个方法之后，理解`terminalOp.evaluateParallel`就非常简单了。

```java
ReduceOps类：
    public static <T> TerminalOp<T, Long>
    makeRefCounting() {
        return new ReduceOp<T, Long, CountingSink<T>>(StreamShape.REFERENCE) {
            @Override
            public CountingSink<T> makeSink() { return new CountingSink.OfRef<>(); }

            @Override
            public <P_IN> Long evaluateSequential(PipelineHelper<T> helper,
                                                  Spliterator<P_IN> spliterator) {
                return super.evaluateSequential(helper, spliterator);
            }
        };
    }

ReduceOp类：
private abstract static class ReduceOp<T, R, S extends AccumulatingSink<T, R, S>>
            implements TerminalOp<T, R> {
        @Override
        public <P_IN> R evaluateSequential(PipelineHelper<T> helper,
                                           Spliterator<P_IN> spliterator) {
            return helper.wrapAndCopyInto(makeSink(), spliterator).get();
        }
}

```

```java
    abstract static class CountingSink<T>
            extends Box<Long>
            implements AccumulatingSink<T, Long, CountingSink<T>> {
        long count;

        @Override
        public void begin(long size) {
            count = 0L;
        }

        @Override
        public Long get() {
            return count;
        }

        @Override
        public void combine(CountingSink<T> other) {
            count += other.count;
        }

        static final class OfRef<T> extends CountingSink<T> {
            @Override
            public void accept(T t) {
                count++;
            }
        }
    }
```

可以看到`terminalOp.evaluateSequential()`主要实现在`ReduceOp`这个类中，尾节点`ReduceOp`提供了`makeSink()`方法，该方法返回了一个`CountingSink.OfRef<>()`类型的 sink 对象。这里解释一下 sink 这个概念，sink 中文翻译过来就是水池，形象点理解的话，就是打开水龙头后，水池的水会不断累积起来，水池的状态不断发生变化。这里我们可以看到，sink 有 accept 这样一个方法接收从源头流过来的元素，你可以类比理解为水池接水。这里 sink 对象 begin()方法确定了初始状态，accept()方法是实现了计数功能。那么这个 begin()和 accept()是在哪里被调用的呢？也就是说水龙头是在哪被打开的呢？在`evaluateSequential`这个方法中有`helper.wrapAndCopyInto(makeSink(), spliterator)`这样一个函数调用，这个函数调用实现了初始化 sink 状态和统计的功能。我们继续看看这个函数的实现：

```java
abstract class AbstractPipeline<E_IN, E_OUT, S extends BaseStream<E_OUT, S>>
        extends PipelineHelper<E_OUT> implements BaseStream<E_OUT, S> {

    @Override
    final <P_IN, S extends Sink<E_OUT>> S wrapAndCopyInto(S sink, Spliterator<P_IN> spliterator) {
        copyInto(wrapSink(Objects.requireNonNull(sink)), spliterator);
        return sink;
    }

    @Override
    final <P_IN> Sink<P_IN> wrapSink(Sink<E_OUT> sink) {
        for (AbstractPipeline p=AbstractPipeline.this; p.depth > 0; p=p.previousStage) {
            sink = p.opWrapSink(p.previousStage.combinedFlags, sink);
        }
        return (Sink<P_IN>) sink;
    }

    @Override
    final <P_IN> void copyInto(Sink<P_IN> wrappedSink, Spliterator<P_IN> spliterator) {
         copyIntoWithCancel(wrappedSink, spliterator);
    }

    @Override
    final <P_IN> boolean copyIntoWithCancel(Sink<P_IN> wrappedSink, Spliterator<P_IN> spliterator) {
        AbstractPipeline p = AbstractPipeline.this;
        while (p.depth > 0) {
            p = p.previousStage;
        }

        wrappedSink.begin(spliterator.getExactSizeIfKnown());
        boolean cancelled = p.forEachWithCancel(spliterator, wrappedSink);
        wrappedSink.end();
        return cancelled;
    }

}
```

在`wrapSink`中对链表进行了逆序遍历，不断使用`opWrapSink`方法对当前的 sink 进行装饰，很明显这里就是装饰器模式。在 2.2 部分讲解 filter()的时候，我们创建的`StatelessOp`这个对象就含有这个`opWrapSink`装饰方法，这个方法将 sink
装饰为`Sink.ChainedReference`，装饰后得到的 sink 对象的 accept()这个行为相对装饰前的 sink 对象发生了变化，即过滤掉了不符合条件的元素。

另外，在`copyInto`中，我们可以看到对 wrappedSink 对`begin()`和`end()`方法的调用，同时调用了` p.forEachWithCancel(spliterator, wrappedSink);`这个方法，我们看看这个方法做了什么：

```java
ReferencePipeline类：

    @Override
    final boolean forEachWithCancel(Spliterator<P_OUT> spliterator, Sink<P_OUT> sink) {
        boolean cancelled;
        do { } while (!(cancelled = sink.cancellationRequested()) && spliterator.tryAdvance(sink));
        return cancelled;
    }
```

我们注意到 `spliterator.tryAdvance(sink)`这个方法，还记得在 2.1 中我们提到的`IteratorSpliterator`这个对象吗，没错， `spliterator.tryAdvance(sink)`这个方法就是这个对象实现的。

```java
IteratorSpliterator类：
       @Override
        public boolean tryAdvance(Consumer<? super T> action) {
            if (it == null) {
                it = collection.iterator();
                est = (long) collection.size();
            }
            if (it.hasNext()) {
                action.accept(it.next());
                return true;
            }
            return false;
        }

```

我们可以看到，`tryAdvance`就是使用迭代器遍历集合中的每个元素，而 wrappedSink 实际上就是作为消费者消费（`accept`）这个元素的过程。
至此，`count()`的实现我们就看完了。

接下来我们来看下 Stream 中`collect(Collector.toList())`的实现

### 2.4 `collect(Collectors.toList())`的实现

```java
abstract class ReferencePipeline<P_IN, P_OUT>
        extends AbstractPipeline<P_IN, P_OUT, Stream<P_OUT>>
        implements Stream<P_OUT>  {

    public final <R, A> R collect(Collector<? super P_OUT, A, R> collector) {
        A container;
 		...省略不重要的细节
         container = evaluate(ReduceOps.makeRef(collector));
         ...省略不重要的细节
        return collector.finisher().apply(container);
    }

}
```

`evaluate`方法和之前 2.3 中分析的方法是同一个方法，即对 sink 对象进行装饰，然后调用装饰后的 sink 对象的 accept 方法，因此这里我们只需关注`ReduceOps.makeRef(collector)`这个方法返回了一个什么样的 sink 对象即可：

```java
ReduceOps类：
    public static <T, I> TerminalOp<T, I>
    makeRef(Collector<? super T, I, ?> collector) {
        Supplier<I> supplier = Objects.requireNonNull(collector).supplier();
        BiConsumer<I, ? super T> accumulator = collector.accumulator();
        BinaryOperator<I> combiner = collector.combiner();
        class ReducingSink extends Box<I>
                implements AccumulatingSink<T, I, ReducingSink> {
            @Override
            public void begin(long size) {
                state = supplier.get();
            }

            @Override
            public void accept(T t) {
                accumulator.accept(state, t);
            }

            @Override
            public void combine(ReducingSink other) {
                state = combiner.apply(state, other.state);
            }
        }
        return new ReduceOp<T, I, ReducingSink>(StreamShape.REFERENCE) {
            @Override
            public ReducingSink makeSink() {
                return new ReducingSink();
            }
        };
    }

 Collectors类：
    public static <T>
    Collector<T, ?, List<T>> toList() {
        return new CollectorImpl<>((Supplier<List<T>>) ArrayList::new, List::add,
                                   (left, right) -> { left.addAll(right); return left; },
                                   CH_ID);
    }
```

可以看到 collect 操作只是返回了一个`ReduceOp`对象，这个对象封装了 reduce 这个计算过程。

- collector 中的`supplier()`提供了`ReducingSink`的初始状态。例如 state 初始化为一个空的 ArrayList。
- sink 接收的一个元素 t 后，状态会发生变化，collector 中的`accumulator()`封装了状态将怎样变化。例如对 list 调用 add 方法。
- collector 中的`combiner()`封装了怎样将两个 sink 的计算结果进行合并。例如两个 sink 的计算结果都是 list，只需要把第二个 list 中所有的元素全部添加到第一个 list 中，这样得到的结果就是整个计算过程的结果。
- collector 中的 finisher()就是对计算结果进行再次加工。例如 Stream 计算结果是 `List<Object>`类型的，finisher 可以将它转换为`List<Integer>`类型。

## 总结

- Stream 的核心实现类是 ReferencePipeline。
- Stream 实际上是一个双向链表，链表中每个节点的类型是 ReferencePipeline，每个节点对应了一个特定的计算过程，例如 ReferencePipeline.Head（链表的第一个节点）， StatelessOp（例如 filter()操作，一般作为流中的中间计算过程）、ReduceOp(例如 count()，collect()操作，它是一个 TerminalOp)等。
- 除了 TerminalOp，每个计算过程（即链表中的每个节点）例如 StatelessOp 都支持装饰器模式，重写了`opWrapSink(sink)`对 sink 进行装饰，修改 sink 对象的 accept()行为，例如 filter 对应的 StatelessOp 中如果 sink 接收的对象不符合条件，那么 sink 拒绝接收该对象。
- 最终`Spliterator`中的`tryAdvance()`将会不断推进计算过程的真正执行。也就是遍历 stream 源头的所有元素，将每一个元素递交给经过装饰的 sink 对象，调用`wrappedSink.accept()`
- sink 装饰器链层层调用，得到 Stream 最终的计算结果。

---

欢迎关注我的公众号“**窗外月明**”，原创技术文章第一时间推送。

<center>
    <img src="https://open.weixin.qq.com/qr/code?username=gh_c36a67dfc3b3" style="width: 100px;">
</center>
