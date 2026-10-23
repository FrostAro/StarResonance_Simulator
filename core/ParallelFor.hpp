#pragma once

#include <atomic>
#include <cstddef>
#include <functional>
#include <thread>
#include <vector>

/**
 * @brief 把 [0, count) 分摊到最多 workerCount 个线程执行
 *
 * @details 分配方式与 threadInitializeBase 一致：count / workerCount 作为每线程的
 *          固定份，余数由前 remaining 个线程各多做一个。每个下标只会被一个线程处理，
 *          所以 body 里写 results[i] 这类"按下标落槽"的操作天然无竞争。
 *
 * @param count       总任务数（<=0 直接返回 true）
 * @param workerCount 线程数，会被夹到 [1, count]；1 时不开线程，原地执行
 * @param body        单任务体，参数是下标
 * @param shouldStop  可选取消回调，在每个任务开始前调用；会被多个线程并发调用，
 *                    必须线程安全（用 std::atomic<bool>，不要用 QThread 中断位）
 * @return true = 全部跑完；false = 被取消
 *
 * @note 取消延迟上限是单次 body 的耗时；已经开始的那一个不会被中断。
 */
inline bool parallelFor(int count, int workerCount,
                        const std::function<void(int)>& body,
                        const std::function<bool()>& shouldStop = {})
{
    if (count <= 0 || !body)
        return true;

    if (workerCount < 1)
        workerCount = 1;
    if (workerCount > count)
        workerCount = count;

    const int perThread = count / workerCount;
    const int remaining = count % workerCount;

    std::atomic<bool> cancelled{false};

    auto worker = [&](int threadIndex)
    {
        // 固定份
        const int begin = threadIndex * perThread;
        for (int i = 0; i < perThread; ++i)
        {
            if (shouldStop && shouldStop())
            {
                cancelled.store(true, std::memory_order_relaxed);
                return;
            }
            body(begin + i);
        }

        // 余数：只有前 remaining 个线程各多做一个
        if (threadIndex < remaining)
        {
            if (shouldStop && shouldStop())
            {
                cancelled.store(true, std::memory_order_relaxed);
                return;
            }
            body(perThread * workerCount + threadIndex);
        }
    };

    if (workerCount == 1)
    {
        worker(0);   // 单线程：不开线程，与原串行行为一致
    }
    else
    {
        std::vector<std::thread> threads;
        threads.reserve(static_cast<std::size_t>(workerCount));
        for (int t = 0; t < workerCount; ++t)
            threads.emplace_back(worker, t);

        for (auto& th : threads)
        {
            if (th.joinable())
                th.join();
        }
    }

    return !cancelled.load();
}
