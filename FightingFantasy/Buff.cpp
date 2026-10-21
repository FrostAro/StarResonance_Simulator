#include "Buff.h"
#include "../core/GameConstants.h"
#include "../core/Action.h"
#include "../core/Person.h"
#include "../core/Skill.h"
#include "../core/AutoAttack.h"
#include "../core/Logger.h"
#include "../core/creators.hpp"

// 姆克头目
std::string MukuChiefBuff::name = "MukuChiefBuff";

MukuChiefBuff::MukuChiefBuff(Person *p, double) : Buff(p)
{

    this->duration = 20000;
    this->maxDuration = this->duration;

    Logger::debugBuff(p->getSimulationTime(),
                        this->getBuffName(),
                        " - before person Crit Count + 4480, percent: " +
                        std::to_string(p->Critical));

    //p->changeCriticalCount(11200);
    this->p->triggerAction<CriticalCountModifyAction>(44800);

    Logger::debugBuff(p->getSimulationTime(),
                        this->getBuffName(),
                        " - after person Crit Count + 4480, percent: " +
                        std::to_string(p->Critical));

    this->p->triggerAction<CriticalDamageModifyAction>(0.4);
}

void MukuChiefBuff::listenerCallback(double) {}
void MukuChiefBuff::update(const double) {}
bool MukuChiefBuff::shouldBeRemoved() { return this->duration < 0; }
std::string MukuChiefBuff::getBuffName() const { return MukuChiefBuff::name; }

MukuChiefBuff::~MukuChiefBuff()
{
    //p->changeCriticalCount(-11200);
    this->p->triggerAction<CriticalCountModifyAction>(-44800);
    this->p->triggerAction<CriticalDamageModifyAction>(-0.4);
}

// 姆克尖兵
std::string MukuScoutBuff::name = "MukuScoutBuff";

MukuScoutBuff::MukuScoutBuff(Person *p, double) : Buff(p)
{
    this->number = 0.15; // 用作增攻数值
    this->duration = 20000;
    this->maxDuration = this->duration;

    //p->changeAattackIncrease(this->number);
    this->p->triggerAction<AttackIncreaseModifyAction>(this->number);
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "attackIncrease applied");
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "attackIncrease applied");
}

void MukuScoutBuff::listenerCallback(const DamageInfo &) {}
void MukuScoutBuff::update(const double) {}
bool MukuScoutBuff::shouldBeRemoved() { return this->duration < 0; }
std::string MukuScoutBuff::getBuffName() const { return MukuScoutBuff::name; }

MukuScoutBuff::~MukuScoutBuff()
{
    //this->p->changeAattackIncrease(-this->number);
    this->p->triggerAction<AttackIncreaseModifyAction>(-this->number);
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "attackIncrease removed");
}

// 博伊斯
std::string BYSBuff::name = "BYSBuff";

BYSBuff::BYSBuff(Person *p, double) : Buff(p)
{
    this->number = 0; // 用作增攻数值
    this->duration = 20000;
    this->maxDuration = this->duration;
    this->p->triggerAction<AttackIncreaseModifyAction>(0.26);
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "attackIncrease applied");
    this->p->triggerAction<AttackSpeedPercentModifyAction>(0.1);
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "attackSpeed applied");
}

void BYSBuff::listenerCallback(const DamageInfo &) {}

void BYSBuff::update(const double) {}
bool BYSBuff::shouldBeRemoved() { return this->duration < 0; }
std::string BYSBuff::getBuffName() const { return BYSBuff::name; }

BYSBuff::~BYSBuff()
{
    this->p->triggerAction<AttackIncreaseModifyAction>(-0.26);
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "attackIncrease removed");
    this->p->triggerAction<AttackSpeedPercentModifyAction>(-0.1);
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "attackSpeed removed");
}

// 伊戈雷乌斯
std::string YGLWSBuff::name = "YGLWSBuff";

YGLWSBuff::YGLWSBuff(Person *p, double) : Buff(p)
{
    this->number = 0; // 用作增攻数值
    this->duration = 20000;
    this->maxDuration = this->duration;
    this->p->triggerAction<CriticalCountModifyAction>(56000);
    this->p->triggerAction<CriticalPercentModifyAction>(0.14);
}

void YGLWSBuff::listenerCallback(const DamageInfo &) {}
void YGLWSBuff::update(const double)
{
    // 暴击上限：超过60%的部分按30%折算为爆伤修正，暴击回落到60%以下时还原
    const double currentCritical = this->p->getCritical();
    const double targetReduction = (currentCritical > 0.6) ? (currentCritical - 0.6) * 0.3 : 0.0;
    if (targetReduction != this->lastReduction)
    {
        if (this->lastReduction != 0.0)
        {
            this->p->triggerAction<CriticalDamageModifyAction>(-this->lastReduction);
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "critical damage correction removed");
        }
        if (targetReduction != 0.0)
        {
            this->p->triggerAction<CriticalDamageModifyAction>(targetReduction);
        }
        this->lastReduction = targetReduction;
        Logger::debugBuff(p->getSimulationTime(), this->getBuffName(),
                          "critical damage adjusted, reduction: " + std::to_string(targetReduction));
    }
}
bool YGLWSBuff::shouldBeRemoved() { return this->duration < 0; }
std::string YGLWSBuff::getBuffName() const { return YGLWSBuff::name; }

YGLWSBuff::~YGLWSBuff()
{
    this->p->triggerAction<CriticalCountModifyAction>(-56000);
    this->p->triggerAction<CriticalPercentModifyAction>(-0.14);
    this->p->triggerAction<CriticalDamageModifyAction>(-this->lastReduction);
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "critical damage correction removed");
}

// 嗜血毛球
std::string SXMQBuff::name = "SXMQBuff";

SXMQBuff::SXMQBuff(Person *p, double) : Buff(p)
{
    this->number = 0.78; // 用作主动倍率
    this->duration = 20000;
    this->maxDuration = this->duration;

    auto info = std::make_unique<CreateSkillListener>(
        this->getBuffID(), [this](Skill *const skill)
        { this->listenerCallback(skill); });
    CreateSkillAction::addListener(std::move(info));
}

void SXMQBuff::listenerCallback(Skill *const skill) 
{
    bool a = false;
    for(auto i : skill->getSkillType())
    {
        if(i == Skill::skillTypeEnum::PARTICULAR || i == Skill::skillTypeEnum::SPECIALIZED)
        {
            a = true;
        }
    }
    if(a)
    {
        skill->fixedValue += this->number * this->p->getATK() * (1 + this->p->getAttackIncrease());
        Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "skill fixedValue increased");
    }
    if(skill->getCanTriggerLucky())
    {
        skill->luckyFixedValue += this->number * this->p->getATK() * (1 + this->p->getAttackIncrease());
        Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "skill luckyFixedValue increased");
    }
}

void SXMQBuff::update(const double) {}
bool SXMQBuff::shouldBeRemoved() { return this->duration < 0; }
std::string SXMQBuff::getBuffName() const { return SXMQBuff::name; }

SXMQBuff::~SXMQBuff() 
{
    CreateSkillAction::deleteListener(this->getBuffID());
}

// 嗜血毛球(被动)
std::string SXMQBuff_Passive::name = "SXMQBuff_Passive";

SXMQBuff_Passive::SXMQBuff_Passive(Person *p, double) : Buff(p)
{
    this->number = 10; // 用作触发层数
    this->duration = kPermanentBuffDurationLarge;
    this->maxDuration = this->duration;

    this->tempPerson = std::make_unique<temp_Person>();

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &info)
        { this->listenerCallback(info); });
    AttackAction::addListener(std::move(info));
}

void SXMQBuff_Passive::listenerCallback(const DamageInfo &info) 
{
    // 防重入标志（成员变量，初始为 false）
    if (inCallback) return;
    inCallback = true;

    auto skill = SkillCreator::createSkill(info.skillName,this->tempPerson.get());
    bool a = false;
    for(const auto& i : skill->getSkillType())
    {
        if(i == Skill::skillTypeEnum::PARTICULAR || i == Skill::skillTypeEnum::SPECIALIZED)
        {
            a = true;
        }
    }
    if(a)
    {
        this->stack++;
        if(this->stack >= this->number)
        {
            auto willAttackSkill = std::make_unique<temp_InstantSkill>("SXMQ_Passive",4.00,0);
            this->p->triggerAction<AttackAction>(0,willAttackSkill.get());
            Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "SXMQ passive attack triggered");
            this->stack = 0;
        }
    }

    inCallback = false;
}

void SXMQBuff_Passive::update(const double) {}
bool SXMQBuff_Passive::shouldBeRemoved() { return this->duration < 0; }
std::string SXMQBuff_Passive::getBuffName() const { return SXMQBuff_Passive::name; }
SXMQBuff_Passive::~SXMQBuff_Passive() 
{
    AttackAction::deleteListener(this->getBuffID());
}


// 幻妖蟹蛛
std::string HYXZBuff::name = "HYXZBuff";

HYXZBuff::HYXZBuff(Person *p, double) : Buff(p)
{
    this->number = 0.6; // 用作主动倍率
    this->duration = 20000;
    this->maxDuration = this->duration;

    p->triggerAction<ProficientPercentModifyAction>(0.2);
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "proficientPercent applied");
}

void HYXZBuff::listenerCallback(Skill *const skill) {}

void HYXZBuff::update(const double) {}
bool HYXZBuff::shouldBeRemoved() { return this->duration < 0; }
std::string HYXZBuff::getBuffName() const { return HYXZBuff::name; }
HYXZBuff::~HYXZBuff() 
{
    this->p->triggerAction<ProficientPercentModifyAction>(-0.2);
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "proficientPercent removed");
}

// 掠食蜘蛛
std::string LSZZBuff::name = "LSZZBuff";

LSZZBuff::LSZZBuff(Person *p, double) : Buff(p)
{
    this->number = 0.35; // 用作主动倍率
    this->duration = 20000;
    this->maxDuration = this->duration;

    p->triggerAction<AttackIncreaseModifyAction>(this->number);
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "attackIncrease applied");
}

void LSZZBuff::update(const double) {}
bool LSZZBuff::shouldBeRemoved() { return this->duration < 0; }
std::string LSZZBuff::getBuffName() const { return LSZZBuff::name; }

LSZZBuff::~LSZZBuff() 
{
    this->p->triggerAction<AttackIncreaseModifyAction>(-this->number);
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "attackIncrease removed");
}

// 游子
std::string YZBuff::name = "YZBuff";

YZBuff::YZBuff(Person *p, double) : Buff(p)
{
    this->duration = 20000;
    this->maxDuration = this->duration;

    p->triggerAction<LuckyPercentModifyAction>(0.2);
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "luckyPercent applied");
}

void YZBuff::listenerCallback(Skill *const skill) {}

void YZBuff::update(const double) {}
bool YZBuff::shouldBeRemoved() { return this->duration < 0; }
std::string YZBuff::getBuffName() const { return YZBuff::name; }
YZBuff::~YZBuff() 
{
    this->p->triggerAction<LuckyPercentModifyAction>(-0.2);
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "luckyPercent removed");
}

// 机器人
std::string JQRBuff::name = "JQRBuff";

JQRBuff::JQRBuff(Person *p, double) : Buff(p)
{
    this->duration = 20000;
    this->maxDuration = this->duration;
    this->number = 1.2;

    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "now proficient is " + std::to_string(this->p->getProficient()));
    double proficientIncrease = this->p->getProficient() / 5 * this->number;
    Logger::debugBuff(p->getSimulationTime(), this->getBuffName(), "proficientIncrease is " + std::to_string(proficientIncrease));
    double refineATKIncrease = this->p->getTotalATK() * proficientIncrease;
    this->tempValue = refineATKIncrease;
    this->p->triggerAction<RefineATKCountModifyAction>(refineATKIncrease);

    auto info = std::make_unique<CreateSkillListener>(
        this->getBuffID(), [this](Skill *const skill)
        { this->listenerCallback(skill); });
    CreateSkillAction::addListener(std::move(info));

    auto info4 = std::make_unique<SecondaryAttributeListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback2(n); });
    ProficientCountModifyAction::addListener(std::move(info4));
}

void JQRBuff::listenerCallback(Skill *const skill) 
{
    if(skill->hasSkillType(Skill::skillTypeEnum::SPECIALIZED) || skill->hasSkillType(Skill::skillTypeEnum::PARTICULAR))
    {
        // 专精技能增伤30%
        skill->damageIncreaseAdd += 0.3;
    }
}

void JQRBuff::listenerCallback2(double n) 
{
    // 当精通属性点数变化时，重新计算 refineATKIncrease
    double proficientIncrease = this->p->getProficient() / 5 * this->number;
    double refineATKIncrease = this->p->getTotalATK() * proficientIncrease;

    // 先移除之前的增伤，再应用新的增伤
    this->p->triggerAction<RefineATKCountModifyAction>(-this->tempValue);
    this->p->triggerAction<RefineATKCountModifyAction>(refineATKIncrease);

    // 更新 tempValue 为新的增伤值
    this->tempValue = refineATKIncrease;
}

void JQRBuff::update(const double) {}
bool JQRBuff::shouldBeRemoved() { return this->duration < 0; }
std::string JQRBuff::getBuffName() const { return JQRBuff::name; }

JQRBuff::~JQRBuff()
{
    CreateSkillAction::deleteListener(this->getBuffID());
    ProficientCountModifyAction::deleteListener(this->getBuffID());
    this->p->triggerAction<RefineATKCountModifyAction>(-this->tempValue);
}