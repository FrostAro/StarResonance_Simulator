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
#include "Person.h"          // WeaponConfig / SecondaryAttributeConfig
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
    double criticaldamage_set = 0;   // 爆伤额外值
    double increasedamage_set = 0;  // 增伤额外值
    double elementdamage_set = 0;   // 元素增伤额外值
    int fantasyConfig = 0;          // 幻想配置
    int flowConfig = 0;             // 流派配置：0=急速精通流，1=幸运流

    // 武器第一/第二属性。这两项在游戏里会加进站街面板，所以输入的面板值已经
    // 包含它们；Person 构造函数会用 reclassify 把这段百分比从点数通道切到
    // 额外通道（总值不变）。GUI 与 CLI 必须用同一套值，否则同一份面板会算出不同 DPS。
    WeaponConfig weapon;
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

/**
 * @brief 单个候选配置的配对对比结果（表格的一行）
 *
 * @details deltaPercent 容易被噪声干扰，真正可信的是 deltaStd 与 tValue：
 *          deltaStd 是 Δi 的样本标准差，CRN 生效时应远小于 baseStd / candStd；
 *          若三者量级接近，说明随机流已分叉（候选改动了随机消耗顺序）。
 *          tValue 的绝对值 >= 2 时（N≈20）可认为差异真实；tValue 为 0 表示
 *          deltaStd 为 0 或 N<2，不是"没有提升"的意思。
 */
struct ComparisonResult
{
    std::string label;          // 候选配置名
    double baseDps = 0;         // 基准平均 DPS（所有候选共用）
    double candDps = 0;         // 候选平均 DPS
    double deltaDps = 0;        // candDps - baseDps
    double deltaPercent = 0;    // 相对提升 (%)
    double baseStd = 0;         // 基准 DPS 样本标准差（仅对照）
    double candStd = 0;         // 候选 DPS 样本标准差（仅对照）
    double deltaStd = 0;        // 配对差值的样本标准差（CRN 核心指标）
    double tValue = 0;          // 配对 t 值 = mean(Δ) / (deltaStd / sqrt(N))
    int pairs = 0;              // 实际使用的配对数
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
 * @param shouldStop 可选提前终止回调：返回 true 时停止并返回已完成的候选结果
 *                   （用于GUI响应窗口关闭/用户中断，控制台可不传）
 * @return 每个候选的对比结果
 */
std::vector<ComparisonResult> runPairedComparison(
    const SimConfig& base,
    const std::vector<SimConfig>& candidates,
    std::uint32_t firstSeed,
    int seedCount,
    int maxTime,
    int deltaTime,
    const SimulateFn& simulate,
    const std::function<bool()>& shouldStop = {});

/**
 * @brief 并行初始化基准：把 seedCount 个种子分摊到 threadCount 个线程
 *
 * @details 前 remainingSeeds 个线程各多跑一个种子，其余线程只跑 seedsPerThread 个。
 *          每个线程只写自己那几个下标，所以不需要锁。
 *
 * @param base          基准配置
 * @param firstSeed     起始种子，使用 firstSeed..firstSeed+seedCount-1
 * @param seedCount     配对数即种子个数
 * @param maxTime       单次模拟时长（毫秒）
 * @param deltaTime     模拟步长（毫秒）
 * @param threadCount   并行线程数，会被夹到 [1, seedCount]
 * @param simulate      单次模拟函数
 * @param baseDpsBySeed 输出参数，函数内会 resize 到 seedCount
 * @param shouldStop    可选取消回调，在每个种子开始前于工作线程内调用
 * @return true = 全部种子跑完；false = 被取消，baseDpsBySeed 可能只填了一部分
 *
 * @warning shouldStop 会被多个工作线程并发调用，必须线程安全。
 *          注意不能用 QThread::currentThread()->isInterruptionRequested()：
 *          它在非 Qt 线程上恒为 false，会让取消静默失效；应传一个 atomic 标志。
 * @note 取消延迟上限是一次模拟的耗时（约 0.35 秒），已开始的那次不会被打断。
 */
bool threadInitializeBase(
    const SimConfig& base,
    std::uint32_t firstSeed,
    int seedCount,
    int maxTime,
    int deltaTime,
    int threadCount,
    const SimulateFn& simulate,
    std::vector<double>& baseDpsBySeed,
    const std::function<bool()>& shouldStop = {});

/**
 * @brief 并行执行候选配对对比：先并行填 [候选][种子] 矩阵，再单线程聚合
 *
 * @details 每个 (候选, 种子) 是一个独立任务，用原子取号分配，天然处理余数；
 *          聚合阶段按种子升序遍历，浮点求和顺序与串行一致，结果与
 *          runPairedComparison 逐位相同。
 *
 * @param baseDpsBySeed 已由 threadInitializeBase 填好的基准 DPS，每种子一个
 * @param cands         候选配置列表
 * @param firstSeed     起始种子
 * @param seedCount     配对数即种子个数
 * @param maxTime       单次模拟时长（毫秒）
 * @param deltaTime     模拟步长（毫秒）
 * @param threadCount   并行线程数，会被夹到 [1, seedCount]
 * @param simulate      单次模拟函数
 * @param results       输出参数，函数开头会 clear，每个完整候选一行
 * @param shouldStop    可选取消回调，在每个任务开始前于工作线程内调用
 * @return true = 全部任务跑完；false = 被取消
 *
 * @warning shouldStop 会被多个工作线程并发调用，必须线程安全；
 *          不要用 QThread 的中断位（在非 Qt 线程上恒为 false）。
 * @note 取消时只输出「所有种子都已完成」的候选，避免用残缺样本算出错误统计。
 */
bool threadComparison(
    const std::vector<double>& baseDpsBySeed,
    const std::vector<SimConfig>& cands,
    std::uint32_t firstSeed,
    int seedCount,
    int maxTime,
    int deltaTime,
    int threadCount,
    const SimulateFn& simulate,
    std::vector<ComparisonResult>& results,
    const std::function<bool()>& shouldStop = {});

/**
 * @brief 并行配对对比：先并行跑基准，再并行跑候选
 *
 * @details 阶段一由 threadInitializeBase 得到每种子一个基准 DPS；阶段二由
 *          threadComparison 并行填 [候选][种子] 矩阵后单线程聚合。
 *          两阶段必须串行：阶段二的 CRN 配对要拿阶段一的基准逐种子相减，
 *          基准不完整就没有可比对的参照。结果与 runPairedComparison 逐位相同。
 *
 * @param base        基准配置
 * @param candidates  候选配置列表
 * @param firstSeed   起始种子
 * @param seedCount   配对数即种子个数
 * @param maxTime     单次模拟时长（毫秒）
 * @param deltaTime   模拟步长（毫秒）
 * @param threadCount 并行线程数，会被夹到 [1, seedCount]
 * @param simulate    单次模拟函数
 * @param results     输出参数，函数开头会 clear，每个完整候选一行
 * @param shouldStop  可选取消回调，在每个任务开始前于工作线程内调用
 * @return true = 全部跑完；false = 被取消
 *
 * @warning shouldStop 会被多个工作线程并发调用，必须线程安全；
 *          不要用 QThread 的中断位（在非 Qt 线程上恒为 false）。
 * @note 基准阶段被取消时直接返回 false 且 results 为空 —— 残缺的基准算出的
 *       提升率全是垃圾，不能拿去和候选相减。
 */
bool threadRunPairedComparison(
    const SimConfig& base,
    const std::vector<SimConfig>& candidates,
    std::uint32_t firstSeed,
    int seedCount,
    int maxTime,
    int deltaTime,
    int threadCount,
    const SimulateFn& simulate,
    std::vector<ComparisonResult>& results,
    const std::function<bool()>& shouldStop = {});

/**
 * @brief 打印对比结果表
 */
void printComparisonResults(const std::vector<ComparisonResult>& results, int pairs);
