// ============================================================================
// 文件: main_beam_max.cpp
// 描述: 射线各场景技能伤害峰值统计（独立 exe）
//
// 用法:
//   dps_simulator_beam_max
//
// 说明:
//   1. beam_person 集中保存面板数值与 fantasyConfig/flowConfig，便于修改。
//   2. 每个场景创建全新的 Mage_Beam，按一波爆发顺序释放技能。
//   3. 从 Beam 第一次造成伤害开始计时，窗口结束立即停止模拟。
//   4. 输出每个技能在窗口内的总伤害、DPS、1秒最高伤害与峰值秒。
// ============================================================================

#include "Mage/Beam/Person.h"
#include "Mage/Beam/Initializer.hpp"
#include "Mage/Beam/Skill.h"
#include "FightingFantasy/Skill.h"
#include "core/AutoAttack.h"
#include "core/Person.h"
#include "core/Logger.h"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================================
// beam_person：集中配置面板与模拟参数
// ============================================================================
class beam_person
{
public:
    // ---- 面板属性（按需修改） ----
    double primaryAttributes = 6760;
    double critical = 5;
    double quickness = 30;
    double lucky = 5;
    double proficient = 45;
    double almighty = 10;
    int atk = 5000;
    int refineATK = 1000;
    int elementATK = 230;
    double attackSpeed = 0;
    double castingSpeed = 0;
    double criticaldamage_set = 0;
    double increasedamage_set = 0;
    double elementdamage_set = 0;

    // ---- 配置项 ----
    int fantasyConfig = 5;  // 0..6，与 Initializer_Mage_Beam 对应
    int flowConfig = 0;     // 0=急速精通流，1=幸运流
    std::uint32_t seed = 42;

    std::unique_ptr<Mage_Beam> build() const
    {
        return std::make_unique<Mage_Beam>(
            primaryAttributes, critical, quickness, lucky, proficient, almighty,
            atk, refineATK, elementATK,
            attackSpeed, castingSpeed,
            criticaldamage_set, increasedamage_set, elementdamage_set,
            /*totalTime*/ 200000,
            fantasyConfig);
    }
};

// ============================================================================
// 脚本化 AutoAttack：只负责按顺序释放一波技能
// ============================================================================
class AutoAttack_Mage_Beam_Scripted : public AutoAttack
{
public:
    AutoAttack_Mage_Beam_Scripted(Person* p, std::vector<std::string> wave)
        : AutoAttack(p), m_wave(std::move(wave)) {}

    void update(int deltaTime) override
    {
        if (!m_started)
        {
            for (const auto& skillName : m_wave)
            {
                maniAddPriorSkillList(skillName);
            }
            m_started = true;
        }

        createSkillByAuto();
        updatePerson(deltaTime);
    }

private:
    std::vector<std::string> m_wave;
    bool m_started = false;
};

// ============================================================================
// 场景统计结果
// ============================================================================
struct SkillStat
{
    double totalDamage = 0;   // 窗口内总伤害（含幸运期望）
    double dps = 0;           // 总伤害 / 窗口秒数
    double peakDamage = 0;    // 1秒最高伤害
    int peakSecond = -1;      // 峰值出现在第几秒（从 0 开始）
};

struct ScenarioResult
{
    std::string title;
    int durationTicks = 0;
    std::unordered_map<std::string, SkillStat> stats;
    double totalAllDamage = 0;
};

// ============================================================================
// 幻想配置 → 技能名列表（与 Initializer_Mage_Beam 的 equipSkills 保持一致）
// ============================================================================
static std::vector<std::string> fantasySkillsFor(int fantasyConfig)
{
    switch (fantasyConfig)
    {
    case 0: return { MukuChief::name, MukuScout::name };
    case 1: return { MukuChief::name, YGLWS::name };
    case 2: return { MukuChief::name, SXMQ::name };
    case 3: return { HYXZ::name, SXMQ::name };
    case 4: return { MukuScout::name, SXMQ::name };
    case 5: return { LSZZ::name, SXMQ::name };
    case 6: return { YZ::name, SXMQ::name };
    default: return { MukuChief::name, MukuScout::name };
    }
}

// ============================================================================
// 运行一个场景
// ============================================================================
static ScenarioResult runScenario(const beam_person& cfg,
                                  const std::string& title,
                                  const std::vector<std::string>& wave,
                                  int durationTicks,
                                  const std::string& singleFantasySkill = "")
{
    const int deltaTime = 10;  // 1 tick = 1ms

    ScenarioResult result;
    result.title = title;
    result.durationTicks = durationTicks;

    auto person = cfg.build();
    person->setRandomSeed(cfg.seed);

    // 用脚本化 AutoAttack 替换默认自动战斗
    person->autoAttackPtr = std::make_unique<AutoAttack_Mage_Beam_Scripted>(person.get(), wave);

    auto initializer = std::make_unique<Initializer_Mage_Beam>(
        person.get(), deltaTime, cfg.fantasyConfig, cfg.flowConfig, singleFantasySkill);
    initializer->Initialize();

    // 从 0 开始计时
    AutoAttack::setTimer() = 0;

    int currentTime = 0;
    int startedTime = -1;
    const int maxGuard = 300000;  // 安全上限，防止异常情况下死循环

    while (currentTime < maxGuard)
    {
        person->autoAttackPtr->update(deltaTime);
        currentTime += deltaTime;
        AutoAttack::setTimer() += deltaTime;

        // 找到 Beam 第一次出伤的时间
        if (startedTime < 0)
        {
            for (const auto& info : person->getDamageListInfoRef())
            {
                if (info.skillName == Beam::name)
                {
                    startedTime = static_cast<int>(info.time);
                    break;
                }
            }
        }

        // 窗口结束立即停止
        if (startedTime >= 0 && currentTime >= startedTime + durationTicks)
            break;
    }

    if (startedTime < 0)
    {
        std::cout << "[警告] 场景「" << title << "」未检测到 Beam 伤害，跳过统计。\n";
        return result;
    }

    const int seconds = durationTicks / 1000;
    std::unordered_map<std::string, std::vector<double>> perSecond;

    for (const auto& info : person->getDamageListInfoRef())
    {
        if (info.time < startedTime || info.time >= startedTime + durationTicks)
            continue;

        const int sec = static_cast<int>((info.time - startedTime) / 1000);
        if (sec < 0 || sec >= seconds)
            continue;

        const double damage = info.damageNum + info.luckyNum;
        auto& buckets = perSecond[info.skillName];
        if (static_cast<int>(buckets.size()) < seconds)
            buckets.resize(seconds, 0.0);
        buckets[sec] += damage;
    }

    result.totalAllDamage = 0.0;
    for (const auto& entry : perSecond)
    {
        SkillStat stat;
        stat.totalDamage = 0.0;
        stat.peakDamage = 0.0;
        stat.peakSecond = -1;

        const auto& buckets = entry.second;
        for (int i = 0; i < static_cast<int>(buckets.size()); ++i)
        {
            stat.totalDamage += buckets[i];
            if (buckets[i] > stat.peakDamage)
            {
                stat.peakDamage = buckets[i];
                stat.peakSecond = i;
            }
        }

        stat.dps = (seconds > 0) ? stat.totalDamage / static_cast<double>(seconds) : 0.0;
        result.totalAllDamage += stat.totalDamage;
        result.stats[entry.first] = stat;
    }

    return result;
}

// ============================================================================
// 只注册技能、不装备任何 Buff/被动的初始化器（用于裸技能首次伤害）
// ============================================================================
class SkillOnlyInitializer_Mage_Beam : public Initializer
{
public:
    SkillOnlyInitializer_Mage_Beam(Person* p, double deltaTime)
        : Initializer(p, deltaTime, 0) {}

    // 只把技能注册到 SkillCreator，不装备技能、不创建任何 Buff
    void SetupSkillsOnly()
    {
        registerSkills();
    }

protected:
    void equipSkills() override {}

    void registerSkills() override
    {
        registerCertainSkill<IceArrow_Beam>();
        registerCertainSkill<Flood_Beam>();
        registerCertainSkill<Ultimate_Beam>();
        registerCertainSkill<Beam>();
        registerCertainSkill<Vortex>();
        registerCertainSkill<FrostWind>();
        registerCertainSkill<WaterSpout>();
        registerCertainSkill<CrystalsHail>();
        registerCertainSkill<FrostDecreePulse>();
        registerCertainSkill<FrostBurst>();
        registerCertainSkill<MukuChief>();
        registerCertainSkill<MukuScout>();
        registerCertainSkill<YGLWS>();
        registerCertainSkill<SXMQ>();
        registerCertainSkill<HYXZ>();
        registerCertainSkill<LSZZ>();
        registerCertainSkill<YZ>();
    }

    void registerBuffs() override {}
};

// ============================================================================
// 单独技能首次伤害（用于与游戏内数据对齐）
// ============================================================================
struct FirstHitResult
{
    std::string skillName;
    bool found = false;
    DamageInfo info;
};

static FirstHitResult runFirstHit(const beam_person& cfg, const std::string& skillName)
{
    const int deltaTime = 10;  // 1 tick = 1ms

    FirstHitResult result;
    result.skillName = skillName;

    auto person = cfg.build();
    person->setRandomSeed(cfg.seed);

    std::vector<std::string> wave = { skillName };
    person->autoAttackPtr = std::make_unique<AutoAttack_Mage_Beam_Scripted>(person.get(), wave);

    // 裸技能测试：只注册技能，不装备任何 Buff/被动
    auto initializer = std::make_unique<SkillOnlyInitializer_Mage_Beam>(person.get(), deltaTime);
    initializer->SetupSkillsOnly();

    AutoAttack::setTimer() = 0;

    int currentTime = 0;
    const int maxGuard = 30000;  // 单个技能首次出伤的安全上限

    while (currentTime < maxGuard)
    {
        person->autoAttackPtr->update(deltaTime);
        currentTime += deltaTime;
        AutoAttack::setTimer() += deltaTime;

        for (const auto& info : person->getDamageListInfoRef())
        {
            if (info.skillName == skillName)
            {
                result.found = true;
                result.info = info;
                return result;
            }
        }
    }

    return result;
}

// ============================================================================
// 终端对齐辅助：中文按 2 列宽计算
// ============================================================================
static int utf8CodePointWidth(std::uint32_t cp)
{
    if (cp >= 0x1100 &&
        (cp <= 0x115F ||
         cp == 0x2329 || cp == 0x232A ||
         (cp >= 0x2E80 && cp <= 0xA4CF && cp != 0x303F) ||
         (cp >= 0xAC00 && cp <= 0xD7A3) ||
         (cp >= 0xF900 && cp <= 0xFAFF) ||
         (cp >= 0xFE30 && cp <= 0xFE6F) ||
         (cp >= 0xFF00 && cp <= 0xFF60) ||
         (cp >= 0xFFE0 && cp <= 0xFFE6)))
    {
        return 2;
    }
    return 1;
}

static int displayWidth(const std::string& text)
{
    int width = 0;
    for (size_t i = 0; i < text.size();)
    {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        std::uint32_t cp = 0;
        int len = 1;

        if (c < 0x80)
        {
            cp = c;
            len = 1;
        }
        else if ((c >> 5) == 0x06)
        {
            cp = c & 0x1F;
            len = 2;
        }
        else if ((c >> 4) == 0x0E)
        {
            cp = c & 0x0F;
            len = 3;
        }
        else if ((c >> 3) == 0x1E)
        {
            cp = c & 0x07;
            len = 4;
        }
        else
        {
            ++i;
            continue;
        }

        for (int k = 1; k < len && i + k < text.size(); ++k)
        {
            cp = (cp << 6) | (static_cast<unsigned char>(text[i + k]) & 0x3F);
        }
        i += len;
        width += utf8CodePointWidth(cp);
    }
    return width;
}

static std::string padRight(const std::string& text, int width)
{
    const int len = displayWidth(text);
    return (len < width) ? text + std::string(width - len, ' ') : text;
}

static std::string padLeft(const std::string& text, int width)
{
    const int len = displayWidth(text);
    return (len < width) ? std::string(width - len, ' ') + text : text;
}

static std::string formatFixed(double value, int precision)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

// ============================================================================
// 打印结果
// ============================================================================
static void printResult(const ScenarioResult& result)
{
    std::cout << "\n============================================================\n";
    std::cout << "场景: " << result.title << "\n";
    std::cout << "统计窗口: " << (result.durationTicks / 1000) << "s（从 Beam 首次出伤开始）\n";
    std::cout << "============================================================\n";

    if (result.stats.empty())
    {
        std::cout << "(无有效伤害数据)\n";
        return;
    }

    // 按总伤害从高到低排序
    std::vector<std::pair<std::string, SkillStat>> rows(result.stats.begin(), result.stats.end());
    std::sort(rows.begin(), rows.end(),
              [](const auto& a, const auto& b) { return a.second.totalDamage > b.second.totalDamage; });

    std::cout << padRight("技能", 18)
              << padLeft("总伤害", 14)
              << padLeft("DPS", 14)
              << padLeft("1秒最高伤害", 14)
              << padLeft("峰值秒", 10)
              << padLeft("占比", 10)
              << "\n";
    std::cout << std::string(80, '-') << "\n";

    for (const auto& row : rows)
    {
        const auto& stat = row.second;
        const double percent = (result.totalAllDamage > 0.0)
                                   ? (stat.totalDamage / result.totalAllDamage) * 100.0
                                   : 0.0;
        std::cout << padRight(row.first, 18)
                  << padLeft(formatFixed(stat.totalDamage, 0), 14)
                  << padLeft(formatFixed(stat.dps, 0), 14)
                  << padLeft(formatFixed(stat.peakDamage, 0), 14)
                  << padLeft(std::to_string(stat.peakSecond), 10)
                  << padLeft(formatFixed(percent, 2) + "%", 10)
                  << "\n";
    }

    std::cout << std::string(80, '-') << "\n";
    std::cout << "总伤害: " << formatFixed(result.totalAllDamage, 0) << "\n";
}

// ============================================================================
// 打印单独技能首次伤害
// ============================================================================
static void printFirstHits(const std::vector<FirstHitResult>& results)
{
    std::cout << "\n============================================================\n";
    std::cout << "单独技能首次伤害（用于与游戏内数据对齐）\n";
    std::cout << "裸技能：不装备任何 Buff/被动，仅保留角色基础属性与技能自身系数\n";
    std::cout << "============================================================\n";
    std::cout << padRight("技能", 18)
              << padLeft("首次伤害", 14)
              << padLeft("首次幸运期望", 16)
              << padLeft("首次合计", 14)
              << "\n";
    std::cout << std::string(64, '-') << "\n";

    for (const auto& result : results)
    {
        if (!result.found)
            continue;

        const double damage = result.info.damageNum;
        const double lucky = result.info.luckyNum;
        std::cout << padRight(result.skillName, 18)
                  << padLeft(formatFixed(damage, 2), 14)
                  << padLeft(formatFixed(lucky, 2), 16)
                  << padLeft(formatFixed(damage + lucky, 2), 14)
                  << "\n";
    }
}

// ============================================================================
// main
// ============================================================================
int main()
{
    // 压低日志输出，避免刷屏；只保留错误级别
    Logger::initialize(Logger::Level::ERROR);

    beam_person cfg;  // ← 需要改面板时，直接改这里的字段

    const std::vector<std::string> fantasySkills = fantasySkillsFor(cfg.fantasyConfig);
    if (fantasySkills.empty())
    {
        std::cout << "fantasyConfig 非法或没有幻想技能。\n";
        return 1;
    }

    const int DUR_15S = 15000;  // 15s（1 tick = 1ms）
    const int DUR_10S = 10000;  // 10s（1 tick = 1ms）

    // ---- 公共技能 ----
    const std::string VORTEX = Vortex::name;
    const std::string FROSTWIND = FrostWind::name;
    const std::string FLOOD = Flood_Beam::name;
    const std::string ULTIMATE = Ultimate_Beam::name;
    const std::string BEAM = Beam::name;
    const std::string WATERSPOUT = WaterSpout::name;

    // ---- 场景 1：单灌注 15s ----
    {
        std::vector<std::string> wave = { VORTEX, FROSTWIND, FLOOD, BEAM, WATERSPOUT };
        printResult(runScenario(cfg, "单灌注15s", wave, DUR_15S));
    }

    // ---- 场景 2：大招灌注 15s ----
    {
        std::vector<std::string> wave = { VORTEX, FROSTWIND, ULTIMATE, FLOOD, BEAM, WATERSPOUT };
        printResult(runScenario(cfg, "大招灌注15s", wave, DUR_15S));
    }

    // ---- 场景 3：单幻想灌注 15s（每个幻想各跑一遍） ----
    for (const auto& fantasy : fantasySkills)
    {
        std::vector<std::string> wave = { fantasy, VORTEX, FROSTWIND, FLOOD, BEAM, WATERSPOUT };
        printResult(runScenario(cfg, "单幻想灌注15s（幻想：" + fantasy + "）", wave, DUR_15S, fantasy));
    }

    // ---- 场景 4：双幻想灌注 15s ----
    if (fantasySkills.size() >= 2)
    {
        std::vector<std::string> wave = { fantasySkills[0], fantasySkills[1],
                                          VORTEX, FROSTWIND, FLOOD, BEAM, WATERSPOUT };
        printResult(runScenario(cfg, "双幻想灌注15s", wave, DUR_15S));
    }

    // ---- 场景 5：双幻想大招灌注 15s ----
    if (fantasySkills.size() >= 2)
    {
        std::vector<std::string> wave = { fantasySkills[0], fantasySkills[1],
                                          VORTEX, FROSTWIND, ULTIMATE, FLOOD, BEAM, WATERSPOUT };
        printResult(runScenario(cfg, "双幻想大招灌注15s", wave, DUR_15S));
    }

    // ---- 场景 6：空窗期 10s（FrostWind → Vortex → Beam，不放 WaterSpout） ----
    {
        std::vector<std::string> wave = { FROSTWIND, VORTEX, BEAM };
        printResult(runScenario(cfg, "空窗期10s", wave, DUR_10S));
    }

    // ---- 单独技能首次伤害（用于与游戏内数据对齐） ----
    {
        const std::vector<std::string> firstHitSkills = {
            Beam::name,
            IceArrow_Beam::name,
            FrostBurst::name,
            CrystalsHail::name,
            FrostDecreePulse::name,
            Ultimate_Beam::name,
            Vortex::name,
            FrostWind::name,
            WaterSpout::name,
            MukuChief::name,
            MukuScout::name,
            YGLWS::name,
            SXMQ::name,
            HYXZ::name,
            LSZZ::name,
            YZ::name
        };

        std::vector<FirstHitResult> firstHits;
        firstHits.reserve(firstHitSkills.size());
        for (const auto& skillName : firstHitSkills)
        {
            firstHits.push_back(runFirstHit(cfg, skillName));
        }
        printFirstHits(firstHits);
    }

    return 0;
}
