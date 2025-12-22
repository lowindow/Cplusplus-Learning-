#pragma once
#include <iostream>
#include <memory>
/*
    1.为什么引入智能指针
    C++内存管理中 使用new分配内存，必须在合适的时机释放，忘记释放会造成内存泄漏，释放后没有置为nullptr变成野指针,多个指针指向同一内存，多次释放程序崩溃
    智能指针利用RALL机制，资源获取即初始化，将对内存的管理和智能指针对象的生命周期绑定在一起，智能指针对象创建的时候分配内存，智能指针对象销毁的时候，自动释放内存，无需手动delete
*/

/*
    auto_ptr
    c++98引入 已经被弃用 所有权指针 指针对象过期的时候 自动释放指向的内存
    缺陷在
        1. 赋值和拷贝的时候会发生所有权转移 原来的指针变成了空指针,此时访问原来指针是未定义行为 不符合STL容器对元素拷贝语义的要求
        2. auto_ptr不支持数组 它的析构函数调用的是delete 而不是delete[] 不能使用auto_ptr管理数组
*/
void auto_ptrTest1()
{
    std::auto_ptr<int> p1(new int(10));
    std::auto_ptr<int> p2 = p1;
    std::cout << *p2 << std::endl;
    std::cout << *p1 << std::endl; // 此时访问p1发生未定义行为
    std::auto_ptr<int> p3(new int[10]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    for (int i = 0; i < 10; ++i)
    {
        std::cout << "p3[" << i << "] = " << *(p3.get() + i) << std::endl;
    }
}
/*
    unique_ptr 独占所有权指针 同一时间内只允许有一个智能指针指向对象
    禁止拷贝和赋值 支持移动语义
    支持管理数组 可以使用delete[]释放内存
    可以作为函数的返回值高效的把资源返回给被调用者
*/
std::unique_ptr<int[]> getunique_ptr()
{
    return std::unique_ptr<int[]>(new int[10]{1, 2, 5, 8, 5, 9, 6, 3, 2, 1});
}
void unique_ptrTest1()
{
    std::unique_ptr<int> p1(new int(10));
    // std::unique_ptr<int> p2 = p1; // 禁止拷贝和赋值
    std::unique_ptr<int> p3 = std::move(p1);
    p1 = nullptr;
    std::cout << *p3 << std::endl;
    // std::cout << *p1 << std::endl; // 此时访问p1发生未定义行为
    std::unique_ptr<int[]> p5 = getunique_ptr(); // 支持数组 支持delete[] 且 支持 移动构造 移动赋值
}
/*
    shared_ptr 共享拥有智能指针
    允许多个智能指针指向同一对象 通过引用计数管理对象的生命周期 多一个指向 ++引用计数 当智能指针离开作用域重新赋值的时候 --引用计数 检查引用计数为0自动释放对象
    1.  shared_ptr对其管理资源的引用计数是线程安全的
        多个线程创建了指向同一资源的shared_ptr的不同智能指针对象进行对引用计数的修改(拷贝 构造 析构 reset等)无需再加锁 线程安全
        在堆上分配一个控制块（control block）来管理所指向对象的引用计数和其他相关信息 。
        当创建一个std::shared_ptr对象并指向一个新的对象时，会同时在堆上分配一个控制块，控制块中包含指向对象的指针、引用计数（初始值为 1）以及可能的自定义删除器等信息
        有新的std::shared_ptr对象指向同一个对象时(拷贝赋值和拷贝构造)，它们会共享同一个控制块，并且引用计数加 1 移动赋值和移动构造不增加引用计数
    相同shared_ptr智能指针对象本身（内部的资源指针和控制块指针）并不是线程安全的
    2. 多个线程同时修改同一个shared_ptr智能指针对象 (reset、std::move()、赋值等) 不是线程安全的
    3. 一个线程修改shared_ptr智能指针对象，同时另一个读取同一个shared_ptr智能指针对象是不安全的

*/
void shared_ptrTest1()
{
    std::shared_ptr<int[]> p1(new int[5]{1, 2, 3, 4, 5});
    std::cout << p1.use_count() << std::endl;
    auto p2 = p1;
    std::cout << p1.use_count() << std::endl;
    std::cout << p2.use_count() << std::endl;
    p1.reset(/*new int(30)*/); // 放弃对原对象的管理 可以接管新的对象

    std::cout << p2.use_count() << std::endl;
}
/*
    weak_ptr 不控制对象生命周期的智能指针 指向shared_ptr管理的对象 协助shared_ptr解决循环引用 weak_ptr不会增加对象的引用计数，它的构造和析构不会影响对象的生命周期 。
    循环引用问题
        shared_ptr  pa持有资源指针和控制块指针 资源指针指向堆上分配的A类对象
        shared_ptr  pb持有资源指针和控制块指针 资源指针指向堆上分配的B类对象
        交叉引用之后 
        pa->pb_ = pb 会拷贝 pb的资源指针 递增引用计数 
        pb->pa_ = pa 会拷贝 pa的资源指针 递增引用计数
        pa和pb生命周期结束的时候 
        先析构pb --引用计数 此时为1不能释放资源 清空资源指针和控制块指针
        再析构pa --引用计数 此时为1不能释放资源 清空资源指针和控制块指针
        导致对象的析构永远不会调用
    weak_ptr解决 将A中的std::shared_ptr<B> pb_;改为std::weak_ptr<B> pb_
    使用weak_ptr时，需要通过lock函数将其提升为shared_ptr，才能访问对象 。如果对象已经被释放，lock函数会返回一个空的shared_ptr 。
*/
class B;
class A
{
public:
    //std::shared_ptr<B> pb_;
    std::weak_ptr<B> pb_;
    ~A()
    {
        std::cout << "A delete" << std::endl;
    }
};

class B
{
public:
    std::shared_ptr<A> pa_;
    ~B()
    {
        std::cout << "B delete" << std::endl;
    }
};

void fun() {
    std::shared_ptr<A> pa(new A());
    std::shared_ptr<B> pb(new B());
    pa->pb_ = pb;
    pb->pa_ = pa;
    std::cout << "pa use_count: " << pa.use_count() << std::endl; 
    std::cout << "pb use_count: " << pb.use_count() << std::endl; 
}

void weak_ptrTest1()
{
    std::shared_ptr<int> s1(new int(10));
    std::weak_ptr<int> w1 = s1; 
    if (std::shared_ptr<int> s2 = w1.lock()) { 
        std::cout << *s2 << std::endl; 
    } else {
        std::cout << "object has been deleted" << std::endl;
    }
    s1.reset(); 
    if (std::shared_ptr<int> s3 = w1.lock()) { 
        std::cout << *s3 << std::endl; 
    } else {
        std::cout << "object has been deleted" << std::endl;
    }
}