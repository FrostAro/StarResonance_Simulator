#include "AutoAttack.h"
#include "Skill.h"
#include "../../core/Person.h"
#include "../../core/Action.h"
#include "../../FightingFantasy/Skill.h"
#include "../../core/Logger.h"
#include <cassert>

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
    : AutoAttack(p), m_stages(stages)
    {
      this->registerOutBurstLogic();
    }

void AutoAttack_Mage_Beam_Base::update(int deltaTime) {
    // if (m_stages.size() == 6) {
    //     // LSZZ 六阶段轴：按实测 CD 就绪时点设置（1 tick = 1ms）。
    //     // Flood_Beam 基础 CD 为 42290ms（冷却缩减后约 38061ms），
    //     // 第 6 个 Flood 约 163870 启动，接近 164000 上限。
    //     if (timer > 0 && timer < 10000)             tryTriggerStage(1);
    //     if (timer > 30000 && timer < 40000)         tryTriggerStage(2);
    //     if (timer > 60000 && timer < 68000)         tryTriggerStage(3);
    //     if (timer > 89000 && timer < 96000)         tryTriggerStage(4);
    //     if (timer > 116000 && timer < 124000)       tryTriggerStage(5);
    //     if (timer > 160000 && timer < 180000)       tryTriggerStage(6);
    // } else {
    //     // 七阶段轴保留原时间窗口
    //     if (timer > 0 && timer < 5000)              tryTriggerStage(1);
    //     if (timer > 27000 && timer < 40000)         tryTriggerStage(2);
    //     if (timer > 54000 && timer < 65000)         tryTriggerStage(3);
    //     if (timer > 81000 && timer < 95000)         tryTriggerStage(4);
    //     if (timer > 108000 && timer < 128000)       tryTriggerStage(5);
    //     if (timer > 135000 && timer < 180000)       tryTriggerStage(6);
    //     if (timer > 163000 && timer < 175000)       tryTriggerStage(7);
    // }
  

    // windowPeriodLogic();
    this->checkSpecificLogicForOutBurst();

    // checkAndFinishOutBurst();
    this->createSkillByAuto();
    this->updatePerson(deltaTime);
    std::string currentSkillName = this->p->getNowReleasingSkill() ? this->p->getNowReleasingSkill()->getSkillName() : "None";
    if(currentSkillName != "None")
    {
      int index = this->p->findSkillInSkillCDList(currentSkillName);
      if(index != -1)
      {
        Skill* skill = this->p->getSkillCDListRef().at(index).get();
        if(skill->getStackRef() < skill->getMaxStack())
        {
          // 如果当前技能的堆叠数小于最大堆叠数，则触发CD刷新动作
          this->p->triggerAction<CDRefreshAction>(0, currentSkillName);
        }
      }
    }
}

void AutoAttack_Mage_Beam_Base::tryTriggerStage(int stageIdx) {
    if (nextOutBurstSignal != stageIdx || isOutBurst) return;
    if (stageIdx < 1 || stageIdx > (int)m_stages.size()) return;

    const auto& stage = m_stages[stageIdx - 1];
    if (checkSkills(stage.checkSkills)) {
        Logger::debugAutoAttack(timer, ("outBurst" + std::to_string(stageIdx) + " started").c_str());
        if (this->p->getNowReleasingSkill() &&
            this->p->getNowReleasingSkill()->getSkillName() == Beam::name) {
            (dynamic_cast<Beam*>(this->p->getNowReleasingSkill()))->stop();
        }
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

void AutoAttack_Mage_Beam_Base::checkSpecificLogicForOutBurst()
{
  // if(this->currentOutBurstType == OutBurstTypeEnum::None) return;
  // 如果已经在爆发期间
  if(isOutBurst)
  {
    this->timerForVariousStages -= deltaTime;
    // 如果上一次爆发时间截止
    if(this->timerForVariousStages <= 0)
    {
      this->isOutBurst = false;
      this->currentOutBurstType = OutBurstTypeEnum::None;
      Logger::debugAutoAttack(timer, "outBurst finished");
      // 如果当前正在释放的技能是 Beam，则停止它
      if (this->p->getNowReleasingSkill() && this->p->getNowReleasingSkill()->getSkillName() == Beam::name) 
      {
        (dynamic_cast<Beam*>(this->p->getNowReleasingSkill()))->stop();
      }
      // 如果当前正在释放的技能不是 Beam，则将其置为 nullptr
      else if(this->p->getNowReleasingSkill()) 
      {
        Logger::debugAutoAttack(timer, "outBurst twoFantasyAndUlti finished but nowReleasingSkill is not Beam");
        this->p->clearNowReleasingSkill();
      }
    }
  }
  else
  {
    // 如果不在爆发期间，则检查是否有新的爆发逻辑需要触发
    if(!m_outBurstQueue.empty())
    {
      auto nextOutBurstType = m_outBurstQueue.front();
      m_outBurstQueue.pop();
      if(m_outBurstLogicMap.find(nextOutBurstType) != m_outBurstLogicMap.end())
      {
        // 执行爆发逻辑
        m_outBurstLogicMap[nextOutBurstType].logic();
        this->timerForVariousStages = m_outBurstLogicMap[nextOutBurstType].lastingTime;
        this->isOutBurst = true;
        this->currentOutBurstType = nextOutBurstType;
      }
    }
  }
}

void AutoAttack_Mage_Beam_Base::registerOutBurstLogic()
{
  this->m_outBurstLogicMap[OutBurstTypeEnum::twoFantasyAndUlti] = {
    20000, 
    [this]() 
    { 
      const auto& stage = m_stages[m_logicToStageIndex.at(OutBurstTypeEnum::twoFantasyAndUlti)]; // 阶段1
      Logger::debugAutoAttack(timer, "outBurst twoFantasyAndUlti started");
      addSkillsToList(stage.addSkills);
      this->isOutBurst = true;
      this->timerForVariousStages = 20000; // 设置计时器为 20 秒
      this->currentOutBurstType = OutBurstTypeEnum::twoFantasyAndUlti;
    }
  };
  this->m_outBurstLogicMap[OutBurstTypeEnum::simpleFantasyOnly] = {
    20000, 
    [this]() 
    { 
      const auto& stage = m_stages[m_logicToStageIndex.at(OutBurstTypeEnum::simpleFantasyOnly)]; // 阶段2
      Logger::debugAutoAttack(timer, "outBurst simpleFantasyOnly started");
      addSkillsToList(stage.addSkills);
      this->isOutBurst = true;
      this->timerForVariousStages = 20000; // 设置计时器为 20 秒
      this->currentOutBurstType = OutBurstTypeEnum::simpleFantasyOnly;
    }
  };
  this->m_outBurstLogicMap[OutBurstTypeEnum::ultiOnly] = {
    20000, 
    [this]() 
    { 
      const auto& stage = m_stages[m_logicToStageIndex.at(OutBurstTypeEnum::ultiOnly)]; // 阶段2
      Logger::debugAutoAttack(timer, "outBurst ultiOnly started");
      addSkillsToList(stage.addSkills);
      this->isOutBurst = true;
      this->timerForVariousStages = 20000; // 设置计时器为 20 秒
      this->currentOutBurstType = OutBurstTypeEnum::ultiOnly;
    }
  };
  this->m_outBurstLogicMap[OutBurstTypeEnum::pureBurst] = 
  {
    20000, 
    [this]() 
    { 
      const auto& stage = m_stages[m_logicToStageIndex.at(OutBurstTypeEnum::pureBurst)]; // 阶段2
      Logger::debugAutoAttack(timer, "outBurst pureBurst started");
      addSkillsToList(stage.addSkills);
      this->isOutBurst = true;
      this->timerForVariousStages = 20000; // 设置计时器为 20 秒
      this->currentOutBurstType = OutBurstTypeEnum::pureBurst;
    }
  };
  this->m_outBurstLogicMap[OutBurstTypeEnum::window] = 
  {
    6000, 
    [this]() 
    { 
      const auto& stage = m_stages[m_logicToStageIndex.at(OutBurstTypeEnum::window)]; // 阶段2
      Logger::debugAutoAttack(timer, "outBurst window started");
      addSkillsToList(stage.addSkills);
      this->isOutBurst = true;
      this->timerForVariousStages = 6000; // 设置计时器为 6 秒
      this->currentOutBurstType = OutBurstTypeEnum::window;
    }
  };
}

void AutoAttack_Mage_Beam_Base::addOutBurstLogicToQueue(OutBurstTypeEnum type)
{
  this->m_outBurstQueue.push(type);
}

void AutoAttack_Mage_Beam_Base::windowPeriodLogic() {
    if (!nextIsWindow) return;

    if (!windowSkillTriggered) {
        // 六阶段轴：窗口期只打射线刷 Flood_Beam CD，
        // 保留 Vortex/FrostWind CD 供下一阶段爆发使用。
        maniAddPriorSkillList(FrostWind::name);
        maniAddPriorSkillList(Vortex::name);
        maniAddPriorSkillList(Beam::name);
        //maniAddPriorSkillList(WaterSpout::name);
        //maniAddPriorSkillList(WaterSpout::name);
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
        // { { {MukuChief::name, true}, {MukuScout::name, false}, {Vortex::name, false},
        //     {FrostWind::name, false}, {Flood_Beam::name, false} },
        //   {MukuChief::name, MukuScout::name, Vortex::name, FrostWind::name,
        //    Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
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
        // { { {MukuChief::name, true}, {YGLWS::name, true}, {Vortex::name, false},
        //     {FrostWind::name, false}, {Flood_Beam::name, false} },
        //   {MukuChief::name, YGLWS::name, Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
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
        // { { {MukuChief::name, true}, {SXMQ::name, false}, {Vortex::name, false},
        //     {FrostWind::name, false}, {Flood_Beam::name, false} },
        //   {MukuChief::name, SXMQ::name, Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
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
        // { { {SXMQ::name, false}, {HYXZ::name, false}, {Vortex::name, false},
        //     {FrostWind::name, false}, {Flood_Beam::name, false} },
        //   {SXMQ::name, HYXZ::name, Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
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
        // { { {SXMQ::name, false}, {MukuScout::name, false}, {Vortex::name, false},
        //     {FrostWind::name, false}, {Flood_Beam::name, false} },
        //   {SXMQ::name, MukuScout::name, Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
    }) {}

// LSZZ
AutoAttack_Mage_Beam_LSZZ::AutoAttack_Mage_Beam_LSZZ(Person* p)
    : AutoAttack_Mage_Beam_Base(p, {
        // 阶段1
        { { {SXMQ::name, false}, {LSZZ::name, false}, {Vortex::name, false},
            {FrostWind::name, false}, {Ultimate_Beam::name, false}, {Flood_Beam::name, false} },
          {SXMQ::name, LSZZ::name, Vortex::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段2
        { { {Vortex::name, false}, {FrostWind::name, false}, {Flood_Beam::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段3
        { { {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段4
        { { {SXMQ::name, false}, {LSZZ::name, false}, {Ultimate_Beam::name, false},
            {Vortex::name, false}, {FrostWind::name, false}, {Flood_Beam::name, false} },
          {SXMQ::name, LSZZ::name, Vortex::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段5
        { { {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段6
        { { {Ultimate_Beam::name, false}, {SXMQ::name, false}, {LSZZ::name, false}, {Vortex::name, false},
            {FrostWind::name, false}, {Flood_Beam::name, false} },
          {SXMQ::name, LSZZ::name, Ultimate_Beam::name, Vortex::name, FrostWind::name, 
           Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段7
        // { { {Vortex::name, false}, {FrostWind::name, false}, {Flood_Beam::name, false} },
        //   {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 以下为新逻辑专属
        // [6] twoFantasyAndUlti
        { {},
          {SXMQ::name, LSZZ::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name,Vortex::name, WaterSpout::name} },
        // [7] simpleFantasyOnly
        { {},
          {LSZZ::name, FrostWind::name,
           Flood_Beam::name, Beam::name,Vortex::name, WaterSpout::name} },
        // [8] ultiOnly
        { {},
          {FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name,Vortex::name, WaterSpout::name} },
        // [9] pureBurst
        { {},
          {FrostWind::name,
           Flood_Beam::name, Beam::name,Vortex::name, WaterSpout::name} },
        // [10] window
        { {},
          {FrostWind::name,
           Beam::name,Vortex::name, WaterSpout::name} },
     }) 
     {
      this->m_logicToStageIndex[OutBurstTypeEnum::twoFantasyAndUlti] = 6;
      this->m_logicToStageIndex[OutBurstTypeEnum::simpleFantasyOnly] = 7;
      this->m_logicToStageIndex[OutBurstTypeEnum::ultiOnly] = 8;
      this->m_logicToStageIndex[OutBurstTypeEnum::pureBurst] = 9;
      this->m_logicToStageIndex[OutBurstTypeEnum::window] = 10;

      this->addOutBurstLogicToQueue(OutBurstTypeEnum::twoFantasyAndUlti);
      this->addOutBurstLogicToQueue(OutBurstTypeEnum::window);
      this->addOutBurstLogicToQueue(OutBurstTypeEnum::pureBurst);
      this->addOutBurstLogicToQueue(OutBurstTypeEnum::window);
      this->addOutBurstLogicToQueue(OutBurstTypeEnum::pureBurst);
      this->addOutBurstLogicToQueue(OutBurstTypeEnum::window);
      this->addOutBurstLogicToQueue(OutBurstTypeEnum::twoFantasyAndUlti);
      this->addOutBurstLogicToQueue(OutBurstTypeEnum::window);
      this->addOutBurstLogicToQueue(OutBurstTypeEnum::pureBurst);
      this->addOutBurstLogicToQueue(OutBurstTypeEnum::window);
      this->addOutBurstLogicToQueue(OutBurstTypeEnum::pureBurst);
      this->addOutBurstLogicToQueue(OutBurstTypeEnum::window);
      this->addOutBurstLogicToQueue(OutBurstTypeEnum::twoFantasyAndUlti);
     }

     // 游子
AutoAttack_Mage_Beam_YZ::AutoAttack_Mage_Beam_YZ(Person* p)
    : AutoAttack_Mage_Beam_Base(p, {
        // 阶段1
        { { {SXMQ::name, false}, {YZ::name, false}, {Vortex::name, false},
            {FrostWind::name, false}, {Ultimate_Beam::name, false}, {Flood_Beam::name, false} },
          {SXMQ::name, YZ::name, Vortex::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段2
        { { {Vortex::name, false}, {FrostWind::name, false}, {Flood_Beam::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段3
        { { {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段4
        { { {SXMQ::name, false}, {YZ::name, false}, {Ultimate_Beam::name, false},
            {Vortex::name, false}, {FrostWind::name, false}, {Flood_Beam::name, false} },
          {SXMQ::name, YZ::name, Vortex::name, FrostWind::name,
           Ultimate_Beam::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段5
        { { {Flood_Beam::name, false}, {Vortex::name, false}, {FrostWind::name, false} },
          {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段6
        { { {Ultimate_Beam::name, false}, {SXMQ::name, false}, {YZ::name, false}, {Vortex::name, false},
            {FrostWind::name, false}, {Flood_Beam::name, false} },
          {SXMQ::name, YZ::name, Ultimate_Beam::name, Vortex::name, FrostWind::name, 
           Flood_Beam::name, Beam::name, WaterSpout::name} },
        // 阶段7
        // { { {Vortex::name, false}, {FrostWind::name, false}, {Flood_Beam::name, false} },
        //   {Vortex::name, FrostWind::name, Flood_Beam::name, Beam::name, WaterSpout::name} },
     }) {}

