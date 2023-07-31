## 定义

二分图，又称二部图，英文名叫 Bipartite graph。

二分图是什么？节点由两个集合组成，且两个集合内部没有边的图。

换言之，存在一种方案，将节点划分成满足以上性质的两个集合。

![](./images/bi-graph.svg)

## 性质

-   如果两个集合中的点分别染成黑色和白色，可以发现二分图中的每一条边都一定是连接一个黑色点和一个白色点。

-   ??? question "二分图不存在长度为奇数的环"
        因为每一条边都是从一个集合走到另一个集合，只有走偶数次才可能回到同一个集合。

## 判定

如何判定一个图是不是二分图呢？

换言之，我们需要知道是否可以将图中的顶点分成两个满足条件的集合。

显然，直接枚举答案集合的话实在是太慢了，我们需要更高效的方法。

考虑上文提到的性质，我们可以使用 [DFS（图论）](./dfs.md) 或者 [BFS](./bfs.md) 来遍历这张图。如果发现了奇环，那么就不是二分图，否则是。

## 应用

### 二分图最大匹配

详见 [二分图最大匹配](./graph-matching/bigraph-match.md) 页面。

### 二分图最大权匹配

详见 [二分图最大权匹配](./graph-matching/bigraph-weight-match.md) 页面。

### 一般图最大匹配

详见 [一般图最大匹配](./graph-matching/general-match.md) 页面。

### 一般图最大权匹配

详见 [一般图最大权匹配](./graph-matching/general-weight-match.md) 页面。