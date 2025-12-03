#include <iostream>

#include <vector>
#include <algorithm>
#include <memory>

using namespace std;




struct Child;  // 前向声明

struct Parent {
	std::shared_ptr<Child> child;   // 直接持有孩子
	~Parent() { std::cout << "~Parent\n"; }
};

struct Child {
	std::shared_ptr<Parent> parent; // 直接持有父母 → 形成环
	~Child() { std::cout << "~Child\n"; }
};

struct Elem {
	int val;
	~Elem() { std::cout << "~Elem " << val << '\n'; }
};


std::unique_ptr<int> createInt() {
	return std::unique_ptr<int>(new int(20));
}

int main() {
	int* ptr = new int(10);

	// 忘记释放 内存泄漏
	//delete(ptr); delete(ptr); // 重复释放 程序崩溃
	//delete(ptr);// ptr = nullptr;//
	//cout << *ptr << endl; // 使用已经释放过内存的指针 指针没有置null 野指针

	// RAII（Resource Acquisition Is Initialization，资源获取即初始化）机制，
	// 将内存的管理和对象的生命周期绑定在一起。当智能指针对象创建时，它获取资源（即分配内存）；当智能指针对象销毁时，它自动释放所指向的内存，无需手动调用delete。

	// auto_ptr 所有权指针 
	std::auto_ptr<int> p1(new int(10));
	std::auto_ptr<int> p2 = p1;
	std::cout << *p2 << std::endl;
	//std::cout << *p1 << std::endl; //                             1.赋值和拷贝的时候会发生所有权转移 *p1变成空指针
	//std::auto_ptr<Elem> p(new Elem[5]{ {1},{2},{3},{4},{5} }); // 2.auto_ptr不支持数组的管理，它的析构函数使用delete而不是delete[]
	Elem* p = new Elem[5]{ {1},{2},{3},{4},{5}};                 // 3.auto_ptr不符合 STL 容器对元素拷贝语义的要求，元素赋值/拷贝后无法保持等价，不能STL容器中使用 
	//delete p; 
	delete[] p; 
	 
	// unique_ptr 独占式拥有指针 同一时间内只有一个智能指针可以指向该对象 不允许拷贝和赋值 支持数组 delete[]
	std::unique_ptr<Elem[]> p4(new Elem[5]{{6},{7},{8},{9},{10}});
	// std::unique_ptr<Elem> p5 = p4; // 编译错误 不允许拷贝
	std::unique_ptr<Elem[]> p5 = std::move(p4); // 通过std::move 返回右值引用，匹配到移动构造/移动赋值 所有权转移
	std::unique_ptr<int> uptr = createInt(); // std::unique_ptr还可以作为函数的返回对象（临时对象，右值，匹配到移动构造/移动赋值 所有权转移）

	// shared_ptr 共享式拥有指针 允许多个智能指针指向相同对象，通过引用计数机制来管理对象的生命周期 。


	std::shared_ptr<int> s1(new int(10));
	std::shared_ptr<int> s2 = s1;
	std::cout << "s1 use_count: " << s1.use_count() << std::endl;
	std::cout << "s2 use_count: " << s2.use_count() << std::endl;
	s1.reset();
	// 无参 自身引用对象计数-1 为0 释放资源 带参的 s1.reset(new int(7)); 放弃当前托管对象，对新对象进行引用计数
	std::cout << "s2 use_count: " << s2.use_count() << std::endl;

	// 
	// 线程安全问题
	// 引用计数本身是线程安全的 引用计数这个变量存在堆上，然后所有的指向相同对象的shared_ptr指向堆上同一个引用计数变量，对引用计数的修改是原子性的
	// 多线程同时对一个shared_ptr实例进行修改 并发地调用非const操作（reset、swap、赋值等）属于数据竞争，必须外部加锁。 
	// 注意 C++20 用 std::atomic<std::shared_ptr<T>> 让“多个线程安全地修改同一个 shared_ptr 指向”成为官方标准，无需手工加锁。
	// 所指向的对象 如果多个线程通过* sp 或 sp->同时读写同一份数据，仍需你自己加锁或其他同步手段。



	// weak_ptr 不控制对象生命周期的智能指针
	// 指向一个由shared_ptr管理的对象，主要用于协助shared_ptr解决循环引用问题 
	// 使用weak_ptr时，需要通过lock函数将其提升为shared_ptr，才能访问对象 。如果对象已经被释放，lock函数会返回一个空的shared_ptr 。

	auto paaaaa = std::make_shared<Parent>();
	auto cccccc = std::make_shared<Child>();
	paaaaa->child = cccccc;   // 交叉引用
	cccccc->parent = paaaaa;  // 环闭合
	//// 离开作用域，引用计数仍为 1，无法减到 0 → 泄漏
    //  运行后看不到任何析构函数打印 
	std::shared_ptr<int> s10(new int(10));
	std::weak_ptr<int> w10 = s10;
	// s10.reset();
	if (std::shared_ptr<int> s11 = w10.lock()) {
		std::cout << *s11 << std::endl;
	}
	return 0;


	
}

// 在使用std::shared_ptr时，有两种常见的创建方式：std::make_shared和std::shared_ptr(new T(args...)) ，这两种方式在性能、安全性和代码简洁性等方面存在一些差异 。
// 1.std::make_shared在性能上具有明显的优势 。它通过一次内存分配，就可以同时创建对象和控制块 。而std::shared_ptr(new T(args...))则需要进行两次内存分配，
// 一次是为对象分配内存，另一次是为控制块分配内存 。多次内存分配不仅会增加时间开销，还可能导致内存碎片化，降低内存的使用效率 。
// 2.std::make_shared一次性完成了对象的构造和智能指针的创建，即使在创建过程中发生异常，也不会导致内存泄漏，因为对象和控制块是在同一个分配操作中创建的，
// 如果分配失败，不会有部分分配成功的情况，从而保证了异常安全 




