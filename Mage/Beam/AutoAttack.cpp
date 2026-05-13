#include "AutoAttack.h"
#include "Skill.h"
#include "../../core/Person.h"
#include "../../FightingFantasy/Skill.h"
#include "../../core/Logger.h"
#include <cassert>

// 假设这些技能名称常量已在对应头文件中定义
// 实际项目中请包含相应的技能头文件
class MukuChief; class MukuScout; class YGLWS; class SXMQ; class HYXZ;
class Vortex; class FrostWind; class Ultimate_Beam; class Flood_Beam; class Beam; class WaterSpout;

// 辅助函数：检查单个技能是否就绪
static bool isSkillReady(Person* p, const std::string& skillName, bool allowStack) {
    int idx = p->findSkillInSkillCDList(skillName);
    if (idx == -1) return false;
    const auto& cd = p->getSkillCDListRef().at(idx);
    if (cd->getCurrentCD_Value() == 0) return true;
    if (allowStack && cd->getStackValue() > 0) return true;
    return false;
}

// 基类实现
AutoAttack_Mage_Beam_Base::AutoAttack_Mage_Beam_Base(Person* p, const std::vector<BurstStage>& stages)
    : AutoAttack(p), m_stages(stages) {}

void AutoAttack_Mage_Beam_Base::update(int deltaTime) {
    // 时间窗口完全复制原代码
    if (timer > 0 && timer < 500)          tryTriggerStage(1);
    if (timer > 2700 && timer < 4000)      tryTriggerStage(2);
    if (timer > 5400 && timer < 6500)      tryTriggerStage(3);
    if (timer > 8100 && timer < 9500)      tryTriggerStage(4);
    if (timer > 10800 && timer < 12800)    tryTriggerStage(5);
    if (timer > 13500 && timer < 14500)    tryTriggerStage(6);
    if (timer > 16300 && timer < 17500)    tryTriggerStage(7);

    windowPeriodLogic();
    checkAndFinishOutBurst();
    createSkillByAuto();
    updatePerson(deltaTime);
}

void AutoAttack_Mage_Beam_Base::tryTriggerStage(int stageIdx) {
    if (nextOutBurstSignal != stageIdx || isOutBurst) return;
    if (stageIdx < 1 || stageIdx > (int)m_stages.size()) return;

    const auto& stage = m_stages[stageIdx - 1];
    if (checkSkills(stage.checkSkills)) {
        Logger::debugAutoAttack(timer, ("outBurst" + std::to_string(stageIdx) + " started").c_str());
        addSkillsToList(stage.addSkills);
        nextOutBurstSignal = stageIdx + 1;
        isOutBurst = true;
    }
}

bool AutoAttack_Mage_Beam_Base::checkSkills(const std::vector<std::pair<std::string, bool>>& skills) const {
    for (const auto& [skillName, allowStack] : skills) {
        if (!isSkillReady(p, skillName, allowStack)) return false;
    }
    return true;
}

void AutoAttack_Mage_Beam_Base::addSkillsToList(const std::vector<std::string>& skills) {
    for (const auto& skill : skills) {
        maniAddPriorSkillList(skill);
    }
}

void AutoAttack_Mage_Beam_Base::windowPeriodLogic() {
    if (!nextIsWindow) return;

    if (!windowSkillTriggered) {
        maniAddPriorSkillList(Vortex::name);
        maniAddPriorSkillList(FrostWind::name);
        maniAddPriorSkillList(Beam::name);
        windowSkillTriggered = true;
    }

    if (p->getNowReleasingSkill() != nullptr &&
        p->getNowReleasingSkill()->getSkillName() == Beam::name &&
        !windowSecondBeamTriggered && windowSkillTriggered) {
        Beam* beam = static_cast<Beam*>(p->getNowReleasingSkill());
        assert(dynamic_cast<Beam*>(p->getNowReleasingSkill()) != nullptr &&
            "NowReleasingSkill type is not Beam!");
        if (beam->count > beam->noEnergyConsumeTime) {
            beam->stop();
            maniAddPriorSkillList(Beam::name);
            windowSecondBeamTriggered = true;
            Logger::debugAutoAttack(timer, "beam has been stopped compulsorily by autoAttack");
        }
    }

    if (windowSkillTriggered && windowSecondBeamTriggered && priorSkillList.empty()) {
        windowSkillTriggered = false;
        windowSecondBeamTriggered = false;
        nextIsWindow = false;
    }
}

void AutoAttack_Mage_Beam_Base::checkAndFinishOutBurst() {
    if (priorSkillList.empty() && isOutBurst && p->getNowReleasingSkill() == nullptr) {
        isOutBurst = false;
        nextIsWindow = true;
        Logger::debugAutoAttack(timer, "outBurst finished");
    }
}

// -------------------- 具体派生类的配置 --------------------

// MukuScout
AutoAttack_Mage_Beam_MukuScout::AutoAttack_Mage_Beam_MukuScout(Person* p)
    : AutoAttack_Mage_Beam_Base(p, {
        // 阶段1
        { { {MukuChief::name, true}, {MukuScout::name, false}, {Vortex::name, false},
            {FrostWind::name, false}, {Ultimate_Beam::name, false}, {Flood_Beam::name, false} },
          {MukuChief::name, MukuScout::name, Vortex::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段2
        { { {MukuChief::name, true}, {Vortex::name, false}, {FrostWind::name, false}, {Flood_Beam::name, false} },
          {MukuChief::name, Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段3
        { { {MukuChief::name, true}, {MukuScout::name, false}, {Vortex::name, false},
            {FrostWind::name, false}, {Ultimate_Beam::name, false}, {Flood_Beam::name, false} },
          {MukuChief::name, MukuScout::name, Vortex::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段4
        { { {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段5
        { { {MukuChief::name, true}, {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {MukuChief::name, Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段6
        { { {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段7
        { { {MukuChief::name, true}, {MukuScout::name, false}, {Vortex::name, false},
            {FrostWind::name, false}, {Flood_Beam::name, false} },
          {MukuChief::name, MukuScout::name, Vortex::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
    }) {}

// YGLWS
AutoAttack_Mage_Beam_YGLWS::AutoAttack_Mage_Beam_YGLWS(Person* p)
    : AutoAttack_Mage_Beam_Base(p, {
        // 阶段1
        { { {MukuChief::name, true}, {YGLWS::name, true}, {Vortex::name, false},
            {FrostWind::name, false}, {Ultimate_Beam::name, false}, {Flood_Beam::name, false} },
          {MukuChief::name, YGLWS::name, Vortex::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段2
        { { {MukuChief::name, true}, {YGLWS::name, true}, {Vortex::name, false},
            {FrostWind::name, false}, {Flood_Beam::name, false} },
          {MukuChief::name, YGLWS::name, Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段3
        { { {MukuChief::name, true}, {YGLWS::name, true}, {Vortex::name, false},
            {FrostWind::name, false}, {Ultimate_Beam::name, false}, {Flood_Beam::name, false} },
          {MukuChief::name, YGLWS::name, Vortex::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段4
        { { {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段5
        { { {MukuChief::name, true}, {YGLWS::name, true}, {Flood_Beam::name, false},
            {Vortex::name, false}, {FrostWind::name, false} },
          {MukuChief::name, YGLWS::name, Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段6
        { { {Ultimate_Beam::name, false}, {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段7
        { { {MukuChief::name, true}, {YGLWS::name, true}, {Vortex::name, false},
            {FrostWind::name, false}, {Flood_Beam::name, false} },
          {MukuChief::name, YGLWS::name, Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
    }) {}

// SXMQ
AutoAttack_Mage_Beam_SXMQ::AutoAttack_Mage_Beam_SXMQ(Person* p)
    : AutoAttack_Mage_Beam_Base(p, {
        // 阶段1
        { { {MukuChief::name, true}, {SXMQ::name, false}, {Vortex::name, false},
            {FrostWind::name, false}, {Ultimate_Beam::name, false}, {Flood_Beam::name, false} },
          {MukuChief::name, SXMQ::name, Vortex::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段2
        { { {MukuChief::name, true}, {Vortex::name, false}, {FrostWind::name, false}, {Flood_Beam::name, false} },
          {MukuChief::name, Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段3
        { { {MukuChief::name, true}, {SXMQ::name, false}, {Vortex::name, false},
            {FrostWind::name, false}, {Ultimate_Beam::name, false}, {Flood_Beam::name, false} },
          {MukuChief::name, SXMQ::name, Vortex::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段4
        { { {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段5
        { { {MukuChief::name, true}, {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {MukuChief::name, Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段6
        { { {Ultimate_Beam::name, false}, {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段7
        { { {MukuChief::name, true}, {SXMQ::name, false}, {Vortex::name, false},
            {FrostWind::name, false}, {Flood_Beam::name, false} },
          {MukuChief::name, SXMQ::name, Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
    }) {}

// HYXZ
AutoAttack_Mage_Beam_HYXZ::AutoAttack_Mage_Beam_HYXZ(Person* p)
    : AutoAttack_Mage_Beam_Base(p, {
        // 阶段1
        { { {SXMQ::name, false}, {HYXZ::name, false}, {Vortex::name, false},
            {FrostWind::name, false}, {Ultimate_Beam::name, false}, {Flood_Beam::name, false} },
          {SXMQ::name, HYXZ::name, Vortex::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段2
        { { {Vortex::name, false}, {FrostWind::name, false}, {Flood_Beam::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段3
        { { {SXMQ::name, false}, {HYXZ::name, false}, {Ultimate_Beam::name, false},
            {Vortex::name, false}, {FrostWind::name, false}, {Flood_Beam::name, false} },
          {SXMQ::name, HYXZ::name, Vortex::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段4
        { { {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段5
        { { {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段6
        { { {Ultimate_Beam::name, false}, {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段7
        { { {SXMQ::name, false}, {HYXZ::name, false}, {Vortex::name, false},
            {FrostWind::name, false}, {Flood_Beam::name, false} },
          {SXMQ::name, HYXZ::name, Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
    }) {}

// JBMQ
AutoAttack_Mage_Beam_JBMQ::AutoAttack_Mage_Beam_JBMQ(Person* p)
    : AutoAttack_Mage_Beam_Base(p, {
        // 阶段1
        { { {SXMQ::name, false}, {MukuScout::name, false}, {Vortex::name, false},
            {FrostWind::name, false}, {Ultimate_Beam::name, false}, {Flood_Beam::name, false} },
          {SXMQ::name, MukuScout::name, Vortex::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段2
        { { {Vortex::name, false}, {FrostWind::name, false}, {Flood_Beam::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段3
        { { {SXMQ::name, false}, {MukuScout::name, false}, {Ultimate_Beam::name, false},
            {Vortex::name, false}, {FrostWind::name, false}, {Flood_Beam::name, false} },
          {SXMQ::name, MukuScout::name, Vortex::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段4
        { { {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段5
        { { {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段6
        { { {Ultimate_Beam::name, false}, {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段7
        { { {SXMQ::name, false}, {MukuScout::name, false}, {Vortex::name, false},
            {FrostWind::name, false}, {Flood_Beam::name, false} },
          {SXMQ::name, MukuScout::name, Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
    }) {}