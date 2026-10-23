// ============================================================================
// 并行执行的一致性测试
//
// 用确定性的假模拟（DPS 只由 cfg.atk 和 seed 决定），毫秒级跑完，验证：
//   1. threadInitializeBase 的种子覆盖完整（不重不漏，含余数）
//   2. 候选与基准完全相同时 deltaPercent / deltaStd 必须为 0
//      （这一条能抓住"多分配的余数位是 0、被算进均值"那类偏差）
//   3. 并行结果与串行 runPairedComparison 逐字段精确相等
//   4. 通用并行循环 parallelFor 的覆盖、等价性与取消
// ============================================================================

#include "TestHarness.h"
#include "core/Comparison.h"
#include "core/ParallelFor.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

using StatsMap = std::unordered_map<std::string, DamageStatistics>;

/// 假模拟：DPS = atk * 1000 + seed，只依赖配置与种子，完全确定
SimulateFn makeFakeSimulate()
{
    return [](const SimConfig& cfg, std::uint32_t seed, int, int)
    {
        StatsMap m;
        DamageStatistics s;
        s.skillName = "Fake";
        s.damage = static_cast<double>(cfg.atk) * 1000.0 + static_cast<double>(seed);
        s.damageCount = 1;
        m["Fake"] = s;
        return m;
    };
}

/// 构造 base + 若干候选；atk 不同则 DPS 不同
SimConfig makeBase() { SimConfig c; c.label = "base"; c.atk = 100; return c; }

std::vector<SimConfig> makeCandidates()
{
    std::vector<SimConfig> v(3);
    for (int i = 0; i < 3; ++i)
    {
        v[static_cast<std::size_t>(i)].label = "c" + std::to_string(i);
        v[static_cast<std::size_t>(i)].atk = 100 + i * 7;
    }
    return v;
}

} // namespace

// ---------------------------------------------------------------------------
// 1. 基准初始化：种子覆盖完整
// ---------------------------------------------------------------------------
TEST(thread_base_coverage, "并行基准初始化：种子覆盖完整")
{
    const SimConfig base = makeBase();
    const SimulateFn fake = makeFakeSimulate();

    for (int threadCount : {1, 2, 3, 4, 5, 8, 17, 32})
    {
        for (int seedCount : {1, 2, 5, 7, 20, 33})
        {
            std::vector<double> baseDps;
            threadInitializeBase(base, 42, seedCount, 1000, 10, threadCount, fake, baseDps);

            CHECK_EQ(static_cast<int>(baseDps.size()), seedCount);
            for (int s = 0; s < seedCount; ++s)
            {
                // maxTime=1000 → 秒数=1，所以 DPS 就等于 damage
                const double expected = 100.0 * 1000.0 + static_cast<double>(42 + s);
                CHECK_EQ(baseDps[static_cast<std::size_t>(s)], expected);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 2. 候选 == 基准 → 提升必须为 0（抓统计偏差）
// ---------------------------------------------------------------------------
TEST(thread_compare_identical, "候选与基准相同时提升为 0")
{
    const SimConfig base = makeBase();
    const SimulateFn fake = makeFakeSimulate();

    // 候选的 atk 与 base 完全一致 → 每个种子下 DPS 都相同
    std::vector<SimConfig> same(3);
    for (int i = 0; i < 3; ++i)
    {
        same[static_cast<std::size_t>(i)].label = "same";
        same[static_cast<std::size_t>(i)].atk = base.atk;
    }

    for (int threadCount : {1, 2, 3, 4, 5, 8, 20})
    {
        for (int seedCount : {4, 20, 33})
        {
            std::vector<double> baseDps;
            threadInitializeBase(base, 42, seedCount, 1000, 10, threadCount, fake, baseDps);

            std::vector<ComparisonResult> results;
            threadComparison(baseDps, same, 42, seedCount, 1000, 10, threadCount, fake, results);

            // 行数必须等于候选数（不是 线程数 × 候选数）
            CHECK_EQ(results.size(), same.size());

            for (const auto& r : results)
            {
                CHECK_EQ(r.baseDps, r.candDps);        // 均值相同
                CHECK_EQ(r.deltaDps, 0.0);             // 差值必须是 0
                CHECK_EQ(r.deltaPercent, 0.0);         // 提升必须是 0%
                CHECK_EQ(r.deltaStd, 0.0);             // 配对噪声必须是 0
                CHECK_EQ(r.pairs, seedCount);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 3. 并行 vs 串行：逐字段精确相等
// ---------------------------------------------------------------------------
TEST(thread_compare_matches_serial, "并行对比与串行逐字段一致")
{
    const SimConfig base = makeBase();
    const std::vector<SimConfig> cands = makeCandidates();
    const SimulateFn fake = makeFakeSimulate();

    for (int seedCount : {1, 5, 20, 33})
    {
        // 串行参照
        const auto serial = runPairedComparison(base, cands, 42, seedCount, 1000, 10, fake);
        CHECK_EQ(serial.size(), cands.size());

        for (int threadCount : {1, 2, 3, 4, 8, 16})
        {
            std::vector<double> baseDps;
            threadInitializeBase(base, 42, seedCount, 1000, 10, threadCount, fake, baseDps);

            std::vector<ComparisonResult> par;
            threadComparison(baseDps, cands, 42, seedCount, 1000, 10, threadCount, fake, par);

            CHECK_EQ(par.size(), serial.size());
            if (par.size() != serial.size())
                continue;

            for (std::size_t i = 0; i < serial.size(); ++i)
            {
                CHECK_EQ(par[i].label,        serial[i].label);
                CHECK_EQ(par[i].baseDps,      serial[i].baseDps);
                CHECK_EQ(par[i].candDps,      serial[i].candDps);
                CHECK_EQ(par[i].deltaDps,     serial[i].deltaDps);
                CHECK_EQ(par[i].deltaPercent, serial[i].deltaPercent);
                CHECK_EQ(par[i].baseStd,      serial[i].baseStd);
                CHECK_EQ(par[i].candStd,      serial[i].candStd);
                CHECK_EQ(par[i].deltaStd,     serial[i].deltaStd);
                CHECK_EQ(par[i].tValue,       serial[i].tValue);
                CHECK_EQ(par[i].pairs,        serial[i].pairs);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 4. 边界：空候选 / seedCount<=0
// ---------------------------------------------------------------------------
TEST(thread_compare_edge, "空候选与非法配对数应安全返回")
{
    const SimConfig base = makeBase();
    const SimulateFn fake = makeFakeSimulate();

    std::vector<ComparisonResult> results;
    results.resize(5);   // 预置脏数据，验证会被 clear

    std::vector<double> baseDps;
    threadInitializeBase(base, 42, 20, 1000, 10, 4, fake, baseDps);

    // 空候选
    threadComparison(baseDps, {}, 42, 20, 1000, 10, 4, fake, results);
    CHECK_TRUE(results.empty());

    // seedCount = 0
    std::vector<ComparisonResult> r2;
    threadComparison(baseDps, makeCandidates(), 42, 0, 1000, 10, 4, fake, r2);
    CHECK_TRUE(r2.empty());

    // threadCount = 0 / 负数（会被夹到 1）
    std::vector<double> b2;
    threadInitializeBase(base, 42, 6, 1000, 10, 0, fake, b2);
    CHECK_EQ(static_cast<int>(b2.size()), 6);
    threadInitializeBase(base, 42, 6, 1000, 10, -5, fake, b2);
    CHECK_EQ(static_cast<int>(b2.size()), 6);
}

// ---------------------------------------------------------------------------
// 5. 取消：正常路径返回 true，取消后返回 false 且不输出半成品
// ---------------------------------------------------------------------------
TEST(thread_compare_cancel, "取消语义：返回值与部分结果")
{
    const SimConfig base = makeBase();
    const std::vector<SimConfig> cands = makeCandidates();
    const SimulateFn fake = makeFakeSimulate();

    // ---- 正常路径：两个函数都应返回 true ----
    {
        std::vector<double> baseDps;
        CHECK_TRUE(threadInitializeBase(base, 42, 20, 1000, 10, 4, fake, baseDps));

        std::vector<ComparisonResult> results;
        CHECK_TRUE(threadComparison(baseDps, cands, 42, 20, 1000, 10, 4, fake, results));
        CHECK_EQ(results.size(), cands.size());
    }

    // ---- 立即取消：还没开始就该退出 ----
    {
        auto stopNow = []() { return true; };

        std::vector<double> baseDps;
        CHECK_FALSE(threadInitializeBase(base, 42, 20, 1000, 10, 4, fake, baseDps, stopNow));
        CHECK_EQ(static_cast<int>(baseDps.size()), 20);   // resize 已做，但值未填

        // 用一份完整的基准 + 立即取消，候选一个都完成不了 → 结果为空
        std::vector<double> goodBase;
        threadInitializeBase(base, 42, 20, 1000, 10, 4, fake, goodBase);

        std::vector<ComparisonResult> results;
        CHECK_FALSE(threadComparison(goodBase, cands, 42, 20, 1000, 10, 4, fake, results, stopNow));
        CHECK_TRUE(results.empty());
    }

    // ---- 中途取消：只允许输出「所有种子都完成」的候选 ----
    {
        std::vector<double> goodBase;
        threadInitializeBase(base, 42, 20, 1000, 10, 4, fake, goodBase);

        std::atomic<int> probe{0};
        // 第 30 次探测后请求取消（总任务 3*20=60，确保取消发生在中途）
        auto stopLater = [&probe]() { return probe.fetch_add(1) >= 30; };

        // 单线程跑，保证任务分配顺序可预测
        std::vector<ComparisonResult> results;
        const bool ok = threadComparison(goodBase, cands, 42, 20, 1000, 10, 1, fake, results, stopLater);

        CHECK_FALSE(ok);
        CHECK_TRUE(results.size() <= cands.size());
        // 输出的每一行都必须基于完整样本，不能出现「只跑了一半」的候选
        for (const auto& r : results)
            CHECK_EQ(r.pairs, 20);
    }
}

// ---------------------------------------------------------------------------
// 6. 并行统一入口：与串行一致 + 取消传播
// ---------------------------------------------------------------------------
TEST(thread_run_paired, "并行统一入口：结果与串行一致、取消正确传播")
{
    const SimConfig base = makeBase();
    const std::vector<SimConfig> cands = makeCandidates();
    const SimulateFn fake = makeFakeSimulate();

    // 串行参照
    const auto serial = runPairedComparison(base, cands, 42, 20, 1000, 10, fake);
    CHECK_EQ(serial.size(), cands.size());

    // 各线程数下都必须与串行逐字段相等
    for (int threadCount : {1, 2, 3, 4, 8, 20})
    {
        std::vector<ComparisonResult> par;
        CHECK_TRUE(threadRunPairedComparison(base, cands, 42, 20, 1000, 10,
                                             threadCount, fake, par));
        CHECK_EQ(par.size(), serial.size());
        if (par.size() != serial.size())
            continue;

        for (std::size_t i = 0; i < serial.size(); ++i)
        {
            CHECK_EQ(par[i].label,        serial[i].label);
            CHECK_EQ(par[i].baseDps,      serial[i].baseDps);
            CHECK_EQ(par[i].candDps,      serial[i].candDps);
            CHECK_EQ(par[i].deltaDps,     serial[i].deltaDps);
            CHECK_EQ(par[i].deltaPercent, serial[i].deltaPercent);
            CHECK_EQ(par[i].deltaStd,     serial[i].deltaStd);
            CHECK_EQ(par[i].tValue,       serial[i].tValue);
            CHECK_EQ(par[i].pairs,        serial[i].pairs);
        }
    }

    // 立即取消：返回 false，且必须把上一次留下的脏数据清掉
    {
        auto stopNow = []() { return true; };
        std::vector<ComparisonResult> dirty(3);   // 预置旧结果
        CHECK_FALSE(threadRunPairedComparison(base, cands, 42, 20, 1000, 10,
                                              4, fake, dirty, stopNow));
        CHECK_TRUE(dirty.empty());
    }
}

// ---------------------------------------------------------------------------
// 7. 通用并行循环：覆盖完整、与串行等价、取消传播
// ---------------------------------------------------------------------------
TEST(parallel_for_basic, "parallelFor：每个下标恰好执行一次")
{
    for (int count : {1, 2, 5, 7, 20, 33})
    {
        for (int workers : {1, 2, 3, 4, 8, 100})
        {
            std::vector<int> hits(static_cast<std::size_t>(count), 0);

            const bool ok = parallelFor(count, workers, [&](int i) {
                hits[static_cast<std::size_t>(i)] += 1;   // 下标唯一，无竞争
            });

            CHECK_TRUE(ok);
            for (int i = 0; i < count; ++i)
                CHECK_EQ(hits[static_cast<std::size_t>(i)], 1);   // 不重不漏
        }
    }
}

TEST(parallel_for_equivalent, "parallelFor：与串行结果一致")
{
    constexpr int kCount = 20;
    std::vector<double> serial(static_cast<std::size_t>(kCount), 0.0);
    for (int i = 0; i < kCount; ++i)
        serial[static_cast<std::size_t>(i)] = static_cast<double>(i) * 1.5;

    for (int workers : {1, 2, 4, 8, 32})
    {
        std::vector<double> out(static_cast<std::size_t>(kCount), 0.0);
        parallelFor(kCount, workers, [&](int i) {
            out[static_cast<std::size_t>(i)] = static_cast<double>(i) * 1.5;
        });
        for (int i = 0; i < kCount; ++i)
            CHECK_EQ(out[static_cast<std::size_t>(i)], serial[static_cast<std::size_t>(i)]);
    }
}

TEST(parallel_for_cancel, "parallelFor：取消返回 false 且不再执行后续")
{
    // 立即取消
    int executed = 0;
    CHECK_FALSE(parallelFor(100, 4, [&](int) { ++executed; }, []() { return true; }));
    CHECK_EQ(executed, 0);

    // 中途取消：原子计数，保证线程安全
    std::atomic<int> calls{0};
    std::atomic<int> done{0};
    const bool ok = parallelFor(100, 1, [&](int) { ++done; },
                                [&calls]() { return calls.fetch_add(1) >= 10; });
    CHECK_FALSE(ok);
    CHECK_TRUE(done.load() <= 10);   // 单线程：取消后不再执行

    // 边界：count<=0 视为完成
    CHECK_TRUE(parallelFor(0, 4, [](int) {}));
    CHECK_TRUE(parallelFor(-3, 4, [](int) {}));
}
