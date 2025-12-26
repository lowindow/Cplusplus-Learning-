#pragma once 


 
/*
Lambda表达式是C++11中引入的一种匿名函数特性，允许我们在代码中定义匿名的函数对象
Lambda表达式的基本语法结构如下：
[capture](parameters) -> return_type {
  // function body
}
    capture: 捕获列表，定义了在Lambda表达式外部定义的变量在Lambda内部的可见性和使用方式。
    // 捕获方式
    值捕获: 通过值捕获外部变量，捕获时会对变量进行拷贝。
    int x = 10;
    auto lambda = [x] { return x; };
    引用捕获: 通过引用捕获外部变量。
    int x = 10;
    auto lambda = [&x] { return x; };
    隐式值捕获: 捕获所有外部变量（以值的方式）。
    int x = 10;
    auto lambda = [=] { return x; };
    隐式引用
    捕获: 捕获所有外部变量（以引用的方式）。
    int x = 10;
    auto lambda = [&] { return x; };
    混合捕获: 同时使用值捕获和引用捕获
    int x = 10, y = 20;
    auto lambda = [x, &y] { return x + y; };
    无捕获: 不捕获任何外部变量。auto lambda = [] { return 42; };

    parameters: 参数列表，和普通函数的参数列表一样。
    return_type: 返回类型，可以省略，编译器会自动推导。
    function body: 函数体，包含Lambda表达式的代码逻辑。
*/
#include <iostream>



/*
    1.如何在类内使用lambda表达式捕获类成员 为什么需要this 类成员不是类内可见吗
        lambda的捕获机制只能作用于局部作用域对象 非静态类成员不属于该作用域 必须通过this指针才能够访问到

*/

class Foo{
    int x = 10;
    void Func(){
        // auto lam = [](){ return x; }; 
        // ❌ 错误：x 是 Foo 的非静态成员；lambda 的捕获列表只能捕获局部实体，
        //    成员访问必须经由某个对象（在成员函数里是隐式 this），因此这里无法解析 x。

        // auto lam = [this](){ return x; };
        //    lambda 捕获 this（一个指针），x 在 lambda 体内解析为 this->x。
        //    风险：如果 lam 在对象析构后仍被调用，会通过悬空 this 访问成员，产生 UB。

        // auto lam = [&](){ return x; };
        //    在成员函数里通常也能工作，但其关键仍是“捕获到 this 才能访问 x”。
        //    注意：这里并非“按引用捕获成员 x”，而是（隐式）捕获了 this，
        //    然后通过 this->x 访问成员。其生命周期风险与 [this] 同类。

        // auto lam = [=](){ return x; };
        //   在成员函数里通常也能工作；本质仍是捕获 this 指针（拷贝指针值），
        //    并非拷贝成员 x 的值。生命周期风险与 [this] 同类：对象销毁后调用即 UB。

        auto lam = [*this](){return x;}; // 捕获类对象的副本 在lambda运行期间确保了，脱离原对象的生命周期仍安全 
    }
};