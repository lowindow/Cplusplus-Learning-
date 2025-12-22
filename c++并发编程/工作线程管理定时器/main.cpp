#include "TimerManager.hpp"   // 你的 TimerManager 定义头文件
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

using namespace std::chrono_literals;

static void SleepMs(uint64_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int main() {
    std::cout << "===== TimerManager 综合测试开始 =====" << std::endl;

    TimerManager tmgr;

    // 记录程序启动时间
    auto start_ms = TimerManager::GetNow_ms();
    std::cout << "[Main] 程序启动时间(ms): " << start_ms << std::endl;

    // 1. 基础测试：不同超时时间的定时器
    std::cout << "\n--- 测试 1：基础单次定时器 ---" << std::endl;
    tmgr.addTimer(1000, [start_ms]() {
        auto now = TimerManager::GetNow_ms();
        std::cout << "[T1] 1s 定时器触发，now=" << now
                  << " (相对启动 " << now - start_ms << " ms)" << std::endl;
    });

    tmgr.addTimer(3000, [start_ms]() {
        auto now = TimerManager::GetNow_ms();
        std::cout << "[T2] 3s 定时器触发，now=" << now
                  << " (相对启动 " << now - start_ms << " ms)" << std::endl;
    });

    tmgr.addTimer(2000, [start_ms]() {
        auto now = TimerManager::GetNow_ms();
        std::cout << "[T3] 2s 定时器触发，now=" << now
                  << " (相对启动 " << now - start_ms << " ms)" << std::endl;
    });

    // 2. 相同触发时间的多个定时器（测试 multimap 同 key 顺序）
    std::cout << "\n--- 测试 2：相同超时时间的多个定时器 ---" << std::endl;
    tmgr.addTimer(2000, [start_ms]() {
        auto now = TimerManager::GetNow_ms();
        std::cout << "[T4] 2s 定时器 A 触发，now=" << now
                  << " (相对启动 " << now - start_ms << " ms)" << std::endl;
    });

    tmgr.addTimer(2000, [start_ms]() {
        auto now = TimerManager::GetNow_ms();
        std::cout << "[T5] 2s 定时器 B 触发，now=" << now
                  << " (相对启动 " << now - start_ms << " ms)" << std::endl;
    });

    // 3. 删除定时器测试：删除尚未触发的定时器
    std::cout << "\n--- 测试 3：删除定时器 ---" << std::endl;
    int id_to_cancel = tmgr.addTimer(4000, [start_ms]() {
        auto now = TimerManager::GetNow_ms();
        std::cout << "[T6] 4s 定时器(应该被取消，不应看到这行) now=" << now
                  << " (相对启动 " << now - start_ms << " ms)" << std::endl;
    });

    std::cout << "[Main] 注册 4s 定时器 id=" << id_to_cancel << "，2s 后删除..." << std::endl;

    // 2 秒后删除这个 4s 的定时器
    tmgr.addTimer(2000, [&tmgr, id_to_cancel, start_ms]() {
        auto now = TimerManager::GetNow_ms();
        bool ret = tmgr.delTimer(id_to_cancel);
        std::cout << "[T7] 2s 定时器触发，尝试删除 4s 定时器 id=" << id_to_cancel
                  << "，结果=" << (ret ? "成功" : "失败")
                  << "，now=" << now
                  << " (相对启动 " << now - start_ms << " ms)" << std::endl;
    });

    // 再测试删除一个不存在的 id
    tmgr.addTimer(2500, [&tmgr, start_ms]() {
        auto now = TimerManager::GetNow_ms();
        bool ret = tmgr.delTimer(99999); // 不存在的 id
        std::cout << "[T8] 2.5s 定时器触发，尝试删除不存在的定时器 id=99999，结果="
                  << (ret ? "成功(不正常)" : "失败(正常)")
                  << "，now=" << now
                  << " (相对启动 " << now - start_ms << " ms)" << std::endl;
    });

    // 4. 0ms 定时器（立即触发）
    std::cout << "\n--- 测试 4：0ms 定时器（立即触发） ---" << std::endl;
    tmgr.addTimer(0, [start_ms]() {
        auto now = TimerManager::GetNow_ms();
        std::cout << "[T9] 0ms 定时器触发(尽快)，now=" << now
                  << " (相对启动 " << now - start_ms << " ms)" << std::endl;
    });

    // 5. 回调中再注册定时器
    std::cout << "\n--- 测试 5：回调中再注册定时器 ---" << std::endl;
    tmgr.addTimer(1500, [&tmgr, start_ms]() {
        auto now = TimerManager::GetNow_ms();
        std::cout << "[T10] 1.5s 定时器触发，在回调中再注册一个 1s 定时器" << std::endl;

        tmgr.addTimer(1000, [start_ms]() {
            auto now2 = TimerManager::GetNow_ms();
            std::cout << "[T11] 由 T10 回调内部注册的 1s 定时器触发，now=" << now2
                      << " (相对启动 " << now2 - start_ms << " ms)" << std::endl;
        });
    });

    // 6. 其他线程并发添加定时器
    std::cout << "\n--- 测试 6：其他线程并发添加定时器 ---" << std::endl;
    std::thread producer_thread([&tmgr, start_ms]() {
        for (int i = 0; i < 5; ++i) {
            uint64_t delay = 700 + i * 300; // 0.7s, 1.0s, 1.3s, ...
            tmgr.addTimer(delay, [start_ms, i, delay]() {
                auto now = TimerManager::GetNow_ms();
                std::cout << "[TP" << i << "] 生产者线程添加的定时器触发，delay="
                          << delay << "ms, now=" << now
                          << " (相对启动 " << now - start_ms << " ms)" << std::endl;
            });
            std::this_thread::sleep_for(100ms);
        }
    });

    // 给所有定时器足够的时间触发
    std::cout << "\n[Main] 主线程 sleep 8 秒，等待所有定时器触发..." << std::endl;
    SleepMs(8000);

    if (producer_thread.joinable()) {
        producer_thread.join();
    }

    std::cout << "\n===== TimerManager 综合测试结束，准备退出 main =====" << std::endl;
    // 离开 main 时 tmgr 析构，工作线程会被正常 join 并退出
    return 0;
}
