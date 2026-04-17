// ============================================================================
// 文件: Statistics.h
// 描述: 伤害统计相关的数据结构和函数声明
// 功能:
// 1. 定义 DamageStatistics 结构体，用于存储单个技能的伤害统计数据
// 2. 声明打印单次模拟统计结果的函数
// 3. 声明汇总多次模拟结果的函数（普通版和自定义版）
// ============================================================================

#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <functional>

class DamageInfo;

/**
 * @class DamageStatistics
 * @brief 伤害统计类，用于记录单个技能的伤害数据
 * 
 * 存储以下统计信息：
 * - 技能名称、总伤害、攻击次数
 * - 幸运伤害和幸运攻击次数
 * - 暴击次数
 */
class DamageStatistics {
    
public:
    std::string skillName;         ///< 技能名称
    double totalTime;              ///< 总时间（毫秒）
    double damageCount;            ///< 普通伤害次数
    double damage;                 ///< 普通伤害总值
    double luckyDamageCount;       ///< 幸运伤害次数
    double luckyDamage;            ///< 幸运伤害总值
    double CritDamageCount;        ///< 暴击次数
    int releasedTimes = 0;            ///< 技能释放次数

    DamageStatistics();
    DamageStatistics(double totalTime);
    void damageInfoAppend(const DamageInfo &damageInfo);
};

/**
 * @brief 打印单次模拟的伤害统计信息
 * 
 * @param damageStatsMap 按技能名组织的统计映射表
 * @param totalTime 模拟总时间（毫秒）
 */
void printDamageStatistics(const std::unordered_map<std::string, DamageStatistics>& damageStatsMap, 
                           int totalTime);

/**
 * @brief 汇总并打印多次模拟的结果（简化版）
 * 
 * 当模拟次数大于1时，依次打印每次模拟的结果
 * 
 * @param damageStatisticsList 多次模拟的统计结果列表
 * @param maxTime 模拟总时间（毫秒）
 */
void summaryCirculationPrint(const std::vector<std::unordered_map<std::string, DamageStatistics>>&
                           damageStatisticsList,
                           int maxTime);

/**
 * @brief 汇总并打印多次模拟的结果（带自定义函数）
 * 
 * 允许调用者传入自定义的汇总处理函数
 * 
 * @param damageStatisticsList 多次模拟的统计结果列表
 * @param maxTime 模拟总时间（毫秒）
 * @param specialSummaryFunction 自定义汇总函数
 */
void summaryCirculationPrint(const std::vector<std::unordered_map<std::string, DamageStatistics>>&
                           damageStatisticsList,
                           int maxTime, 
                           std::function<void(
                            const std::vector<std::unordered_map<std::string, DamageStatistics>>&,
                            int)> specialSummaryFunction);

