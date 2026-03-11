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
#include "core/Logger.h"
#include "core/Statistics.h"
#include <iostream>
#include <memory>
#include <vector>
#include <random>

/**
 * @brief 执行射线职业的模拟循环
 * 
 * 根据指定参数运行多次模拟，收集伤害统计数据
 * 注：射线默认每0.5s必出一发冰箭
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
    
    // 循环执行指定次数的模拟
    while (times > 0)
    {
        int currentTime = 0;  // 当前模拟时间g
        
        // 创建射线法师角色对象，并设置基础属性
        // 参数顺序：三维属性,暴击,急速,幸运,精通,全能,攻击,精炼攻击,元素攻击,攻击速度,施法速度,
        //          爆伤额外值,增伤额外值,元素增伤额外值,程序运行总tick
        std::unique_ptr<Mage_Beam> p = std::make_unique<Mage_Beam>(
        /*三维属性*/ 4593,
        /*暴击(%)*/ 40.00,/*例如51.63*/
        /*急速(%)*/ 30.0,
        /*幸运(%)*/ 05.00,
        /*精通(%)*/ 30.00,
        /*全能(%)*/ 30.00,
        /*攻击(物理攻击/魔法攻击)*/ 3111,
        /*精炼攻击*/ 820,
        /*元素攻击*/ 35,
        /*攻击速度(%)*/ 10.00,
        /*施法速度(%)*/ 60.00,
        /*爆伤额外值*/ 0,/*例如0.2*/
        /*增伤额外值*/ 0,
        /*元素增伤额外值*/ 0,
        /*程序运行总tick*/ maxTime,
        /*幻想配置*/ 0);
        
        // 设置随机种子
        if (isRandomSeed) {
            // 使用真随机数种子
            p->setRandomSeed(std::random_device{}());
        } else {
            // 使用固定种子，确保模拟可重现
            p->setRandomSeed(seed);
        }
        
        // 初始化角色（装备技能、设置buff等）
        auto Initializer = std::make_unique<Initializer_Mage_Beam>(p.get(), deltaTime,0);
        Initializer->Initialize();
        
        // 开始模拟运行
        std::cout << "Starting simulation..." << std::endl;
        while (currentTime < maxTime)
        {
            // 更新自动攻击系统
            p->autoAttackPtr->update(deltaTime);
            
            // 更新当前时间
            currentTime += deltaTime;
            p->autoAttackPtr->setTimer() += deltaTime;
        }
        
        // 计算伤害统计信息
        p->calculateDamageStatistics();
        std::cout << "Simulation ended." << std::endl;
        
        // 打印本次模拟的详细统计
        printDamageStatistics(p->damageStatsMap, maxTime);
        
        // 将统计结果添加到列表中
        damageStatisticsList.push_back(p->damageStatsMap);
        
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
    // 1. 初始化日志系统，设置日志级别为DEBUG
    Logger::initialize(Logger::Level::INFO);
    
    // 2. 伤害统计结果列表（用于存储多次模拟的结果）
    std::vector<std::unordered_map<std::string, DamageStatistics>> damageStatisticsList;
    
    // 3. 模拟参数配置
    const int maxTime = 18000;       // 最大运行时间：180秒
    const int deltaTime = 1;         // 时间增量：0.01s  (建议值：1-3)
    
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