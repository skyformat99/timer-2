#ifndef SIMPLETIMER_H_
#define SIMPLETIMER_H_

/**
 * copy from http://www.cnblogs.com/gtarcoder/p/4924097.html
 */

#include<functional>
#include<chrono>
#include<thread>
#include<atomic>
#include<memory>
#include<mutex>
#include<condition_variable>

class SimpleTimer
{
public:
    SimpleTimer() :expired_(true), try_to_expire_(false){}

    SimpleTimer(const SimpleTimer& t)
    {
        expired_ = t.expired_.load();
        try_to_expire_ = t.try_to_expire_.load();
    }

    ~SimpleTimer()
    {
        Expire();
    }

    void StartTimer(int interval, std::function<void()> task)
    {
        if (expired_ == false)
        {
            return;
        }
        expired_ = false;

        std::thread([this, interval, task] () {
            while (!try_to_expire_){
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                task();
            }

            {
                std::lock_guard<std::mutex> locker(mutex_);
                expired_ = true;
                expired_cond_.notify_one();
            }
        }).detach();
    }

    void Expire() {
        if (expired_)
        {
            return;
        }

        if (try_to_expire_)
        {
            return;
        }

        try_to_expire_ = true;

        {
            std::unique_lock<std::mutex> locker(mutex_);
            expired_cond_.wait(locker, [this]{ return expired_ == true; });
            if (expired_ == true)
            {
                try_to_expire_ = false;
            }
        }
    }

    template<typename callable, class... arguments>
    void SyncWait(int after, callable&& f, arguments&&... args)
    {
        std::function<typename std::result_of<callable(arguments...)>::type()> task
                (std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));
        std::this_thread::sleep_for(std::chrono::milliseconds(after));
        task();
    }

    template<typename callable, class... arguments>
    void AsyncWait(int after, callable&& f, arguments&&... args)
    {
        std::function<typename std::result_of<callable(arguments...)>::type()> task
                (std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));

        std::thread([after, task](){
            std::this_thread::sleep_for(std::chrono::milliseconds(after));
            task();
        }).detach();
    }

private:
    std::atomic<bool> expired_;
    std::atomic<bool> try_to_expire_;
    std::mutex mutex_;
    std::condition_variable expired_cond_;
};
#endif // SIMPLETIMER_H_
