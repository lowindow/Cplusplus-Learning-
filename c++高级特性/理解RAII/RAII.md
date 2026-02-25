

# RAII 

------

# RAII 是什么

**核心机制：**

> 资源获取即初始化（Resource Acquisition Is Initialization）
>  资源释放即析构

将**资源的生命周期和对象的生命周期进行绑定**。

------

## 析构函数要求

- 析构函数必须保证不抛出异常
- 应标记为 `noexcept`
- C++11 起析构函数默认 `noexcept`

### 为什么不能抛异常？

如果析构函数在**栈展开（处理另一个异常）**的时候抛出异常：

- 运行时无法同时处理两个异常
- 直接调用 `std::terminate`
- 程序终止

因此：

> RAII 类必须保证析构阶段不抛异常。
>  如果释放资源可能失败，应该提供显式 `close()` 接口处理异常，而不是在析构函数中抛出。

------

# 栈展开

当异常抛出时：

1. 程序开始执行栈展开
2. 寻找匹配的 `catch`
3. 在查找过程中销毁当前作用域的所有局部对象

如果在栈展开期间再抛出新异常：

- 运行时无法同时处理两个异常
- 直接调用 `std::terminate`

------

## 补充

- 构造函数可以抛出异常（对象没有构造成功）
- 异常抛出后一定会栈展开吗？

> 不一定，如果没有找到匹配的 `catch`，会直接 `std::terminate`

------

# 解决问题 —— 异常安全

异常发生时：

- 程序执行栈展开
- 销毁当前作用域内所有已构造对象

因此：

> 只要把资源的生命周期和对象的生命周期绑定在一起
>  异常发生时，资源一定被释放

------

# 手动管理资源的痛点

- 忘记释放操作
- 内存泄漏
- 资源泄漏
- 锁未释放导致死锁

------

# STL 中的 RAII

## 1️⃣ 容器

- `std::vector`
- `std::string`
- `std::map`
- 等等

构造时分配内存，生命周期结束自动析构释放。

无需手动管理。

------

## 2️⃣ 智能指针

解决资源所有权问题。

### `std::unique_ptr`

- 独占所有权
- 被销毁时自动释放资源

### `std::shared_ptr`

- 共享所有权
- 最后一个 `shared_ptr` 被销毁，控制块引用计数归 0 时资源释放

### `std::weak_ptr`

- 解决循环引用问题

------

## 3️⃣ 线程管理

正确回收线程。

C++11 规定：

> 如果线程对象析构时仍然是 joinable 状态
>  既没有 join，也没有 detach
>  程序会直接 `std::terminate`

------

### ❌ 不使用 RAII 的线程管理

```
void f(){
    std::thread t([]{
        // do_work();
    });

    if(someting_wrong)
        return;  // 提前返回忘记 join 或者抛异常

    t.join();
}
// 线程对象析构的时候仍然 joinable，直接调用 std::terminate
```

------

### ✅ 使用 RAII 管理线程

```
// RAII 使用一个对象管理线程，在析构函数里自动join
class ThreadGuard{
public:
    explicit ThreadGuard(std::thread& t)
        : t_(t) {}

    ~ThreadGuard() {
        if(t_.joinable())
            t_.join();
    }

    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;

private:
    std::thread& t_;
};

void f(){
    std::thread t([]{
        // do_work();
    });

    ThreadGuard guard(t);

    if(someting_wrong)
        return;  // 提前 return 或抛异常
}
// 异常或者提前 return，guard 析构时自动 join
```

------

### C++20

提供 `std::jthread`

- 带 RAII 线程
- 析构自动 join

------

## 4️⃣ 互斥锁管理

RAII 在互斥锁上的作用：

> 保证锁在提前 return / 抛出异常时被正确释放
>  避免死锁

------

### ❌ 手动管理锁

```
std::mutex m;

void f() {
    m.lock();

    do_something(); // 可能提前 return 或抛异常

    m.unlock();
}
// unlock 永远不会被执行，导致死锁
```

------

### ✅ 使用 RAII 管理锁

```
#include <mutex>

std::mutex m;

void f() {
    std::lock_guard<std::mutex> lock(m);
    do_something();
}
// 异常或提前 return，lock 析构自动释放锁
```

------

# std::function 与 RAII

- `std::function` 是通用函数包装器
- lambda 的捕获列表对象被值捕获后，其生命周期完全绑定到 `std::function`，资源随 `std::function` 的析构自动释放

------

## 引用捕获注意点

- lambda 里捕获的是 `x` 的引用
- 存的是指向 `x` 的地址
- `std::function` 拥有 lambda 对象
- 并不拥有 `x`
- 不会延长引用捕获资源的生命周期
- 如果对象提前释放了，lambda 对象里就变成了悬空引用

------

# 自定义 RAII 管理资源

## 五法则（Rule of Five）

如果需要自定义：

- 析构函数
- 拷贝构造函数
- 拷贝赋值运算符
- 移动构造函数
- 移动赋值运算符

中的一个，

> 应该显式处理所有五个
>  防止编译器隐式生成不符合资源管理逻辑的函数

------

## 默认生成是“浅拷贝”

编译器默认生成：

- 成员逐个复制
- 对指针成员来说是浅拷贝

------

### 典型错误

只写了析构函数：

- 编译器仍然会隐式生成拷贝构造 / 拷贝赋值
- 两个对象共享同一块内存
- 析构时重复释放
- 导致未定义行为

------

# 零法则（Rule of Zero）

建议：

> 使用标准库组件管理资源

例如：

- `std::vector`
- `std::string`
- `std::unique_ptr`
- `std::shared_ptr`

优点：

- 编译器生成的构造、拷贝、移动都是正确的
- 不需要自己实现五法则

------

# 总结

RAII 的本质：

> 把“必须释放的资源”交给对象的析构函数
>  通过栈展开机制保证异常安全
>  通过所有权语义保证资源只被释放一次

是 C++ 资源管理和异常安全的核心机制。
