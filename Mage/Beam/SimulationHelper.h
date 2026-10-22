#pragma once
#include "Person.h"
#include "Initializer.hpp"
#include "../../core/Comparison.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

/**
 * @file SimulationHelper.h
 * @brief 射线单次模拟的共享实现
 *
 * @details 这是唯一一份「构造角色 → 设种子 → 初始化 → 跑循环 → 收集统计」的实现，
 *          供 GUI（两个 Worker）、控制台对比模式、main_Beam、黄金基线测试共用。
 *          共用是为了避免参数分叉：站街面板、武器归类、fantasyConfig 一旦各写一份，
 *          就会随时间悄悄不一致（曾经 GUI 不传 weaponConfig，同一份面板在 GUI
 *          和 CLI 算出不同 DPS）。
 *
 * @note 线程安全约定（做并行批量对比的前提）：
 *       本函数是纯函数式的，每次调用都新建自己的 Mage_Beam、只读入参、按值返回
 *       统计表，因此可以从 N 个线程同时调用，只要各线程传入自己的 cfg 和 seed。
 *       这是「监听器表下放 Person」重构的核心产出，由
 *       tests/test_parallel_isolation.cpp 守护（双线程与单线程结果逐位一致）。
 *
 *       与模拟无关的进程级共享状态，多线程调用时需注意：
 *       Buff::ID / Skill::ID 是 atomic，只保证号码唯一；
 *       SkillCreator / BuffCreator 的 creatorMap 每次 Initialize 都会 insert，
 *       Initializer 已用静态互斥锁把注册段串行化；
 *       Logger 输出由内部 mutex 保护，但级别是全局的，不要在多线程里各自 setLevel；
 *       SimulationLog 不是线程安全的（ofstream + 非原子 bool），不要在并行路径调用。
 */

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
        person->autoAttackPtr->setTimer(person->getSimulationTime() + deltaTime);
    }

    person->calculateDamageStatistics();
    return person->damageStatsMap;
}

} // namespace beam_sim_detail

/**
 * @brief 跑一次射线模拟，返回按技能名统计的结果表
 *
 * ⚠️ 这四个参数必须保持恰好 4 个、没有默认参数。
 *    因为 main_compare.cpp / mainwindow.cpp 会写 `SimulateFn simulate = runBeamSimulationOnce;`
 *    —— 而默认参数不改变函数类型：一旦声明成 5 个参数（哪怕第 5 个有默认值），
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
