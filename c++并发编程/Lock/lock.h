#pragma once
#include <iostream>
#include <mutex>

/*
    RALL机制管理锁的生命周期 防止忘记解锁导致死锁
    1.lock_guard 和 unique_lock
    lock_gurad
        构造的时候自动获取锁 析构的时候自动释放锁 使用于函数 代码块内部
    unique_lock
        还可以支持手动控制锁的获取和释放 和定时尝试锁定 支持锁的转让 移动构造和移动赋值

*/