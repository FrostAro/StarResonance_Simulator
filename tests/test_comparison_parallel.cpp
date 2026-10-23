// ============================================================================
// 并行配对对比的一致性测试
//
// 用确定性的假模拟（DPS 只由 cfg.atk 和 seed 决定），毫秒级跑完，验证三件事：
//   1. threadInitializeBase 的种子覆盖完整（不重不漏，含余数）
//   2. 候选与基准完全相同是 deltaPercent / deltaStd 必须为 0
//      （这一条能抓住"多分配的余数位是 0、被算进均值"那类偏差）
//   3. 并行结果与串行 runPairedComparison 逐字段精确相等
// ============================================================================

#include "TestHarness.h"
#include "core/Comparison.h"

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
