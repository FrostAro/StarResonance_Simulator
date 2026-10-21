// ============================================================================
// 文件: main_beam.cpp
// 描述: 射线 (Beam) 职业伤害模拟主程序入口
// 功能:
// 1. 初始化日志系统
// 2. 设置射线职业的模拟参数
// 3. 执行单次或多次模拟循环
// 4. 输出伤害统计结果
// ============================================================================

#include "Mage/Beam/Person.h"
#include "Mage/Beam/Initializer.hpp"
#include "Mage/Beam/SimulationHelper.h"

#include "core/Logger.h"
#include "core/Statistics.h"
#include "core/SimulationLog.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief 执行射线职业的模拟循环
 *
 * 根据指定参数运行多次模拟，收集伤害统计数据
 * 注：射线默认每0.5s必出一发冰箭
 *
 * 「构造角色 → 初始化 → 跑循环 → 收集统计」这段不再自己实现，而是调用
 * Mage/Beam/SimulationHelper.h 的 runBeamSimulationOnce —— 与 GUI、对比模式、
 * 黄金基线测试共用同一份代码，避免参数（站街面板/武器归类/fantasyConfig）
 * 在各入口之间悄悄分叉。这里只保留 CLI 独有的部分：随机种子、调试日志、打印。
 *
 * @param damageStatisticsList 输出参数，存储所有模拟的伤害统计结果
 * @param times 模拟循环次数
 * @param maxTime 每次模拟的最大运行时间（毫秒）
 * @param deltaTime 每次update的时间增量（毫秒）
 * @param isRandomSeed 是否使用随机种子（true=随机，false=固定）
 * @param seed 指定的随机数种子，当isRandomSeed=false时使用
 */
void executeSimulation_Beam(std::vector<std::unordered_map<std::string, DamageStatistics>>& damageStatisticsList,
                            int times,
                            int maxTime,
                            int deltaTime,
                            bool isRandomSeed,
                            std::uint32_t seed)
{
    // 参数验证：times必须大于0
    if (times <= 0)
        return;

    // 站街面板：与 GUI「射线」默认值、tests 黄金基线共用同一套数值
    SimConfig cfg;
    cfg.primaryAttributes = 9272;
    cfg.critical          = 10.12;
    cfg.quickness         = 38.18;
    cfg.lucky             = 23.21;
    cfg.proficient        = 74.8;
    cfg.almighty          = 20.88;
    cfg.atk               = 8151;
    cfg.refineATK         = 1100;
    cfg.elementATK        = 430;
    cfg.attackSpeed       = 0.0;
    cfg.castingSpeed      = 0.0;
    cfg.criticaldamage_set  = 0.0;
    cfg.increasedamage_set  = 0.0;
    cfg.elementdamage_set   = 0.0;
    cfg.fantasyConfig     = 7;   // 机器人 + 嗜血毛球
    cfg.flowConfig        = 0;   // 0=急速精通流，1=幸运流
    cfg.weapon = WeaponConfig{SecondaryAttributeConfig::Proficient,
                              SecondaryAttributeConfig::Quickness,
                              0.06, 0.08};

    // Person 构造参数描述（用于日志配置头）
    const std::string personParams =
        "三维=" + std::to_string(cfg.primaryAttributes) 
        + ", 暴击=" + std::to_string(cfg.critical) 
        + ", 急速=" + std::to_string(cfg.quickness) 
        + ", 幸运=" + std::to_string(cfg.lucky) 
        + ", 精通=" + std::to_string(cfg.proficient) 
        + ", 全能=" + std::to_string(cfg.almighty) 
        + ", 攻击=" + std::to_string(cfg.atk) 
        + ", 精炼=" + std::to_string(cfg.refineATK) 
        + ", 元素=" + std::to_string(cfg.elementATK) 
        + ", 攻速=" + std::to_string(cfg.attackSpeed) 
        + ", 施速=" + std::to_string(cfg.castingSpeed) 
        + ", 爆伤=" + std::to_string(cfg.criticaldamage_set) 
        + ", 增伤=" + std::to_string(cfg.increasedamage_set) 
        + ", 元素增伤=" + std::to_string(cfg.elementdamage_set) 
        + ", 幻想配置=" + std::to_string(cfg.fantasyConfig)
        + ", 流派配置=" + std::to_string(cfg.flowConfig)
        + ", 武器第一属性=" + std::to_string(static_cast<int>(cfg.weapon.first))
        + ", 武器第二属性=" + std::to_string(static_cast<int>(cfg.weapon.second))
        + ", 武器增效=" + std::to_string(cfg.weapon.enhanceIncrease)
        + ", 武器额外增伤=" + std::to_string(cfg.weapon.extraIncrease);

    // 循环执行指定次数的模拟
    while (times > 0)
    {
        // 随机种子每轮重新掷（与旧实现一致：times>1 且随机时每轮不同）
        const std::uint32_t runSeed = isRandomSeed ? std::random_device{}() : seed;

        // 记录是否开启了本次的调试日志（多次模拟只取第一次）
        bool loggedFirst = false;

        auto stats = runBeamSimulationOnceWithHook(
            cfg, runSeed, maxTime, deltaTime,
            [&](Person* p)
            {
                // 初始化完成、主循环开始前：开日志 + 打印提示（与旧实现同一时机）
                loggedFirst = SimulationLog::begin("beam", p, personParams);
                std::cout << "Starting simulation..." << std::endl;
            });

        // 模拟结束：收尾日志文件，恢复 INFO 级别避免后续刷屏
        if (loggedFirst)
        {
            SimulationLog::end();
            Logger::setLevel(Logger::Level::INFO);
        }

        std::cout << "Simulation ended." << std::endl;

        // 打印本次模拟的详细统计
        printDamageStatistics(stats, maxTime);

        // 将统计结果添加到列表中
        damageStatisticsList.push_back(std::move(stats));

        times--;
    }
}

/**
 * @brief 程序主入口点
 * 
 * 主要流程：
 * 1. 初始化日志系统
 * 2. 配置模拟参数
 * 3. 执行模拟循环（射线职业）
 * 4. 汇总输出结果
 * 
 * @return int 程序退出码，0表示成功
 */
int main()
{
    // 1. 初始化日志系统，设置日志级别为INFO
    Logger::initialize(Logger::Level::INFO);
    
    // 2. 伤害统计结果列表（用于存储多次模拟的结果）
    std::vector<std::unordered_map<std::string, DamageStatistics>> damageStatisticsList;
    
    // 3. 模拟参数配置
    const int maxTime = 180000;      // 最大运行时间：180秒（1 tick = 1ms）
    const int deltaTime = 10;        // 时间增量：10ms  (建议值：10-30)
    
    // 4. 执行模拟
    // 参数说明：
    // - damageStatisticsList: 输出参数，存储统计结果
    // - 1: 模拟循环次数
    // - maxTime: 最大运行时间
    // - deltaTime: 时间增量
    // - true: 使用随机种子（每次运行结果不同）
    // - 42: 种子值（当 isRandomSeed=false 时有效，此处未使用）
    executeSimulation_Beam(damageStatisticsList, 1, maxTime, deltaTime, true, 42);
    
    // 5. 汇总输出（如果有多次循环，进行汇总）
    summaryCirculationPrint(damageStatisticsList, maxTime);
    
    //system("pause"); // Windows系统下暂停，等待用户按键后退出
    return 0;  // 程序正常退出
}
