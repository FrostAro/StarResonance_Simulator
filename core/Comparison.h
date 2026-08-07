// ============================================================================
// 文件: Comparison.h
// 描述: 同种子配对对比（CRN, Common Random Numbers）模块
//
// 用途:
// 比较"基准配置"与多个"候选配置"的 DPS 提升时，普通做法是各自独立随机跑 N 次
// 再比平均值 —— 但两次平均值各带一份"程序随机"噪声（技能触发/Buff时机/能量循环），
// 1% 的小提升会被噪声淹没。
//
// 本模块让基准与每个候选在【同一组随机种子】下配对运行：
//     Δi = DPS候选(seed_i) − DPS基准(seed_i)
// 同一秒内两份模拟共享相同的随机"天气"，逐对相减时这份噪声互相抵消，
// Δ 中只剩配置本身带来的差异 → 方差骤降，小提升也能稳定测出。
//
// 依赖前提: Person::setRandomSeed() 已存在，随机完全由 mt19937 种子驱动，
// 且 base/候选在相同配置路径下按相同顺序消费随机流。
// ============================================================================

#pragma once

#include "Statistics.h"
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================================
// SimConfig - 一次模拟所需的全部配置
// 两个职业（射线/冰矛）的 Person 构造签名一致（16 个参数），因此可共用此结构
// ============================================================================
struct SimConfig
{
    std::string label;              // 配置名（用于对比表输出）

    double primaryAttributes = 0;   // 三维属性
    double critical = 0;            // 暴击(%)
    double quickness = 0;           // 急速(%)
    double lucky = 0;               // 幸运(%)
    double proficient = 0;          // 精通(%)
    double almighty = 0;            // 全能(%)
    int atk = 0;                    // 攻击(物理/魔法)
    int refineATK = 0;              // 精炼攻击
    int elementATK = 0;             // 元素攻击
    double attackSpeed = 0;         // 额外攻击速度(%)
    double castingSpeed = 0;        // 额外施法速度(%)
    double critialdamage_set = 0;   // 爆伤额外值
    double increasedamage_set = 0;  // 增伤额外值
    double elementdamage_set = 0;   // 元素增伤额外值
    int fantasyConfig = 0;          // 幻想配置
};

// ============================================================================
// SimulateFn - 单次模拟函数签名
// 职业相关：由调用方提供（main_compare.cpp 中分别封装射线/冰矛）
// 返回: 按技能名统计的伤害结果表（含幸运期望）
// ============================================================================
using SimulateFn = std::function<std::unordered_map<std::string, DamageStatistics>(
    const SimConfig& config, std::uint32_t seed, int maxTime, int deltaTime)>;

/**
 * @brief 从统计表计算总 DPS（普通伤害 + 幸运期望伤害，与统计输出一致）
 */
double getTotalDPS(const std::unordered_map<std::string, DamageStatistics>& statsMap, int maxTime);

// ============================================================================
// ComparisonResult - 单个候选配置的配对对比结果
// ============================================================================
struct ComparisonResult
{
    std::string label;          // 候选配置名
    double baseDps = 0;         // 基准平均 DPS（对所有候选相同）
    double candDps = 0;         // 候选平均 DPS
    double deltaDps = 0;        // candDps - baseDps
    double deltaPercent = 0;    // 相对提升 (%)
    double baseStd = 0;         // 基准 DPS 标准差（未配对，仅参考）
    double candStd = 0;         // 候选 DPS 标准差（未配对，仅参考）
    double deltaStd = 0;        // 配对差值标准差（CRN 核心指标，应远小于上面两者）
    double tValue = 0;          // 配对 t 值 = mean(delta) / (deltaStd / sqrt(N))
    int pairs = 0;              // 配对数
};

/**
 * @brief 执行配对对比（CRN）
 *
 * 基准只跑 seedCount 次并缓存结果（确定性可复用），每个候选与基准在相同种子下
 * 逐对相减。
 *
 * @param base       基准配置
 * @param candidates 候选配置列表
 * @param firstSeed  起始种子（使用 firstSeed..firstSeed+seedCount-1 这一组种子）
 * @param seedCount  配对数（建议 20+）
 * @param maxTime    每次模拟总时长（毫秒）
 * @param deltaTime  模拟步长（毫秒）
 * @param simulate   单次模拟函数（职业相关）
 * @return 每个候选的对比结果
 */
std::vector<ComparisonResult> runPairedComparison(
    const SimConfig& base,
    const std::vector<SimConfig>& candidates,
    std::uint32_t firstSeed,
    int seedCount,
    int maxTime,
    int deltaTime,
    const SimulateFn& simulate);

/**
 * @brief 打印对比结果表
 */
void printComparisonResults(const std::vector<ComparisonResult>& results, int pairs);
