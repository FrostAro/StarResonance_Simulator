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
 *          调用方各自的关注点（取消、调试日志开关）通过 BeamRunHooks 注入，
 *          不要让调用方再复制一遍循环 —— 复制出来的那份迟早会漏掉某个修正。
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

/**
 * @brief 单次模拟的三个可选钩子：取消与调试日志都挂在这里
 *
 * @details 这些是「调用方关注点」，不属于模拟本身，所以不放进 SimConfig
 *          （SimConfig 要能被候选 lambda 整体复制，钩子带状态、不可复制语义）。
 */
struct BeamRunHooks
{
    /**
     * @brief Initialize() 之后、主循环之前调用，参数是刚建好的 Person
     * @details main_Beam 用它开 SimulationLog（需要注册表已填充，故必须在 Initialize 之后）
     */
    std::function<void(Person*)> afterInit;

    /**
     * @brief 主循环结束、calculateDamageStatistics() 之前调用
     * @details SimulationLog 必须在这里关：放 afterInit 里关会把整段日志丢掉，
     *          放到统计之后又会把统计输出混进文件
     */
    std::function<void(Person*)> beforeStats;

    /**
     * @brief 每个 tick 开始前问一次，返回 true 则提前结束本次模拟（GUI 取消）
     * @warning 会被多个工作线程并发调用，必须线程安全（通常是读一个 atomic 标志）；
     *          不能用 QThread::currentThread()->isInterruptionRequested()，
     *          在 std::thread 里它恒为 false，取消会静默失效
     */
    std::function<bool()> shouldStop;
};

namespace beam_sim_detail {

/**
 * @brief 真正的实现。外部请用下面的具名入口，不要直接调这个。
 *
 * @param hooks 三个可选钩子，全空即等价于「跑完一整轮、什么都不插」
 */
inline std::unordered_map<std::string, DamageStatistics> runOnceImpl(
    const SimConfig& cfg,
    std::uint32_t seed,
    int maxTime,
    int deltaTime,
    const BeamRunHooks& hooks)
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

    if (hooks.afterInit)
    {
        hooks.afterInit(person.get());
    }

    int currentTime = 0;
    while (currentTime < maxTime)
    {
        if (hooks.shouldStop && hooks.shouldStop())
        {
            break;
        }

        person->autoAttackPtr->update(deltaTime);
        currentTime += deltaTime;
        person->autoAttackPtr->setTimer(person->getSimulationTime() + deltaTime);
    }

    if (hooks.beforeStats)
    {
        hooks.beforeStats(person.get());
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
    return beam_sim_detail::runOnceImpl(cfg, seed, maxTime, deltaTime, BeamRunHooks{});
}

/**
 * @brief 带钩子的版本（GUI / main_Beam 用），其余三个参数同 runBeamSimulationOnce
 *
 * @details 不能给 4 参版本加一个带默认值的钩子参数：默认参数不改变函数类型，
 *          但一旦声明成 5 个形参，函数指针类型就变成 5 参，
 *          `SimulateFn simulate = runBeamSimulationOnce;` 这种写法会编译失败。
 *          所以「插钩子」必须显式写 5 参，调用点一眼能看出这里额外做了事。
 */
inline std::unordered_map<std::string, DamageStatistics> runBeamSimulationOnceWithHooks(
    const SimConfig& cfg,
    std::uint32_t seed,
    int maxTime,
    int deltaTime,
    const BeamRunHooks& hooks)
{
    return beam_sim_detail::runOnceImpl(cfg, seed, maxTime, deltaTime, hooks);
}
