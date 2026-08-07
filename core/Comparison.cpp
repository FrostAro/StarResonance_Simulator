// ============================================================================
// 文件: Comparison.cpp
// 描述: 同种子配对对比（CRN）实现，见 Comparison.h
// ============================================================================

#include "Comparison.h"
#include <cmath>
#include <iomanip>
#include <iostream>

double getTotalDPS(const std::unordered_map<std::string, DamageStatistics>& statsMap, int maxTime)
{
    if (maxTime <= 0)
        return 0;  // 避免除零（GUI输入被清空等情况）
    double total = 0;
    for (const auto& [name, stat] : statsMap)
    {
        total += stat.damage + stat.luckyDamage;
    }
    return total / (static_cast<double>(maxTime) / 100.0);
}

// 平均值（空容器返回 0）
static double mean(const std::vector<double>& values)
{
    if (values.empty())
        return 0;
    double sum = 0;
    for (double v : values)
        sum += v;
    return sum / static_cast<double>(values.size());
}

// 样本标准差（n-1），n<2 返回 0
static double sampleStddev(const std::vector<double>& values, double avg)
{
    if (values.size() < 2)
        return 0;
    double sum = 0;
    for (double v : values)
    {
        double d = v - avg;
        sum += d * d;
    }
    return std::sqrt(sum / static_cast<double>(values.size() - 1));
}

std::vector<ComparisonResult> runPairedComparison(
    const SimConfig& base,
    const std::vector<SimConfig>& candidates,
    std::uint32_t firstSeed,
    int seedCount,
    int maxTime,
    int deltaTime,
    const SimulateFn& simulate,
    const std::function<bool()>& shouldStop)
{
    std::vector<ComparisonResult> results;
    if (seedCount <= 0 || !simulate)
        return results;

    // 1. 基准：在 seedCount 个种子下各跑一次并缓存（模拟是确定性的，可复用）
    std::vector<double> baseDpsBySeed(seedCount);
    for (int i = 0; i < seedCount; ++i)
    {
        if (shouldStop && shouldStop())
            return results;  // 提前终止：基准尚未完成，无候选结果
        const std::uint32_t seed = firstSeed + static_cast<std::uint32_t>(i);
        auto map = simulate(base, seed, maxTime, deltaTime);
        baseDpsBySeed[i] = getTotalDPS(map, maxTime);
    }
    const double baseAvg = mean(baseDpsBySeed);
    const double baseStd = sampleStddev(baseDpsBySeed, baseAvg);

    // 2. 候选：与基准同种子配对，逐对求差值
    results.reserve(candidates.size());
    for (const auto& cand : candidates)
    {
        std::vector<double> candDpsBySeed(seedCount);
        std::vector<double> deltas(seedCount);
        for (int i = 0; i < seedCount; ++i)
        {
            if (shouldStop && shouldStop())
                return results;  // 提前终止：返回已完成的候选
            const std::uint32_t seed = firstSeed + static_cast<std::uint32_t>(i);
            auto map = simulate(cand, seed, maxTime, deltaTime);
            candDpsBySeed[i] = getTotalDPS(map, maxTime);
            deltas[i] = candDpsBySeed[i] - baseDpsBySeed[i];
        }

        const double candAvg = mean(candDpsBySeed);
        const double candStd = sampleStddev(candDpsBySeed, candAvg);
        const double deltaAvg = mean(deltas);
        const double deltaStd = sampleStddev(deltas, deltaAvg);

        ComparisonResult r;
        r.label = cand.label;
        r.baseDps = baseAvg;
        r.candDps = candAvg;
        r.deltaDps = candAvg - baseAvg;
        r.deltaPercent = (baseAvg != 0) ? (r.deltaDps / baseAvg) * 100.0 : 0.0;
        r.baseStd = baseStd;
        r.candStd = candStd;
        r.deltaStd = deltaStd;
        r.pairs = seedCount;

        // 配对 t 值 = mean(delta) / (deltaStd / sqrt(N))；deltaStd=0 视为完全一致
        const double deltaSE = (seedCount > 0) ? deltaStd / std::sqrt(static_cast<double>(seedCount)) : 0.0;
        r.tValue = (deltaSE > 0) ? r.deltaDps / deltaSE : 0.0;

        results.push_back(std::move(r));
    }

    return results;
}

void printComparisonResults(const std::vector<ComparisonResult>& results, int pairs)
{
    std::cout << "====================================================\n";
    std::cout << " 配对对比模式（CRN 同种子）\n";
    std::cout << " 每对基准与候选使用相同随机种子，逐对相减抵消程序随机\n";
    std::cout << "====================================================\n";

    if (results.empty())
    {
        std::cout << "(无候选配置)\n";
        return;
    }

    // 基准信息
    const auto& first = results.front();
    std::cout << " 基准 DPS = " << std::fixed << std::setprecision(0) << first.baseDps
              << "（基准σ = " << first.baseStd << "，配对 N = " << pairs << "）\n\n";

    // 表头
    std::cout << std::fixed << std::setprecision(0)
              << std::left << std::setw(16) << "候选配置"
              << std::right << std::setw(12) << "基准DPS"
              << std::setw(12) << "候选DPS"
              << std::setw(11) << "ΔDPS"
              << std::setw(9) << "Δ%"
              << std::setw(10) << "配对σ"
              << std::setw(8) << "t值"
              << std::endl;

    // 数据行
    for (const auto& r : results)
    {
        std::cout << std::left << std::setw(16) << r.label
                  << std::right << std::setw(12) << r.baseDps
                  << std::setw(12) << r.candDps
                  << std::setw(11) << (r.deltaDps >= 0 ? "+" : "") << r.deltaDps
                  << std::setw(9) << (r.deltaPercent >= 0 ? "+" : "") << std::setprecision(2) << r.deltaPercent << std::setprecision(0)
                  << std::setw(10) << r.deltaStd
                  << std::setw(8) << std::setprecision(1) << r.tValue << std::setprecision(0)
                  << std::endl;
    }

    // 解读说明
    std::cout << "\n 解读：\n"
              << "  · 配对σ 远小于 基准σ/候选σ ⇒ CRN 生效，差值里只剩配置差异\n"
              << "  · |t值| ≥ 2 视为差异显著（N=" << pairs << " 时）\n"
              << "  · 候选σ/基准σ 需对比时，把第一列换成同名模拟多次求标准差即可\n";
}
