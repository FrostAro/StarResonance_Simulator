#pragma once
#include "Person.h"
#include "Initializer.hpp"
#include "../../core/Comparison.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

// ============================================================================
// 射线单次模拟的共享实现
//
// 这是**唯一**一份「构造角色 → 设种子 → 初始化 → 跑循环 → 收集统计」的实现，
// 供四处共用：
//   · GUI（mainwindow.cpp 的 SimulationWorker / ComparisonWorker）
//   · 控制台对比模式（main_compare.cpp）
//   · 控制台主程序（main_Beam.cpp）
//   · 黄金基线测试（tests/test_golden_beam.cpp）
//
// 共用这份实现的理由：站街面板、武器归类、fantasyConfig 这些参数一旦在两个
// 入口各写一份，就会随时间悄悄分叉（例如曾经 GUI 不传 weaponConfig，同一份
// 面板在 GUI 和 CLI 算出不同 DPS）。放在这里改一处即全部生效。
// ============================================================================

namespace beam_sim_detail {

/**
 * @brief 真正的实现。外部请用下面三个具名入口，不要直接调这个。
 *
 * @param afterInit 可选钩子：Initialize() 之后、主循环之前调用，参数是刚建好的
 *                  Person*。main_Beam 用它开 SimulationLog；其余调用方传空。
 */
inline std::unordered_map<std::string, DamageStatistics> runOnceImpl(
    const SimConfig& cfg,
    std::uint32_t seed,
    int maxTime,
    int deltaTime,
    const std::function<void(Person*)>& afterInit)
{
    auto person = std::make_unique<Mage_Beam>(
        cfg.primaryAttributes, cfg.critical, cfg.quickness, cfg.lucky, cfg.proficient, cfg.almighty,
        cfg.atk, cfg.refineATK, cfg.elementATK,
        cfg.attackSpeed, cfg.castingSpeed,
        cfg.criticaldamage_set, cfg.increasedamage_set, cfg.elementdamage_set,
        maxTime, cfg.fantasyConfig, cfg.weapon);
    person->setRandomSeed(seed);

    auto init = std::make_unique<Initializer_Mage_Beam>(person.get(), deltaTime, cfg.fantasyConfig, cfg.flowConfig);
    init->Initialize();

    if (afterInit)
    {
        afterInit(person.get());
    }

    int currentTime = 0;
    while (currentTime < maxTime)
    {
        person->autoAttackPtr->update(deltaTime);
        currentTime += deltaTime;
        person->autoAttackPtr->setTimer() += deltaTime;
    }

    person->calculateDamageStatistics();
    return person->damageStatsMap;
}

} // namespace beam_sim_detail

/**
 * @brief 跑一次射线模拟，返回按技能名统计的结果表
 *
 * ⚠️ 这四个参数必须保持**恰好 4 个、没有默认参数**。
 *    因为 main_compare.cpp / mainwindow.cpp 会写 `SimulateFn simulate = runBeamSimulationOnce;`
 *    —— 而**默认参数不改变函数类型**：一旦声明成 5 个参数（哪怕第 5 个有默认值），
 *    函数指针类型就变成 5 参，再也无法赋给 4 参的 std::function。
 */
inline std::unordered_map<std::string, DamageStatistics> runBeamSimulationOnce(
    const SimConfig& cfg, std::uint32_t seed, int maxTime, int deltaTime)
{
    return beam_sim_detail::runOnceImpl(cfg, seed, maxTime, deltaTime, {});
}

/**
 * @brief 同上，但允许在 Initialize() 之后、主循环之前插入一段逻辑
 *
 * main_Beam 用它开 SimulationLog（需要拿到 Person*）。
 * 单独一个函数而不是给上面那个加默认参数，原因见上面的注释。
 */
inline std::unordered_map<std::string, DamageStatistics> runBeamSimulationOnceWithHook(
    const SimConfig& cfg,
    std::uint32_t seed,
    int maxTime,
    int deltaTime,
    const std::function<void(Person*)>& afterInit)
{
    return beam_sim_detail::runOnceImpl(cfg, seed, maxTime, deltaTime, afterInit);
}
