# 博客运维

本站使用[mkdocs工具](https://mkdocs.org)搭建，模板使用[mkdocs-material](https://github.com/squidfunk/mkdocs-material)

# 环境搭建
```
pip install -r requirements.txt
pip install mkdocs-minify-plugin
```


# 添加文档
1. 在docs文档下添加markdown文件
2. 在mkdocs.yml中的nav中添加内容
3. mkdocs serve预览一下是否有问题
4. mkdocs gh-deploy发布

```
git add .
git commit -m 'feat;publish article'
git push
```

# 鸣谢
算法部分来自[OI-wiki](https://github.com/OI-wiki/OI-wik)项目，感谢[OI-wiki](https://github.com/OI-wiki/OI-wik)项目的无私贡献。
