#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

template<typename T>
class _atomic_queue
{
public:
    void push(T&v)
    {
        std::unique_lock<std::mutex> lck(mtx);
        q.push(v);
    }
    bool pop(T&v)
    {
        std::unique_lock<std::mutex> lck(mtx);
        if(!q.empty())
        {
            v = std::move(q.front());
            q.pop();
            return true;
        }
        return false;
    }
    size_t size()
    {
        std::unique_lock<std::mutex> lck(mtx);
        return q.size();
    }
private:
    std::queue<T> q;
    std::mutex mtx;
};

class thread_pool
{
public:
    thread_pool(size_t njobs): waiting_threads(0), stop(false), wait_interrupt(false)
    {
        thr.resize(njobs);
        thstop.resize(njobs);
        for(size_t i = 0; i < njobs; ++i)
        {
            auto cstop = thstop[i] = std::make_shared<std::atomic<bool>>(false);
            auto looper = [this, i, cstop]
            {
                std::atomic<bool>&stop = *cstop;
                std::function<void(int)> *f;
                bool popped = wq.pop(f);
                while(1)
                {
                    for(; popped; popped = wq.pop(f))
                    {
                        std::unique_ptr<std::function<void(int)>> pf(f);
                        (*f)(i);
                        if(stop)return;
                    }
                    std::unique_lock<std::mutex> lck(mtx);
                    ++waiting_threads;
                    cv.wait(lck, [this, &f, &popped, &stop]
                    {
                        popped = wq.pop(f);
                        return popped || wait_interrupt || stop;
                    });
                    --waiting_threads;
                    if(!popped)return;
                }
            };
            thr[i].reset(new std::thread(looper));
        }
    }
    template<typename F, typename...A>
    auto create_task(F&&f, A&&...args)->std::future<decltype(f(0, args...))>
    {
        auto task = std::make_shared<std::packaged_task<decltype(f(0, args...))(int)>>(
                        std::bind(std::forward<F>(f), std::placeholders::_1, std::forward<A>(args)...)
                    );
        auto worktask = new std::function<void(int)>([task](int id)
        {
            (*task)(id);
        });
        wq.push(worktask);
        std::unique_lock<std::mutex> lck(mtx);
        cv.notify_one();
        return task->get_future();
    }
    void wait()
    {
        if(!stop)
            wait_interrupt = true;
        {
            std::unique_lock<std::mutex> lck(mtx);
            cv.notify_all();
        }
        for(size_t i = 0; i < thr.size(); ++i)if(thr[i]->joinable())thr[i]->join();
        std::function<void(int)> *f;
        while(wq.size())
        {
            wq.pop(f);
            delete f;
        }
        thr.clear();
        thstop.clear();
    }
    void terminate()
    {
        stop = true;
        std::function<void(int)> *f;
        while(wq.size())
        {
            wq.pop(f);
            delete f;
        }
        for(size_t i = 0; i < thstop.size(); ++i)*thstop[i] = true;
        {
            std::unique_lock<std::mutex> lck(mtx);
            cv.notify_all();
        }
        for(size_t i = 0; i < thr.size(); ++i)if(thr[i]->joinable())thr[i]->join();
        while(wq.size())
        {
            wq.pop(f);
            delete f;
        }
        thr.clear();
        thstop.clear();
    }
private:
    std::vector<std::unique_ptr<std::thread>> thr;
    std::vector<std::shared_ptr<std::atomic<bool>>> thstop;
    _atomic_queue<std::function<void(int)>*> wq;
    std::atomic<bool> wait_interrupt;
    std::atomic<bool> stop;
    std::atomic<int> waiting_threads;
    std::mutex mtx;
    std::condition_variable cv;
};

#endif
