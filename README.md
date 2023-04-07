# mymuduo
Re-implement the muduo library based on C++11
直接使用 `autobuild.sh` 即可编译生成库文件 `libmymuduo.so`，在自己的程序下包含对应头文件即可使用

使用该库时需要连接如下库

```shell
-lmymuduo -lpthread
```

