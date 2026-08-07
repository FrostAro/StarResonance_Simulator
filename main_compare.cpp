// ============================================================================
// 文件: main_compare.cpp
// 描述: 同种子配对对比（CRN）程序入口
//
// 用法:
//   1. 切换职业：修改 main() 里的 useBeam（true=射线，false=冰矛）
//   2. 修改基准配置 base（从 main_beam.cpp / main_icicle.cpp 复制站街面板）
//   3. 在 buildCandidates() 里按行添加要对比的候选配置
//   4. 编译运行 dps_simulator_compare.exe，阅读对比表
//
// 编译目标: dps_simulator_compare（见 CMakeLists.txt）
// ============================================================================

#include "core/Comparison.h"
#include "core/Logger.h"

#include "Mage/Beam/Person.h"
#include "Mage/Beam/Initializer.hpp"
#include "Mage/Icicle/Person.h"
#include "Mage/Icicle/Initializer.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ============================================================================
// 职业相关：单次模拟函数（只跑一次、返回统计表，不打印）
// ============================================================================

// 射线
static std::unordered_map<std::string, DamageStatistics> simulateBeamOnce(
    const SimConfig& cfg, std::uint32_t seed, int maxTime, int deltaTime)
{
    auto p = std::make_unique<Mage_Beam>(
        cfg.primaryAttributes, cfg.critical, cfg.quickness, cfg.lucky, cfg.proficient, cfg.almighty,
        cfg.atk, cfg.refineATK, cfg.elementATK, cfg.attackSpeed, cfg.castingSpeed,
        cfg.critialdamage_set, cfg.increasedamage_set, cfg.elementdamage_set,
        maxTime, cfg.fantasyConfig);
    p->setRandomSeed(seed);

    auto initializer = std::make_unique<Initializer_Mage_Beam>(p.get(), deltaTime, cfg.fantasyConfig);
    initializer->Initialize();

    int currentTime = 0;
    while (currentTime < maxTime)
    {
        p->autoAttackPtr->update(deltaTime);
        currentTime += deltaTime;
        p->autoAttackPtr->setTimer() += deltaTime;
    }

    p->calculateDamageStatistics();
    return p->damageStatsMap;
}

// 冰矛
static std::unordered_map<std::string, DamageStatistics> simulateIcicleOnce(
    const SimConfig& cfg, std::uint32_t seed, int maxTime, int deltaTime)
{
    auto p = std::make_unique<Mage_Icicle>(
        cfg.primaryAttributes, cfg.critical, cfg.quickness, cfg.lucky, cfg.proficient, cfg.almighty,
        cfg.atk, cfg.refineATK, cfg.elementATK, cfg.attackSpeed, cfg.castingSpeed,
        cfg.critialdamage_set, cfg.increasedamage_set, cfg.elementdamage_set,
        maxTime, cfg.fantasyConfig);
    p->setRandomSeed(seed);

    auto initializer = std::make_unique<Initializer_Mage_Icicle>(p.get(), deltaTime, cfg.fantasyConfig);
    initializer->Initialize();

    int currentTime = 0;
    while (currentTime < maxTime)
    {
        p->autoAttackPtr->update(deltaTime);
        currentTime += deltaTime;
        p->autoAttackPtr->setTimer() += deltaTime;
    }

    p->calculateDamageStatistics();
    return p->damageStatsMap;
}

// ============================================================================
// 候选配置生成：在基准上按行添加要对比的改动
// 示例：make([](SimConfig& c){ c.critical += 5; }, "+5%暴击")
// ============================================================================
static std::vector<SimConfig> buildCandidates(const SimConfig& base)
{
    std::vector<SimConfig> out;

    // 在基准上套用一份改动，生成一个候选
    auto make = [&out, &base](std::function<void(SimConfig&)> modify, std::string label)
    {
        SimConfig c = base;
        c.label = std::move(label);
        modify(c);
        out.push_back(std::move(c));
    };

    // ↓↓↓ 在这里按行定义你要对比的候选（删掉/新增即可）↓↓↓
    make([](SimConfig& c) { c.critical += 5; }, "+5%暴击");
    make([](SimConfig& c) { c.atk += 100; }, "+100攻击");
    make([](SimConfig& c) { c.proficient += 10; }, "+10%精通");
    make([](SimConfig& c) { c.fantasyConfig = 0; }, "幻想0(换)");
    make([](SimConfig& c) { c.refineATK += 100; }, "+100精炼");
    // ↑↑↑

    return out;
}

int main(int argc, char** argv)
{
    // 压低日志输出（INFO 级别的 buff/技能触发日志会刷屏），只看对比表
    Logger::initialize(Logger::Level::WARNING);

    // 模拟参数
    const int maxTime = 18000;   // 180 秒（毫秒）
    const int deltaTime = 1;     // 步长（毫秒，建议 1-3）
    int pairs = 20;              // 配对数（建议 20+，越多配对σ越稳）
    std::uint32_t firstSeed = 42;

    // 命令行可选参数：dps_simulator_compare [配对数] [起始种子]
    if (argc > 1)
        pairs = std::atoi(argv[1]);
    if (argc > 2)
        firstSeed = static_cast<std::uint32_t>(std::atoi(argv[2]));
    if (pairs <= 0)
        pairs = 1;

    // 职业切换：true=射线，false=冰矛
    const bool useBeam = true;

    // 基准配置（站街面板，从对应 main_*.cpp 复制）
    SimConfig base;
    base.label = "基准";
    SimulateFn simulate;
    if (useBeam)
    {
        base.primaryAttributes = 6760;
        base.critical = 5;
        base.quickness = 30;
        base.lucky = 5;
        base.proficient = 45;
        base.almighty = 10;
        base.atk = 5000;
        base.refineATK = 1000;
        base.elementATK = 230;
        base.attackSpeed = 0;
        base.castingSpeed = 0;
        base.critialdamage_set = 0;
        base.increasedamage_set = 0;
        base.elementdamage_set = 0;
        base.fantasyConfig = 5;
        simulate = simulateBeamOnce;
    }
    else
    {
        base.primaryAttributes = 4593;
        base.critical = 45;
        base.quickness = 1.05;
        base.lucky = 66.70;
        base.proficient = 6;
        base.almighty = 14.58;
        base.atk = 3111;
        base.refineATK = 820;
        base.elementATK = 35;
        base.attackSpeed = 0;
        base.castingSpeed = 0;
        base.critialdamage_set = 0;
        base.increasedamage_set = 0;
        base.elementdamage_set = 0;
        base.fantasyConfig = 0;
        simulate = simulateIcicleOnce;
    }

    std::cout << "职业: " << (useBeam ? "射线(Beam)" : "冰矛(Icicle)")
              << " | 配对数: " << pairs
              << " | 种子: " << firstSeed << ".." << (firstSeed + pairs - 1)
              << " | 时长: " << maxTime << "ms | 步长: " << deltaTime << "ms\n";

    auto candidates = buildCandidates(base);
    auto results = runPairedComparison(base, candidates, firstSeed, pairs, maxTime, deltaTime, simulate);
    printComparisonResults(results, pairs);

    return 0;
}
