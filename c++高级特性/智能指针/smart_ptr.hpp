#pragma once
#include <iostream>
#include <memory>
#include <functional>
#include <thread>
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
    如果需要使用shared_ptr交叉引用，需要把一个方向的引用改成weak_ptr 打破循环引用
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
/*
    make_shared 和shared_ptr的区别 make_shared将对象构造和控制块分配合并为一个原子操作
     1) 内存分配上
        make_shared 一次内存分配 同时创建对象和 控制块
        shared_ptr 需要两次 内存分配 分别创建对象和控制块 ->内存碎片化
     2）安全性上 
        make_shared 将对象的构造和智能指针的构造 合并成一个操作 减少显示使用new  
        避免了 对象构造之后，shared_ptr构造之前发生异常，构造对象分配的内存无法正确释放 
    3）内存释放上
        shared_ptr引用计数为0 但只要有weak_ptr指向 对象和控制块无法被销毁
        make_shared 引用计数为0  但只要有weak_ptr指向 对象可以被销毁
    
*/

/*
    make_shared/shared_ptr  
    用同一裸指针多次构造多个 shared_ptr（导致多控制块、引用计数独立）
    创建不同的控制块 引用计数不会正常增加 重复析构风险
    很多接口（尤其是异步操作、回调注册接口）会要求传入shared_ptr（而非裸指针），
    目的是通过引用计数管理类对象生命周期，避免对象在函数执行期间被销毁。此时类成员函数内部需要将this转为shared_ptr传入该接口。
    如果直接传递this指针 ，引用计数不能正确增加 ，
    如果类对象提前销毁 对象先于异步线程析构  异步线程访问类对象就会访问到悬空指针
    而且在析构的时候 还会导致重复释放 第二次析构访问控制块的时候会访问到悬空指针 因为引用计数和资源没有正确关联起来  

*/

class TaskHandler : public std::enable_shared_from_this<TaskHandler> {
public:
    void start_task() {
        // 异步任务接口要求传入shared_ptr<TaskHandler>
        async_task([self = badGetSelf()]() {
            self->do_task(); // 确保任务执行期间，当前对象不会被销毁
            std::cout << "异步任务执行完成，对象引用计数：" << self.use_count() << std::endl;
        });
    }

    std::shared_ptr<TaskHandler> getSelfSharedPtr() {
        //通过shared_from_this()获取this对应的shared_ptr
        return shared_from_this();
    }

    std::shared_ptr<TaskHandler> badGetSelf() {
        // 错误：直接用this创建shared_ptr
        return std::shared_ptr<TaskHandler>(this); 
    }
    void do_task() {
        std::cout << "执行核心任务..." << std::endl;
    }

private:
    // 模拟要求shared_ptr参数的异步任务接口
    void async_task(std::function<void()> task) {
        std::thread t(std::move(task));
        t.detach();
    }
};



void Thistest()
{
   // auto ptr1 = std::make_shared<int[]>(1,2,3,4,5,6,7,8,9,10);
   
    int* A = new int[10]{1,2,3,4,5,6,7,8,9,10};
    std::shared_ptr<int[]> ptr2(A);
    std::cout << ptr2.use_count() << std::endl;
    int* B = A;
   // std::shared_ptr<int[]> ptr3(B); 用同一裸指针多次构造多个 shared_ptr（导致多控制块、引用计数独立）
    std::cout << ptr2.use_count() << std::endl;
    // std::cout << ptr3.use_count() << std::endl;


    auto handler = std::make_shared<TaskHandler>();
    handler->start_task();
    handler.reset();
    std::cout << "主线程引用计数：" << handler.use_count() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1)); // 等待异步任务完成


}