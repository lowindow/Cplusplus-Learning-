/*
工作线程管理定时器
实现一个TimerManger 
要求：1.可注册多个定时器  每个定时器包括 
    -超时时间 
    -回调函数 
    2.内部采用一个线程负责管理和触发定时器 3.定时器触发后执行其回调 
*/

/*
    1.选择什么数据结构？
    能够支持快速找到 即将过期的任务 有序容器按 key->执行时间(绝对时间) 从小到大 排序   考虑优先队列 、红黑树
    支持触发后删除任务 且随时删除   红黑树 
    允许相同时刻 的任务同时触发     相同key mutilset

    （如果选择优先队列 也可以实现 懒删除 
    要执行该定时器的时候检查该定时器是否在删除堆的堆顶，如果在两个堆同时删除该定时器，
    如果任务堆的执行绝对时间 > 删除堆执行的绝对时间，（说明删除堆堆顶的定时器要么已经被删除了，要么不存在），从删除堆移除堆顶元素，
    如果任务堆的执行绝对时间 < 删除堆执行的绝对时间，（不做处理，没有执行到我要删除的任务 我就不删）
    ）
    2.使用chrono库来标记绝对时间 按照uint64_t 比较大小
    3.工作线程只有在有定时器任务的时候，被唤醒执行对应逻辑，否则阻塞等待定时器加入到mutilset  需要用到互斥锁和条件变量
*/

#include <iostream>
#include <pthread.h>
#include <map>
#include <chrono>
#include <functional>
#include <unordered_map>

struct Timer{
    using Callback = std::function<void()>;
    int Timerid_;
    Callback cb_;

};


class TimerManager{
public:
    using Callback = std::function<void()>;
    TimerManager()
    :_running(true)
    ,_timeridx(0)
    {
        pthread_mutex_init(&_mtx,nullptr);
        /*
        1）CLOCK_REALTIME就是当前系统时间会受以下情况影响：用户修改系统时间 NTP 同步自动调整系统时间 时间跨 DST 时钟会跳变到过去或未来
        影响：
        pthread_cond_timedwait 可能突然提前或延后返回
        程序行为可能因为系统时间调整而不稳定
        2）CLOCK_MONOTONIC
        单调递增计时器 不代表现实时间 不会受系统时间校正影响 适用于测量时间间隔
        始终向前，不跳变
        影响：
        sleep / 超时等待稳定
        适合做定时器
        */
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
        pthread_cond_init(&_cond, &attr);
        pthread_condattr_destroy(&attr);
        pthread_create(&_worker,nullptr,start,this);
    }
    ~TimerManager()
    {
        pthread_mutex_lock(&_mtx);
        _running = false;
        pthread_cond_signal(&_cond);
        pthread_mutex_unlock(&_mtx);
        pthread_join(_worker,nullptr);
        pthread_mutex_destroy(&_mtx);
        pthread_cond_destroy(&_cond);
    }
    static uint64_t GetNow_ms()
    {
        uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        return now_ms;
    }

    int addTimer(uint64_t timeout_ms,Callback cb)
    {
        pthread_mutex_lock(&_mtx);
        // 1.timermap里加入该定时器
        uint64_t trigger_time = GetNow_ms() + timeout_ms;
        auto it = _timermap.emplace(trigger_time,Timer{_timeridx,cb});
        _timer_inmap[_timeridx++] = it;
        pthread_cond_signal(&_cond);
        pthread_mutex_unlock(&_mtx);
        return it->second.Timerid_;
    }
    bool delTimer(int idx)
    {
        pthread_mutex_lock(&_mtx);
        if(_timer_inmap.count(idx) == 0){
            std::cout << "需要删除的定时器不存在！！！" << std::endl;
            pthread_mutex_unlock(&_mtx);
            return false; 
        }
        else{
            auto& it = _timer_inmap[idx];
            _timermap.erase(it);
            _timer_inmap.erase(idx);
            pthread_mutex_unlock(&_mtx);
            return true;
        }
    }
private:
    // pthread_create要求传入的函数必须没有this指针 所以必须是静态成员函数 
    // 静态成员函数不能够访问非静态成员变量和方法 所以需要将this指针参数化传入
    static void* start(void* arg)
    {
        TimerManager* tmgr = static_cast<TimerManager*>(arg);
        tmgr->run();
        return nullptr;
    }

    void run()
    {
        pthread_mutex_lock(&_mtx); //外层加锁 方便控制
        while(_running)
        {
            while(_timermap.empty() && _running)
            {
                pthread_cond_wait(&_cond,&_mtx);
            }
            if(!_running) break;

            auto it = _timermap.begin();
            uint64_t trigger_time = it->first;
            uint64_t now_time = GetNow_ms();
            if(now_time >= trigger_time){
                Timer timer = it->second;
                _timermap.erase(it);
                _timer_inmap.erase(timer.Timerid_);
                pthread_mutex_unlock(&_mtx);
                timer.cb_();
                pthread_mutex_lock(&_mtx);
                continue;
            }
            
            uint64_t wait_ms = trigger_time - now_time;
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC,&ts);
            ts.tv_sec += wait_ms / 1000;
            ts.tv_nsec += (wait_ms % 1000) * 1000000ULL;
            if (ts.tv_nsec >= 1000000000L) {
                ts.tv_sec += 1;
                ts.tv_nsec -= 1000000000L;
            }
            pthread_cond_timedwait(&_cond,&_mtx,&ts);
        }
        pthread_mutex_unlock(&_mtx);
    }

    std::multimap<uint64_t,Timer> _timermap;
    int _timeridx;
    pthread_mutex_t _mtx;
    pthread_cond_t _cond;
    pthread_t _worker;
    std::unordered_map<int,std::multimap<uint64_t,Timer>::iterator> _timer_inmap;
    bool _running;
    
};

