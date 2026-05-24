// ============================================================================
// 文件: Statistics.cpp
// 描述: 伤害统计相关函数的实现
// ============================================================================

#include "Statistics.h"
#include <iostream>
#include "Info.h"

/**
 * @brief 打印单次模拟的详细伤害统计
 * 
 * 输出每个技能的总伤害、攻击次数、幸运伤害、DPS 等，
 * 并计算总体统计信息（总伤害、总 DPS、幸运触发率、暴击率等）。
 * 
 * @param damageStatsMap 技能统计映射表
 * @param totalTime 模拟总时间（毫秒）
 */
void printDamageStatistics(const std::unordered_map<std::string, DamageStatistics>& damageStatsMap, 
                           int totalTime)
{
    // 计算总体统计
    double totalDamage = 0;
    double totalLuckyDamage = 0;
    double totalAllDamage = 0;
    double totalDamageCount = 0;
    double totalLuckyDamageCount = 0;
    double totalCritCount = 0;
    
    for (const auto &info : damageStatsMap)
    {
        totalDamage += info.second.damage;
        totalLuckyDamage += info.second.luckyDamage;
        totalAllDamage += info.second.damage + info.second.luckyDamage;
        totalDamageCount += info.second.damageCount;
        totalLuckyDamageCount += info.second.luckyDamageCount;
        totalCritCount += info.second.CritDamageCount;
    }

    // 打印每个技能的统计数据
    std::cout << "skill Damage Statistics:" << std::endl;
    for (const auto &info : damageStatsMap)
    {
        if(info.second.skillName == "NONE")
            continue; // 跳过无效技能统计
        std::cout << "Skill: " << info.second.skillName
                  << ", Damage: " << info.second.damage
                  << ", Damage attack count: " << info.second.damageCount
                  << ", Lucky Damage: " << info.second.luckyDamage
                  << ", Lucky Attack count: " << info.second.luckyDamageCount
                  << ", DPS: " << info.second.damage / (static_cast<double>(totalTime) / 100.0)
                  << ", Propotion:" << (info.second.damage / totalAllDamage) * 100 << "%"
                  << std::endl;
    }

    std::cout << "Lucky: " 
                  << " Damage: " << totalLuckyDamage
                  << ", Damage attack count: " << totalLuckyDamageCount
                  << ", DPS: " << totalLuckyDamage / (static_cast<double>(totalTime) / 100.0)
                  << ", Propotion:" << (totalLuckyDamage / totalAllDamage) * 100 << "%"
                  << std::endl;
    
    // 打印总体统计
    std::cout << std::endl
              << "total Damage Statistics:" << std::endl;
    
    std::cout << "Total Damage: " << totalDamage
              << ", Total DPS: " << (totalDamage + totalLuckyDamage) / (static_cast<double>(totalTime) / 100.0)
              << std::endl;
    
    std::cout << "Total Damage Count: " << totalDamageCount
              << ", Total Lucky Damage Count: " << totalLuckyDamageCount
              << ", Lucky rate:" << (totalLuckyDamageCount / totalDamageCount) * 100 << "%"
              << ", Crit rate:" << (totalCritCount / totalDamageCount) * 100 << "%"
              << std::endl;
}

/**
 * @brief 汇总多次模拟的结果（简化版）
 * 
 * 仅当模拟次数大于1时，依次打印每次模拟的结果
 * 
 * @param damageStatisticsList 多次模拟的统计列表
 * @param maxTime 模拟总时间（毫秒）
 */
void summaryCirculationPrint(const std::vector<std::unordered_map<std::string, DamageStatistics>>&
                           damageStatisticsList,
                           int maxTime)
{
    if (damageStatisticsList.size() <= 1) 
        return;
    
    std::cout << "Summary of multiple simulations:" << std::endl;
    size_t simCount = damageStatisticsList.size();
    
    // 用于累加所有模拟中各技能的数据
    std::unordered_map<std::string, DamageStatistics> accumulatedMap;
    
    // 遍历每次模拟
    for (const auto& statsMap : damageStatisticsList)
    {
        for (const auto& [skillName, stat] : statsMap)
        {
            auto& acc = accumulatedMap[skillName]; // 自动创建新条目或获取已有
            acc.skillName = skillName;             // 技能名保持一致
            // 累加各项统计数据
            acc.damage += stat.damage;
            acc.damageCount += stat.damageCount;
            acc.luckyDamage += stat.luckyDamage;
            acc.luckyDamageCount += stat.luckyDamageCount;
            acc.CritDamageCount += stat.CritDamageCount;
            // totalTime 在此处无用，忽略
        }
    }
    
    // 计算平均值
    for (auto& [skillName, acc] : accumulatedMap)
    {
        acc.damage /= simCount;
        acc.damageCount /= simCount;
        acc.luckyDamage /= simCount;
        acc.luckyDamageCount /= simCount;
        acc.CritDamageCount /= simCount;
    }
    
    // 输出平均后的统计结果
    std::cout << "Average statistics over " << simCount << " simulations:" << std::endl;
    printDamageStatistics(accumulatedMap, maxTime);
}

/**
 * @brief 汇总多次模拟的结果（带自定义函数）
 * 
 * 调用用户提供的自定义函数对统计列表进行处理
 * 
 * @param damageStatisticsList 多次模拟的统计列表
 * @param maxTime 模拟总时间（毫秒）
 * @param specialSummaryFunction 自定义汇总函数
 */
void summaryCirculationPrint(const std::vector<std::unordered_map<std::string, DamageStatistics>>&
                           damageStatisticsList,
                           int maxTime, 
                           std::function<void(
                            const std::vector<std::unordered_map<std::string, DamageStatistics>>&,
                            int)> specialSummaryFunction)
{
    if (damageStatisticsList.size() <= 1) 
        return;
    
    std::cout << "Summary of multiple simulations:" << std::endl;
    
    specialSummaryFunction(damageStatisticsList, maxTime);
}

DamageStatistics::DamageStatistics()
    : skillName("NONE"),
      totalTime(0),
      damageCount(0),
      damage(0),
      luckyDamageCount(0),
      luckyDamage(0),
      CritDamageCount(0)
{}

DamageStatistics::DamageStatistics(double totalTime)
    : skillName("NONE"),
      totalTime(totalTime),
      damageCount(0),
      damage(0),
      luckyDamageCount(0),
      luckyDamage(0),
      CritDamageCount(0)
{}

void DamageStatistics::damageInfoAppend(const DamageInfo &damageInfo)
{
    damageCount += 1;
    damage += damageInfo.damageNum;

    if (damageInfo.isLucky)
    {
        luckyDamageCount += 1;
        luckyDamage += damageInfo.luckyNum;
    }
    if (damageInfo.isCritical)
    {
        CritDamageCount += 1;
    }
}
