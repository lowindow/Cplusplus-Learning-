#include "myvector.h"
#include <vector>


/*                                              vector扩容问题
	vs下vector是按照1.5倍增长的
	g++下vector是按照2倍增长的
	在知道需要多少空间，提前将空间设置足够，可以缓解vector频繁扩容的代价缺陷
	vector扩容代价重新分配一片更大的内存区域，把旧数据搬过去，然后释放旧的内存区域 为什么不能原地扩容？-》尾部空闲空间的不确定性
	为什么不用relloc呢？
	1.尝试重新分配 ptr 指向的内存块，使其大小变为 new_size。
	2.如果能够原地扩展，直接增大原始内存块，并返回原始 ptr。
	3.如果无法原地扩展，realloc() 会分配一块新的、大小为 new_size 的内存块。旧内存块中的所有数据以原始字节流的形式拷贝到新内存块中。然后释放旧的内存块。再返回新内存块的地址。
	realloc() 仅仅进行原始字节的复制。它对所操作的内存内容一无所知，也毫不关心这些字节是否代表着有复杂内部状态的 C++ 对象。
	也就是说，realloc() 不会调用 C++ 对象的拷贝构造函数、移动构造函数，也不会调用它们的析构函数。只管搬运字节，不管理对象的生命周期。
	对于非基本类型，引入资源重复。双重释放。悬空指针问题。
*/
void test_vs_reserve()
{
	std::vector<int> a;
	size_t as = a.capacity();

	for (int i = 0; i < 100; i++) {
		a.emplace_back(i);
		if (as != a.capacity()) {
			as = a.capacity();
			std::cout << as << std::endl;
		}
	}

	return;
}



/*                     vector 迭代器失效问题

	在vector底层内存空间发生改变的时候,reserve,resize,insert,assign,emplace\push back，原先迭代器对应指针指向的那片空间已经被释放掉了，
	此时使用没有更新的迭代器，就会导致程序崩溃 在使用前，重新给要使用的迭代器赋值即可 
	Linux下，g++编译器对迭代器失效的检测并不是非常严格，处理也没有vs下极端。程序可能正常允许 但运行结果已经不对了
*/
void test_iterator_invalidation() {
	std::vector<int> a(10,6);
	auto it = a.begin();
	size_t sz = a.capacity();
	//a.emplace_back(50);
	//a.reserve(1);
	//a.assign(100, 8);
	a.resize(50); // 可以原地缩 erase的话 可能出现删完迭代器指向end()  erase删除pos位置元素后，pos位置之后的元素会往前搬移
	if (a.capacity() != sz) { std::cout << "capacity change! " << std::endl; }
	std::cout << *it << std::endl;
}


 
using namespace lowindow;

int main()
{
	// test_vs_reserve();
	// test_iterator_invalidation();
	size_t  c = 10;
	myvector<int> a(c);
	//std::cout << a.back() << std::endl;
	a.resize(20,8);
	//std::cout << a.back() << std::endl;
	myvector<int> b(a);
	//std::cout << b.back() << std::endl;
	b.push_back(6);
	//std::cout << b.back() << std::endl;

	a.insert(0, 100000);


	for (size_t i = 0; i < a.size(); i++)
	{
		std::cout << a[i] << std::endl;
	}
}