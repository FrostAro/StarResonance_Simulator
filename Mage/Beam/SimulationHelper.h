#pragma once
#include "Person.h"
#include "Initializer.hpp"
#include "../../core/Comparison.h"

// ============================================================================
// 射线单次模拟的共享实现
// 供 GUI（mainwindow.cpp）与控制台对比模式（main_compare.cpp）共同使用，
// 避免同一份"构造角色→设种子→初始化→跑循环→收集统计"逻辑复制两份。
// ============================================================================
inline std::unordered_map<std::string, DamageStatistics> runBeamSimulationOnce(
    const SimConfig& cfg, std::uint32_t seed, int maxTime, int deltaTime)
{
    auto person = std::make_unique<Mage_Beam>(
        cfg.primaryAttributes, cfg.critical, cfg.quickness, cfg.lucky, cfg.proficient, cfg.almighty,
        cfg.atk, cfg.refineATK, cfg.elementATK,
        cfg.attackSpeed, cfg.castingSpeed,
        cfg.criticaldamage_set, cfg.increasedamage_set, cfg.elementdamage_set,
        maxTime, cfg.fantasyConfig);
    person->setRandomSeed(seed);

    auto init = std::make_unique<Initializer_Mage_Beam>(person.get(), deltaTime, cfg.fantasyConfig, cfg.flowConfig);
    init->Initialize();

    int currentTime = 0;
    while (currentTime < maxTime)
    {
        person->autoAttackPtr->update(deltaTime);
        currentTime += deltaTime;
        person->autoAttackPtr->setTimer() += deltaTime;
    }

    person->calculateDamageStatistics();
    return person->damageStatsMap;
}
