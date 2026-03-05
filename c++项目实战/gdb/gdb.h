#pragma once 

#include <iostream>
#include <cstring>


/*
    什么是coredump文件？                
        核心转储文件 进程运行时，突然崩溃的一瞬间进程在内存中的快照，会把进程此刻的内存 寄存器状态、运行堆栈等信息转储保存在一个文件里,
    程序员后续可以通过coredump文件进行查看进程崩溃的原因，进而修复代码bug
    产生不了coredump怎么办 ？
          ulimit -a               # 查看core file size
          ulimit -c unlimited    # 设置core file size  
          此时是否能产生core文件 需要根据磁盘大小空间而定
          ubuntu的默认策略是将这个核心文件丢给一个apport程序进行处理,存放在默认路径，需要设置一下 才能在当前文件下产生核心文件 
          sudo sysctl -w kernel.core_pattern=./core.%e.%p # 设置coredump文件产生路径和名字

    如何使用gdb调试coredump文件？ 
    调试：
        编译 g++ -g 
        运行： gdb [可执行程序] [coredump 文件]
    思路:
        i.查看调用堆栈寻找崩溃原因   
            bt            查看
            f [堆栈编号] 切换到堆栈中去
            p 输出变量  在崩溃的时候值

        ii.根据崩溃点 查找代码 分析原因
        iii. 修复bug
*/
void func(char *ptr)
{
    strcpy(ptr,"my name is xiaofei");
}

/*
    如何使用gdb调试多线程程序?
    gdb 进入多线程程序

    （gdb）l [文件名]:1,100 # 查看哪个文件 行范围内的源码
    1.合适的位置设置断点 
     (gdb) b [文件名]:20  # 在指定个文件多少行打断点
     (gdb) n/c           # 单步/下一个断点处
    i.查看线程信息 info thread  ii.t + 线程id进行切换 
    strcpy(ptr,"my name is xiaofei");iii.切换到具体的某一层调用堆栈 bt 查看  f [堆栈号] 切换 p打印变量
    2.调度器锁模式
    i. 查看、设置调度期锁
    show scheduler-locking
    set sheduler-locking [off/on/step] 
    不锁/ 只有当前线程可以运行 其他线程锁定/ 单步执行某一线程 保证当前线程不被切换 其他线程不执行
    该模式下执行continue until finish命令其它线程也执行 遇到断点 切换断点线程为当前线程
    ii.测试调度器锁为step模式
*/

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void* thread_entry_funcA(void* agrs){
    int i = 0;
    for(;i < 100;i++){
        std::cout << "[thread_entry_funcA] : " << i << std::endl;
        sleep(1); 
    }
    return nullptr;
}
void* thread_entry_funcB(void* agrs){
    int i = 0;
    for(;i < 100;i++){
        std::cout << "[thread_entry_funcB] : " << i << std::endl;
        sleep(1); 
    }
    return nullptr;
}

void func1()
{                      
    pthread_t a,b;
    int ret = pthread_create(&a,nullptr,thread_entry_funcA,nullptr);
    if(ret < 0)     {
        perror("thread_funcA create error");
    }
    ret = pthread_create(&b,nullptr,thread_entry_funcB,nullptr);
    if(ret < 0)     {
        perror("thread_funcA create error");
    }
    pthread_join(a,nullptr);
    pthread_join(b,nullptr);
}



