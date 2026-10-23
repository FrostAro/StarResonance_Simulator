// ============================================================================
// 文件: main_beam_parallel.cpp
// 描述: 射线 (Beam) 并行多次模拟入口
//
// 与 main_beam.cpp 的区别：
//   main_beam          串行跑 times 次，每次都 printDamageStatistics（刷屏）
//   本程序             把 times 次分摊到多线程并行跑，只输出平均 DPS 与波动
//
// 每次模拟互相独立（各自构造自己的 Person、各自的种子），所以可以直接并行。
// 与 main_beam 共用 runBeamSimulationOnce，数值口径完全一致。
//
// 用法:
//   dps_simulator_beam_parallel.exe [次数=8] [线程数=8] [种子=42] [1=随机种子]
//
//   固定种子时第 i 次使用 种子+i，因此「次数 4 线程 1」与「次数 4 线程 4」
//   的输出应当完全一致 —— 这是验证并行没算错的最直接办法。
// ============================================================================

#include "Mage/Beam/SimulationHelper.h"

#include "core/Logger.h"
#include "core/ParallelFor.hpp"
#include "core/Statistics.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr int kMaxTime = 180000;   // 180 秒（1 tick = 1ms）
constexpr int kDeltaTime = 10;     // 步长 10ms

/**
 * @brief 站街面板：与 main_beam.cpp / GUI 射线默认值 / 黄金基线共用同一套数值
 */
SimConfig makeDefaultConfig()
{
    SimConfig cfg;
    cfg.primaryAttributes = 9272;
    cfg.critical = 10.12;
    cfg.quickness = 38.18;
    cfg.lucky = 23.21;
    cfg.proficient = 74.8;
    cfg.almighty = 20.88;
    cfg.atk = 8151;
    cfg.refineATK = 1100;
    cfg.elementATK = 430;
    cfg.attackSpeed = 0.0;
    cfg.castingSpeed = 0.0;
    cfg.criticaldamage_set = 0.0;
    cfg.increasedamage_set = 0.0;
    cfg.elementdamage_set = 0.0;
    cfg.fantasyConfig = 7;   // 机器人 + 嗜血毛球
    cfg.flowConfig = 0;      // 0=急速精通流，1=幸运流
    cfg.weapon = WeaponConfig{SecondaryAttributeConfig::Proficient,
                              SecondaryAttributeConfig::Quickness,
                              0.06, 0.08};
    return cfg;
}

/// 单次模拟的总 DPS（总伤害 / 秒数）
double totalDps(const std::unordered_map<std::string, DamageStatistics>& stats, int maxTime)
{
    double total = 0.0;
    for (const auto& [skillName, stat] : stats)
        total += stat.damage + stat.luckyDamage;
    return total / (maxTime / 1000.0);
}

/// 样本标准差（n-1）；样本不足 2 个时返回 0
double sampleStddev(const std::vector<double>& v, double avg)
{
    if (v.size() < 2)
        return 0.0;
    double acc = 0.0;
    for (double x : v)
        acc += (x - avg) * (x - avg);
    return std::sqrt(acc / static_cast<double>(v.size() - 1));
}

} // namespace

/**
 * @brief 程序主入口
 *
 * @param argc 参数个数
 * @param argv [次数] [线程数] [种子] [是否随机种子]
 * @return 0 正常退出
 */
int main(int argc, char** argv)
{
    // 并行时不要把日志开到 DEBUG：多线程写同一个 Logger 会互相交错，而且很慢
    Logger::initialize(Logger::Level::WARNING);

    int times = 8;
    int workers = 8;
    std::uint32_t firstSeed = 42;
    bool randomSeed = false;   // 默认固定种子：结果可复现，便于和串行版逐字对照

    if (argc > 1)
        times = std::atoi(argv[1]);
    if (argc > 2)
        workers = std::atoi(argv[2]);
    if (argc > 3)
        firstSeed = static_cast<std::uint32_t>(std::atoi(argv[3]));
    if (argc > 4)
        randomSeed = std::atoi(argv[4]) != 0;

    if (times < 1)
        times = 1;
    if (workers < 1)
        workers = 1;
    if (workers > times)
        workers = times;   // 线程多于任务只是空转

    const SimConfig cfg = makeDefaultConfig();

    // 种子统一在主线程生成：std::random_device 不保证线程安全
    std::vector<std::uint32_t> seeds(static_cast<std::size_t>(times));
    if (randomSeed)
    {
        std::random_device rd;
        for (auto& s : seeds)
            s = rd();
    }
    else
    {
        // 固定种子时第 i 次用 firstSeed + i，保证串行/并行使用同一组种子
        for (int i = 0; i < times; ++i)
            seeds[static_cast<std::size_t>(i)] = firstSeed + static_cast<std::uint32_t>(i);
    }

    std::cout << "====================================================================\n";
    std::cout << " 射线并行模拟 | 次数: " << times << " | 线程数: " << workers
              << " | 种子: ";
    if (randomSeed)
        std::cout << "随机";
    else if (times == 1)
        std::cout << firstSeed;
    else
        std::cout << firstSeed << ".." << (firstSeed + times - 1);
    std::cout << " | 时长: " << kMaxTime << "ms | 步长: " << kDeltaTime << "ms\n";
    std::cout << "====================================================================\n";

    if (times == 1)
        std::cout << "提示: 次数为 1 时无法并行（单次模拟内部是 tick 串行的）。\n";

    // 每次模拟写自己的下标槽位，互不相交，无需加锁
    std::vector<std::unordered_map<std::string, DamageStatistics>> results(
        static_cast<std::size_t>(times));

    const auto begin = std::chrono::steady_clock::now();

    const bool finished = parallelFor(
        times, workers,
        [&](int index)
        {
            results[static_cast<std::size_t>(index)] =
                runBeamSimulationOnce(cfg, seeds[static_cast<std::size_t>(index)],
                                      kMaxTime, kDeltaTime);
        });

    const auto end = std::chrono::steady_clock::now();
    const double seconds =
        std::chrono::duration<double>(end - begin).count();

    int completed = 0;
    for (const auto& m : results)
        if (!m.empty())
            ++completed;

    if (completed < times)
        std::cout << "注意: 只完成了 " << completed << " / " << times << " 次模拟。\n";

    std::cout << "用时: " << std::fixed << std::setprecision(3) << seconds << " s（"
              << std::setprecision(1)
              << (seconds > 0 ? static_cast<double>(completed) / seconds : 0.0)
              << " 次/秒）\n\n";

    if (completed == 0)
        return 1;

    // 逐次总 DPS + 平均/波动
    std::vector<double> dpsList;
    dpsList.reserve(static_cast<std::size_t>(completed));
    for (const auto& m : results)
        if (!m.empty())
            dpsList.push_back(totalDps(m, kMaxTime));

    double sum = 0.0;
    for (double d : dpsList)
        sum += d;
    const double avg = sum / static_cast<double>(dpsList.size());
    const double sd = sampleStddev(dpsList, avg);

    std::cout << "每次总 DPS:";
    for (double d : dpsList)
        std::cout << " " << std::fixed << std::setprecision(0) << d;
    std::cout << "\n";

    std::cout << "平均总 DPS: " << std::setprecision(1) << avg
              << "（σ = " << std::setprecision(2) << sd;
    if (avg > 0.0)
        std::cout << "，相对 " << std::setprecision(3) << (sd / avg) * 100.0 << "%";
    std::cout << "）\n\n";

    // 表格沿用统计模块自己的格式，这里把流恢复成默认
    std::cout.unsetf(std::ios::fixed);
    std::cout << std::setprecision(6);

    // 各技能的平均值与占比。只有一次时直接打印明细（汇总函数在 <=1 次时直接返回）
    if (completed == 1)
    {
        for (const auto& m : results)
        {
            if (!m.empty())
            {
                printDamageStatistics(m, kMaxTime);
                break;
            }
        }
    }
    else
    {
        summaryCirculationPrint(results, kMaxTime);
    }

    return finished ? 0 : 1;
}
