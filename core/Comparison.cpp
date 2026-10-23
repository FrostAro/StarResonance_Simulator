// ============================================================================
// 文件: Comparison.cpp
// 描述: 同种子配对对比（CRN）实现，见 Comparison.h
// ============================================================================

#include "Comparison.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <atomic>
#include <cassert>
#include <thread>

/**
 * @brief 从统计表计算总 DPS
 *
 * @details 口径与 printDamageStatistics 的输出一致：总伤害含幸运期望，再除以秒数。
 *          注意每技能"占比"的分子不含幸运，那是既有口径，不要为了统一而改。
 *
 * @param statsMap 按技能名统计的伤害表
 * @param maxTime  模拟总时长（毫秒），<=0 时返回 0 避免除零
 * @return 总 DPS（伤害/秒）
 */
double getTotalDPS(const std::unordered_map<std::string, DamageStatistics> &statsMap, int maxTime)
{
    if (maxTime <= 0)
        return 0; // 避免除零（GUI 输入被清空 / 配置错误等情况）
    double total = 0;
    for (const auto &[name, stat] : statsMap)
    {
        total += stat.damage + stat.luckyDamage;
    }
    return total / (static_cast<double>(maxTime) / 1000.0);
}

/**
 * @brief 算术平均（空容器返回 0）
 */
static double mean(const std::vector<double> &values)
{
    if (values.empty())
        return 0;
    double sum = 0;
    for (double v : values)
        sum += v;
    return sum / static_cast<double>(values.size());
}

/**
 * @brief 样本标准差（分母 n-1）
 *
 * @details 用 n-1 而非 n：这 N 个种子是从全部可能种子里抽的样本，除以 n-1 才是无偏估计。
 *          n<2 时无法计算，约定返回 0（调用方据此把 t 值也置 0）。
 *
 * @param values 样本值
 * @param avg    已算好的平均值（避免重复遍历）
 * @return 样本标准差
 */
static double sampleStddev(const std::vector<double> &values, double avg)
{
    if (values.size() < 2)
        return 0;
    double sum = 0;
    for (double v : values)
    {
        double d = v - avg;
        sum += d * d;
    }
    return std::sqrt(sum / static_cast<double>(values.size() - 1));
}

/**
 * @brief 同种子配对对比（CRN）主流程
 *
 * @details 跑 (1 + 候选数) × 配对数 次独立模拟。基准先跑 P 个种子并缓存，各候选
 *          再用同一组种子逐对相减：Δi = 候选(seed_i) - 基准(seed_i)。同种子下两份
 *          模拟的随机波动同向同幅，相减时抵消，只剩配置差异，所以 deltaStd 应远小于
 *          baseStd / candStd。前提是二者按相同顺序消费随机流；若候选改了幻想或自动
 *          战斗这类结构，配对收益会打折。
 *
 *          取消：基准未跑完返回空；候选跑到一半只返回已完成全部种子的那些。
 *
 *          并行化改造点：结果按 task = variant * seedCount + seedIndex 落槽，再按
 *          任务号升序聚合，浮点加法顺序与串行一致，才能保证结果逐位相同。
 *
 * @param base       基准配置
 * @param candidates 候选配置列表
 * @param firstSeed  起始种子，使用 firstSeed..firstSeed+seedCount-1
 * @param seedCount  配对数即种子个数（建议 20 以上）
 * @param maxTime    单次模拟时长（毫秒）
 * @param deltaTime  模拟步长（毫秒）
 * @param simulate   单次模拟函数
 * @param shouldStop 可选提前终止回调
 * @return 每个候选一条结果，按 candidates 顺序排列
 */
std::vector<ComparisonResult> runPairedComparison(
    const SimConfig &base,
    const std::vector<SimConfig> &candidates,
    std::uint32_t firstSeed,
    int seedCount,
    int maxTime,
    int deltaTime,
    const SimulateFn &simulate,
    const std::function<bool()> &shouldStop)
{
    std::vector<ComparisonResult> results;

    // 边界：配对数 <= 0 没有统计意义；simulate 为空说明调用方没给职业实现
    if (seedCount <= 0 || !simulate)
        return results;

    // 阶段 1：基准
    //
    // 只跑 seedCount 次并缓存（确定性 ⇒ 可复用）。所有候选共用这一份基准，
    // 所以候选数再多，基准也只跑一次。
    std::vector<double> baseDpsBySeed(seedCount);
    for (int i = 0; i < seedCount; ++i)
    {
        if (shouldStop && shouldStop())
            return results; // 基准未完成：没有可比对的基准，直接返回空
        const std::uint32_t seed = firstSeed + static_cast<std::uint32_t>(i);
        auto map = simulate(base, seed, maxTime, deltaTime);
        baseDpsBySeed[i] = getTotalDPS(map, maxTime);
    }
    const double baseAvg = mean(baseDpsBySeed);
    const double baseStd = sampleStddev(baseDpsBySeed, baseAvg);

    // 阶段 2：逐个候选，与基准在相同种子下逐对相减
    //
    // 循环嵌套是「外层候选、内层种子」：这样每个候选的 P 个结果、P 个 Δ 都落在
    // 本轮作用域内，算完就能产出一行 ComparisonResult，不需要额外的中间容器。
    //
    // 注意：遍历顺序不影响结果 —— 每个 (候选, 种子) 都是独立模拟，
    // 换成「外层种子、内层候选」跑出来的数完全一样，只是代码组织不同。
    results.reserve(candidates.size());
    for (const auto &cand : candidates)
    {
        std::vector<double> candDpsBySeed(seedCount);
        std::vector<double> deltas(seedCount);

        for (int i = 0; i < seedCount; ++i)
        {
            if (shouldStop && shouldStop())
                return results; // 候选跑到一半取消：只返回已完整算完的候选

            // 与基准阶段用同一个 seed 计算公式，保证两边种子一一对应
            const std::uint32_t seed = firstSeed + static_cast<std::uint32_t>(i);
            auto map = simulate(cand, seed, maxTime, deltaTime);
            candDpsBySeed[i] = getTotalDPS(map, maxTime);

            // CRN 的核心一步：同种子相减，抵消掉两边共同的随机波动
            deltas[i] = candDpsBySeed[i] - baseDpsBySeed[i];
        }

        // ---- 统计量 ----
        // candAvg/candStd：候选自身的均值与波动（未配对，仅作参考）
        // deltaStd：配对差值的标准差 —— CRN 是否生效就看它比上面两个小多少
        const double candAvg = mean(candDpsBySeed);
        const double candStd = sampleStddev(candDpsBySeed, candAvg);
        const double deltaAvg = mean(deltas);
        const double deltaStd = sampleStddev(deltas, deltaAvg);

        ComparisonResult r;
        r.label = cand.label;
        r.baseDps = baseAvg;
        r.candDps = candAvg;
        r.deltaDps = candAvg - baseAvg;
        r.deltaPercent = (baseAvg != 0) ? (r.deltaDps / baseAvg) * 100.0 : 0.0;
        r.baseStd = baseStd;
        r.candStd = candStd;
        r.deltaStd = deltaStd;
        r.pairs = seedCount;

        // ---- 配对 t 值 ----
        //   t = mean(Δ) / SE，SE = deltaStd / sqrt(N)   （SE = 配对差值均值的标准误）
        //
        // 物理含义：「这点提升，相对于配对噪声看起来像不像真的」。
        //   t 的绝对值 >= 2 ⇒ 在 N=20 量级下约 95% 置信，可认为差异真实存在
        //   t 的绝对值很小  ⇒ 提升被噪声淹没，需要加大配对数或改动本身太小
        //
        // deltaStd == 0（所有种子下 Δ 完全一致，例如只改了确定性系数）时无法定义 t，
        // 此时约定 tValue = 0（表示"无需用它判断显著性"，不是"没有提升"）。
        const double deltaSE = (seedCount > 0) ? deltaStd / std::sqrt(static_cast<double>(seedCount)) : 0.0;
        r.tValue = (deltaSE > 0) ? r.deltaDps / deltaSE : 0.0;

        results.push_back(std::move(r));
    }

    return results;
}

bool threadInitializeBase(
    const SimConfig &base,
    std::uint32_t firstSeed,
    int seedCount,
    int maxTime,
    int deltaTime,
    int threadCount,
    const SimulateFn &simulate,
    std::vector<double> &baseDpsBySeed,
    const std::function<bool()> &shouldStop)
{
    // 前 remainingSeeds 个线程各多跑一个种子，其余线程只跑 seedsPerThread 个
    if (seedCount <= 0 || !simulate)
        return true; // 没有活要干，视为已完成

    if (threadCount <= 0)
        threadCount = 1;
    if (threadCount > seedCount)
        threadCount = seedCount;

    baseDpsBySeed.resize(static_cast<std::size_t>(seedCount));

    std::vector<std::thread> threads;
    threads.reserve(static_cast<std::size_t>(threadCount));

    const int seedsPerThread = seedCount / threadCount;
    const int remainingSeeds = seedCount % threadCount;

    // 取消标志：任一工作线程发现 shouldStop 为真就置位，其余线程下次检查时退出
    std::atomic<bool> cancelled{false};

    for (int i = 0; i < threadCount; ++i)
    {
        threads.emplace_back([&, i]()
        {
            // 自己负责的固定份
            for (int j = 0; j < seedsPerThread; ++j)
            {
                // 每个种子开始前检查一次；一次模拟约 0.35s，取消延迟以此为上限
                if (shouldStop && shouldStop())
                {
                    cancelled.store(true, std::memory_order_relaxed);
                    return;
                }

                const int seedIndex = i * seedsPerThread + j;
                const std::uint32_t seed = firstSeed + static_cast<std::uint32_t>(seedIndex);
                auto map = simulate(base, seed, maxTime, deltaTime);
                // 各线程写不同下标，互不相交，无需加锁
                baseDpsBySeed[static_cast<std::size_t>(seedIndex)] = getTotalDPS(map, maxTime);
            }

            // 余数：只有前 remainingSeeds 个线程各多跑一个
            if (i < remainingSeeds)
            {
                if (shouldStop && shouldStop())
                {
                    cancelled.store(true, std::memory_order_relaxed);
                    return;
                }

                const int seedIndex = seedsPerThread * threadCount + i;
                const std::uint32_t seed = firstSeed + static_cast<std::uint32_t>(seedIndex);
                auto map = simulate(base, seed, maxTime, deltaTime);
                baseDpsBySeed[static_cast<std::size_t>(seedIndex)] = getTotalDPS(map, maxTime);
            }
        });
    }

    for (auto &th : threads)
        th.join();

    return !cancelled.load();
}

bool threadComparison(
    const std::vector<double> &baseDpsBySeed,
    const std::vector<SimConfig> &cands,
    std::uint32_t firstSeed,
    int seedCount,
    int maxTime,
    int deltaTime,
    int threadCount,
    const SimulateFn &simulate,
    std::vector<ComparisonResult> &results,
    const std::function<bool()> &shouldStop)
{
    results.clear();
    if (seedCount <= 0 || cands.empty() || !simulate)
        return true; // 没有活要干，视为已完成

    assert(baseDpsBySeed.size() >= static_cast<std::size_t>(seedCount) &&
           "baseDpsBySeed 容量不足：应先用 threadInitializeBase 填好");

    if (threadCount < 1)
        threadCount = 1;
    if (threadCount > seedCount)
        threadCount = seedCount;

    const double baseAvg = mean(baseDpsBySeed);
    const double baseStd = sampleStddev(baseDpsBySeed, baseAvg);

    // [候选][种子] 矩阵：每个 (c,s) 只有一个线程写，天然无竞争
    std::vector<std::vector<double>> candDps(
        cands.size(), std::vector<double>(static_cast<std::size_t>(seedCount), 0.0));

    const int taskCount = static_cast<int>(cands.size()) * seedCount;
    std::vector<std::uint8_t> taskDone(static_cast<std::size_t>(taskCount), 0);

    std::atomic<int> nextTask{0};
    std::atomic<bool> cancelled{false};

    auto body = [&]()
    {
        for (;;)
        {
            const int task = nextTask.fetch_add(1, std::memory_order_relaxed);
            if (task >= taskCount)
                return;
            if (cancelled.load(std::memory_order_relaxed))
                return;

            // 每取一个任务检查一次取消；一次模拟约 0.35s，取消延迟以此为上限
            if (shouldStop && shouldStop())
            {
                cancelled.store(true, std::memory_order_relaxed);
                return;
            }

            const std::size_t c = static_cast<std::size_t>(task / seedCount);
            const int s = task % seedCount;
            const std::uint32_t seed = firstSeed + static_cast<std::uint32_t>(s);
            auto map = simulate(cands[c], seed, maxTime, deltaTime);
            candDps[c][static_cast<std::size_t>(s)] = getTotalDPS(map, maxTime);
            taskDone[static_cast<std::size_t>(task)] = 1; // 任务号唯一，无竞争
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(static_cast<std::size_t>(threadCount));
    for (int t = 0; t < threadCount; ++t)
        threads.emplace_back(body);
    for (auto &th : threads)
        th.join();

    // 单线程聚合：seed 顺序固定，浮点求和顺序与串行一致
    // 取消时只输出「所有种子都已完成」的候选，避免用残缺样本算出错误统计
    results.reserve(cands.size());
    for (std::size_t c = 0; c < cands.size(); ++c)
    {
        bool complete = true;
        for (int s = 0; s < seedCount; ++s)
        {
            if (!taskDone[c * static_cast<std::size_t>(seedCount) + static_cast<std::size_t>(s)])
            {
                complete = false;
                break;
            }
        }
        if (!complete)
            break; // 后面的候选更不可能完整，直接结束（保持与串行一致的「已完成前缀」语义）

        std::vector<double> deltas(static_cast<std::size_t>(seedCount));
        for (int s = 0; s < seedCount; ++s)
        {
            // CRN 核心：同一种子下候选减基准
            deltas[static_cast<std::size_t>(s)] =
                candDps[c][static_cast<std::size_t>(s)] -
                baseDpsBySeed[static_cast<std::size_t>(s)];
        }

        const double candAvg = mean(candDps[c]);
        const double candStd = sampleStddev(candDps[c], candAvg);
        const double deltaAvg = mean(deltas);
        const double deltaStd = sampleStddev(deltas, deltaAvg);

        ComparisonResult r;
        r.label = cands[c].label;
        r.baseDps = baseAvg;
        r.candDps = candAvg;
        r.deltaDps = candAvg - baseAvg;
        r.deltaPercent = (baseAvg != 0) ? (r.deltaDps / baseAvg) * 100.0 : 0.0;
        r.baseStd = baseStd;
        r.candStd = candStd;
        r.deltaStd = deltaStd;
        r.pairs = seedCount;

        const double deltaSE = deltaStd / std::sqrt(static_cast<double>(seedCount));
        r.tValue = (deltaSE > 0) ? r.deltaDps / deltaSE : 0.0;

        results.push_back(std::move(r));
    }

    return !cancelled.load();
}

bool threadRunPairedComparison(
    const SimConfig& base,
    const std::vector<SimConfig>& candidates,
    std::uint32_t firstSeed,
    int seedCount,
    int maxTime,
    int deltaTime,
    int threadCount,
    const SimulateFn& simulate,
    std::vector<ComparisonResult>& results,
    const std::function<bool()>& shouldStop)
{
    results.clear();
    std::vector<double> baseDpsBySeed;
    if (!threadInitializeBase(base, firstSeed, seedCount, maxTime, deltaTime,
                              threadCount, simulate, baseDpsBySeed, shouldStop))
        return false;
    return threadComparison(baseDpsBySeed, candidates, firstSeed, seedCount,
                            maxTime, deltaTime, threadCount, simulate, results, shouldStop);
}


void printComparisonResults(const std::vector<ComparisonResult> &results, int pairs)
{
    std::cout << "====================================================\n";
    std::cout << " 配对对比模式（CRN 同种子）\n";
    std::cout << " 每对基准与候选使用相同随机种子，逐对相减抵消程序随机\n";
    std::cout << "====================================================\n";

    if (results.empty())
    {
        std::cout << "(无候选配置)\n";
        return;
    }

    // 基准信息
    const auto &first = results.front();
    std::cout << " 基准 DPS = " << std::fixed << std::setprecision(0) << first.baseDps
              << "（基准σ = " << first.baseStd << "，配对 N = " << pairs << "）\n\n";

    // 表头
    std::cout << std::fixed << std::setprecision(0)
              << std::left << std::setw(16) << "候选配置"
              << std::right << std::setw(12) << "基准DPS"
              << std::setw(12) << "候选DPS"
              << std::setw(11) << "ΔDPS"
              << std::setw(9) << "Δ%"
              << std::setw(10) << "配对σ"
              << std::setw(8) << "t值"
              << std::endl;

    // 数据行
    for (const auto &r : results)
    {
        std::cout << std::left << std::setw(16) << r.label
                  << std::right << std::setw(12) << r.baseDps
                  << std::setw(12) << r.candDps
                  << std::setw(11) << (r.deltaDps >= 0 ? "+" : "") << r.deltaDps
                  << std::setw(9) << (r.deltaPercent >= 0 ? "+" : "") << std::setprecision(2) << r.deltaPercent << std::setprecision(0)
                  << std::setw(10) << r.deltaStd
                  << std::setw(8) << std::setprecision(1) << r.tValue << std::setprecision(0)
                  << std::endl;
    }

    // 解读说明
    std::cout << "\n 解读：\n"
              << "  · 配对σ 远小于 基准σ/候选σ ⇒ CRN 生效，差值里只剩配置差异\n"
              << "  · |t值| ≥ 2 视为差异显著（N=" << pairs << " 时）\n"
              << "  · 候选σ/基准σ 需对比时，把第一列换成同名模拟多次求标准差即可\n";
}
