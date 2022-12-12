# Memory allocator

## Build

1. Build
```shell
$ make
```
You can override the build by setting the `CFALGS` environment variable

2. Clang-tidy check
```shell
make clang-tidy
```

## Run
```shell
$ ./build/alloc
```

# Дополнительные материалы

- Ulrich Drepper. ["What every programmer should know about memory"](https://people.freebsd.org/~lstewart/articles/cpumemory.pdf)
- [`man mmap` online](https://man7.org/linux/man-pages/man2/mmap.2.html)
- [Doug Lee's article about allocator works in `glibc`](http://gee.cs.oswego.edu/dl/html/malloc.html) 

