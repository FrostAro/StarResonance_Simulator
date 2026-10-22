// ============================================================================
// 并行隔离测试：两个 Person 在两个线程里同时跑，结果必须和单线程逐位一致
//
// 这是「监听器表下放 Person」的毕业证。在此之前，监听器挂在
// TypedAction<Derived, Listener>::listeners（inline static，进程级）上，
// 两个 Person 同时存活会互相激活：A 的 Buff 回调被 B 的事件触发，
// 又回头操作 A，形成无限递归 —— 实测退出码 0xC00000FD（栈溢出）。
//
// 表搬到 Person::actionListeners 之后：
//   · 不同 Person 的表互不相干 → 模拟结果与线程数无关
//   · 同一 Person 仍只允许单线程访问（unordered_map 非线程安全），
//     这正是模拟器的实际用法（一个 Person 一个线程）
//
// ⚠️ 三个必须遵守的约束：
//   1. CHECK_* 宏会修改全局计数器，不是线程安全的 → 线程里只算，主线程比对
//   2. 线程函数里抛异常会 std::terminate → 这里不允许抛
//   3. Logger 是全局的，并发写会交错
//      （数值不受影响，只是屏幕会花）
// ============================================================================

#include "TestHarness.h"
#include "Mage/Beam/SimulationHelper.h"

#include <cstdint>
#include <string>
#include <thread>
#include <unordered_map>

namespace {

constexpr int kMaxTime = 180000;
constexpr int kDeltaTime = 10;
constexpr std::uint32_t kSeed = 42;

/// 与黄金基线同一套站街配置
SimConfig parallelTestSimConfig()
{
    SimConfig cfg;
    cfg.primaryAttributes   = 9272;
    cfg.critical            = 10.12;
    cfg.quickness           = 38.18;
    cfg.lucky               = 23.21;
    cfg.proficient          = 74.8;
    cfg.almighty            = 20.88;
    cfg.atk                 = 8151;
    cfg.refineATK           = 1100;
    cfg.elementATK          = 430;
    cfg.attackSpeed         = 0.0;
    cfg.castingSpeed        = 0.0;
    cfg.criticaldamage_set  = 0.0;
    cfg.increasedamage_set  = 0.0;
    cfg.elementdamage_set   = 0.0;
    cfg.fantasyConfig       = 7;
    cfg.flowConfig          = 0;
    cfg.weapon = WeaponConfig{SecondaryAttributeConfig::Proficient,
                              SecondaryAttributeConfig::Quickness,
                              0.06, 0.08};
    return cfg;
}

using StatsMap = std::unordered_map<std::string, DamageStatistics>;

/// 逐字段比较两份统计表，返回不一致字段的描述（空 = 完全一致）
std::string diffStats(const StatsMap& a, const StatsMap& b, const char* tagA, const char* tagB)
{
    if (a.size() != b.size())
    {
        return std::string(tagA) + " 与 " + tagB + " 技能数不同: " +
               std::to_string(a.size()) + " vs " + std::to_string(b.size());
    }

    for (const auto& [skillName, sa] : a)
    {
        const auto it = b.find(skillName);
        if (it == b.end())
        {
            return std::string(tagA) + " 独有的技能: " + skillName;
        }
        const auto& sb = it->second;

        // 全部用精确比较：并行隔离的要求就是"逐位一致"，不是"足够接近"
        if (sa.damage           != sb.damage           ||
            sa.damageCount      != sb.damageCount      ||
            sa.luckyDamage      != sb.luckyDamage      ||
            sa.luckyDamageCount != sb.luckyDamageCount ||
            sa.CritDamageCount  != sb.CritDamageCount  ||
            sa.releasedTimes    != sb.releasedTimes)
        {
            return std::string(tagA) + " 与 " + tagB + " 在技能 " + skillName + " 上不一致";
        }
    }
    return {};
}

} // namespace

TEST(parallel_isolation, "双线程模拟隔离：结果与单线程逐位一致")
{
    // 两线程各跑一遍完整模拟（含构造、Initialize、180s 循环）
    StatsMap singleThread;
    StatsMap threadA;
    StatsMap threadB;

    // ---- 基准：单线程先跑一遍 ----
    singleThread = runBeamSimulationOnce(parallelTestSimConfig(), kSeed, kMaxTime, kDeltaTime);
    CHECK_FALSE(singleThread.empty());

    // ---- 两个线程同时跑（各自独立的 Person，互不共享任何监听器表） ----
    const SimConfig cfgA = parallelTestSimConfig();
    const SimConfig cfgB = parallelTestSimConfig();

    std::thread workerA([&threadA, &cfgA]()
    {
        threadA = runBeamSimulationOnce(cfgA, kSeed, kMaxTime, kDeltaTime);
    });
    std::thread workerB([&threadB, &cfgB]()
    {
        threadB = runBeamSimulationOnce(cfgB, kSeed, kMaxTime, kDeltaTime);
    });

    workerA.join();
    workerB.join();

    // ---- 比对（在主线程做，因为 CHECK_* 会改全局计数器） ----
    CHECK_FALSE(threadA.empty());
    CHECK_FALSE(threadB.empty());

    const std::string diffToSingle = diffStats(singleThread, threadA, "单线程", "线程A");
    if (!diffToSingle.empty())
    {
        CHECK_TRUE(false);   // 记一笔失败
        std::cout << "      " << diffToSingle << '\n';
    }

    const std::string diffBetween = diffStats(threadA, threadB, "线程A", "线程B");
    if (!diffBetween.empty())
    {
        CHECK_TRUE(false);
        std::cout << "      " << diffBetween << '\n';
    }

    // 总 DPS 的整数级校验：逐位相等才有意义，所以这里用精确相等
    double singleTotal = 0, threadATotal = 0;
    for (const auto& [name, s] : singleThread) singleTotal += s.damage + s.luckyDamage;
    for (const auto& [name, s] : threadA)      threadATotal += s.damage + s.luckyDamage;
    CHECK_EQ(singleTotal, threadATotal);

    std::cout << "      单线程总伤 = " << singleTotal
              << " / 线程A总伤 = " << threadATotal << '\n';
}
