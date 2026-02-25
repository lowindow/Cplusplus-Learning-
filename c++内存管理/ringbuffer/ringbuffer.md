Memory Order

内存顺序用于描述多线程环境下对共享内存读写的可见性与重排约束。

编译器与 CPU 为了优化性能，可能对指令进行重排序或延迟传播到其他核心。

单线程语义不会被破坏，但在多线程环境下可能导致：

其他线程观察到的执行顺序与源码顺序不一致。


比如SPSC中，生产者线程写 write_，消费者线程写 read_

生产者的write_指针 先对消费者可见，ringbuffer的data_数据后对消费者可见，此时消费者可能观察到生产者的指针更新了，但是数据并没有更新，处理的数据不是最新的数据

因此需要使用内存序（memory order）建立同步关系。


std::atomic的load()和store() 函数第二个参数可以指定内存顺序。

Relaxed ordering   单线程内对atomic变量的读写是顺序的，不同线程 不建立线程间同步，不保证顺序

Release-Acquire ordering A线程release(X)前的所有写操作，在B线程acquire(X)后都可见；
其他线程可见顺序上，A线程中release前的代码不可以被优化移到release后；B线程中acquire后的代码不可以被优化移到acquire之前。

volatile 不保证原子性，也不保证内存顺序，不能用于线程同步

