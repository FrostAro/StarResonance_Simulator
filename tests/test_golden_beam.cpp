// ============================================================================
// 黄金基线测试
//
// 职责：固定种子跑完整 180 秒模拟，把「总 DPS / 各技能占比 / 计数 / 暴击率」钉住。
// 与单元测试的分工：这里抓「变了」（test_attributes.cpp 抓「算错了」）。
//
// 构造路径**不自己复制**，而是调用 Mage/Beam/SimulationHelper.h 的
// runBeamSimulationOnce —— 与 main_Beam.cpp / GUI / 对比模式共用同一份实现，
// 从根上消除「测试盯着的构造路径和实际程序走的路悄悄分叉」这个风险。
//
// 口径（照抄 printDamageStatistics，不要"统一"成一样的，它们本来就不一样）：
//   总 DPS   = Σ(damage + luckyDamage) / 秒数          ← 含幸运
//   技能占比 = skill.damage / Σ(damage + luckyDamage)  ← 分子不含幸运
//   暴击率   = Σ CritDamageCount / Σ damageCount       ← 次数比次数
//
// ⚠️ 钉值变了不要直接改数字：先确认是有意的机制改动，再更新，
//    并在提交信息里写明「旧值 → 新值」。基线变红 ≠ 测试失败 = 数值变了。
// ============================================================================

#include "TestHarness.h"
#include "Mage/Beam/SimulationHelper.h"   // runBeamSimulationOnce（内含 SimConfig）

#include <cstdint>
#include <string>
#include <unordered_map>

namespace {

constexpr int kMaxTime   = 180000;   // 180 秒（1 tick = 1ms）
constexpr int kDeltaTime = 10;       // 时间增量 10ms
constexpr std::uint32_t kSeed = 42;  // 固定种子：黄金基线必须可复现

/**
 * @brief 射线站街配置，与 main_Beam.cpp 保持一致
 *
 * ⚠️ 若这里和 main_Beam.cpp 的数值分叉，黄金基线就失去意义（它盯的是另一套参数）。
 *    改面板时两处一起改。
 */
SimConfig defaultBeamSimConfig()
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
    cfg.fantasyConfig       = 7;     // 机器人 + 嗜血毛球
    cfg.flowConfig          = 0;     // 0=急速精通流，1=幸运流
    cfg.weapon = WeaponConfig{SecondaryAttributeConfig::Proficient,
                              SecondaryAttributeConfig::Quickness,
                              0.06, 0.08};
    return cfg;
}

/// part / whole * 100，whole<=0 返回 0（与 printDamageStatistics 的防除零一致）
double percentOf(double part, double whole)
{
    return whole > 0 ? part / whole * 100.0 : 0.0;
}

/// Σ(damage + luckyDamage)：占比的分母，也是 Total DPS 的分子
double totalAllDamage(const std::unordered_map<std::string, DamageStatistics>& statsMap)
{
    double total = 0;
    for (const auto& [skillName, stat] : statsMap)
    {
        total += stat.damage + stat.luckyDamage;
    }
    return total;
}

} // namespace

// ---------------------------------------------------------------------------
// 骨架自检：证明本目标能链上射线职业代码
// ---------------------------------------------------------------------------
TEST(harness_beam_person, "骨架自检：射线角色类型可用")
{
    Mage_Beam* probe = nullptr;
    CHECK_TRUE(probe == nullptr);
}

// ---------------------------------------------------------------------------
// 黄金基线：钉住默认配置的数值
// ---------------------------------------------------------------------------
// 钉于 2026-10-21，MSVC Release 与 MinGW GCC 8.1 实测均通过。
TEST(golden_beam_default, "黄金基线：默认面板")
{
    const auto stats = runBeamSimulationOnce(defaultBeamSimConfig(), kSeed, kMaxTime, kDeltaTime);

    const double allDamage = totalAllDamage(stats);
    const double totalDPS  = allDamage / (kMaxTime / 1000.0);

    // 1. 总 DPS ±0.01%（含幸运伤害）
    CHECK_NEAR(totalDPS, 5372804.2890256951, totalDPS * 0.0001);

    // 2. 四个主力技能占比各 ±0.5pp（分子只有普通伤害）
    //    用 .at() 而不是 []：技能没出伤时直接抛异常、能立刻定位，
    //    而不是静默插入一条 damage=0 的空记录、然后让人猜"占比怎么是 0"。
    CHECK_NEAR(percentOf(stats.at(WaterSpout::name).damage,    allDamage), 44.5527, 0.5);
    CHECK_NEAR(percentOf(stats.at(Beam::name).damage,          allDamage), 14.3362, 0.5);
    CHECK_NEAR(percentOf(stats.at(IceArrow_Beam::name).damage, allDamage),  9.5197, 0.5);
    CHECK_NEAR(percentOf(stats.at(FrostBurst::name).damage,    allDamage),  9.5197, 0.5);

    // 3. 出伤/触发计数 —— 先累加成【全局合计】再钉
    //
    //    ⚠️ 6872 / 1406 / 4318 / 2594 都是**所有技能之和**，不是单个技能的
    //       （单个技能的量级是 1793 / 385 / 1670 / 1670）。别钉到 stats.at(某技能) 上。
    //    ⚠️ 为什么值得钉：整数、无浮点误差，任何 Buff 的触发次数一变立刻红，
    //       比总 DPS 的 ±0.01% 容差敏感得多；而且占比对"整体乘区变化"不敏感
    //       （大家乘同一个系数，比值不变），只有 DPS + 计数能抓住那类改动。
    double totalDamageCount      = 0;
    double totalLuckyDamageCount = 0;
    double totalCritCount        = 0;
    double totalReleasedTimes    = 0;
    for (const auto& [skillName, stat] : stats)
    {
        totalDamageCount      += stat.damageCount;
        totalLuckyDamageCount += stat.luckyDamageCount;
        totalCritCount        += stat.CritDamageCount;
        totalReleasedTimes    += stat.releasedTimes;
    }
    CHECK_EQ(totalDamageCount,      6872);
    CHECK_EQ(totalLuckyDamageCount, 1406);
    CHECK_EQ(totalCritCount,        4318);
    CHECK_EQ(totalReleasedTimes,    2594);

    // 4. 总暴击率 ±0.5pp（次数比次数，不是次数比伤害）
    CHECK_NEAR(percentOf(totalCritCount, totalDamageCount), 62.834, 0.5);
}

// ---------------------------------------------------------------------------
// 金丝雀：同种子连跑两遍，必须逐位一致
// ---------------------------------------------------------------------------
// 检测「进程级状态残留」——例如某个 Buff 忘了在析构里移除监听器，
// 第一次模拟留下的回调就会影响第二次。
//
// 现在监听器表已经下放到 Person（core/ActionListeners.h），不再是
// TypedAction 的 inline static；但**库里仍有进程级状态**会跨模拟留存：
//   · Buff::ID / Skill::ID 计数器（只增不清）
//   · SkillCreator / BuffCreator 的 creatorMap
//   · Logger / SimulationLog 的全局配置
// 所以这条金丝雀仍然必要：它盯的是"第二遍结果和第一遍一致"。
//
// ⚠️ 必须**顺序**跑：runBeamSimulationOnce 内部创建 Person 并在返回时销毁。
//    （历史上两个 Person 同时存活会栈溢出 0xC00000FD，那是监听器表全局共享
//     导致的互相激活；下放 Person 后该问题已消失，并行用例见
//     tests/test_parallel_isolation.cpp。）
TEST(golden_beam_canary, "金丝雀：同种子连跑两遍")
{
    const auto first  = runBeamSimulationOnce(defaultBeamSimConfig(), kSeed, kMaxTime, kDeltaTime);
    const auto second = runBeamSimulationOnce(defaultBeamSimConfig(), kSeed, kMaxTime, kDeltaTime);

    CHECK_EQ(first.size(), second.size());

    for (const auto& [skillName, stat1] : first)
    {
        const auto it = second.find(skillName);
        if (it == second.end())
        {
            CHECK_TRUE(false);   // 记一笔：第二遍少了这个技能
            continue;            // 不 continue 的话下面 it->second 是 UB
        }
        const auto& stat2 = it->second;

        // 这里刻意用 CHECK_EQ（精确相等）：金丝雀要的是「逐位一致」而不是「足够接近」。
        // toStr(double) 用 max_digits10 打印，末位差异能看出来。
        CHECK_EQ(stat1.damage,           stat2.damage);
        CHECK_EQ(stat1.damageCount,      stat2.damageCount);
        CHECK_EQ(stat1.luckyDamage,      stat2.luckyDamage);
        CHECK_EQ(stat1.luckyDamageCount, stat2.luckyDamageCount);
        CHECK_EQ(stat1.CritDamageCount,  stat2.CritDamageCount);
        CHECK_EQ(stat1.releasedTimes,    stat2.releasedTimes);
    }
}
