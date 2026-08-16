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

    this->duration = 2000;
    this->maxDuration = this->duration;

    Logger::debugBuff(AutoAttack::getTimer(),
                        this->getBuffName(),
                        " - before person Crit Count + 4480, percent: " +
                        std::to_string(p->Critical));

    //p->changeCriticalCount(11200);
    this->p->triggerAction<CriticalCountModifyAction>(44800);

    Logger::debugBuff(AutoAttack::getTimer(),
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
    this->duration = 2000;
    this->maxDuration = this->duration;

    //p->changeAattackIncrease(this->number);
    this->p->triggerAction<AttackIncreaseModifyAction>(this->number);
    Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "attackIncrease applied");
    Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "attackIncrease applied");
}

void MukuScoutBuff::listenerCallback(const DamageInfo &) {}
void MukuScoutBuff::update(const double) {}
bool MukuScoutBuff::shouldBeRemoved() { return this->duration < 0; }
std::string MukuScoutBuff::getBuffName() const { return MukuScoutBuff::name; }

MukuScoutBuff::~MukuScoutBuff()
{
    //this->p->changeAattackIncrease(-this->number);
    this->p->triggerAction<AttackIncreaseModifyAction>(-this->number);
    Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "attackIncrease removed");
}

// 博伊斯
std::string BYSBuff::name = "BYSBuff";

BYSBuff::BYSBuff(Person *p, double) : Buff(p)
{
    this->number = 0; // 用作增攻数值
    this->duration = 2000;
    this->maxDuration = this->duration;
    this->p->triggerAction<AttackIncreaseModifyAction>(0.26);
    Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "attackIncrease applied");
    this->p->triggerAction<AttackSpeedPercentModifyAction>(0.1);
    Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "attackSpeed applied");
}

void BYSBuff::listenerCallback(const DamageInfo &) {}

void BYSBuff::update(const double) {}
bool BYSBuff::shouldBeRemoved() { return this->duration < 0; }
std::string BYSBuff::getBuffName() const { return BYSBuff::name; }

BYSBuff::~BYSBuff()
{
    this->p->triggerAction<AttackIncreaseModifyAction>(-0.26);
    Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "attackIncrease removed");
    this->p->triggerAction<AttackSpeedPercentModifyAction>(-0.1);
    Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "attackSpeed removed");
}

// 伊戈雷乌斯
std::string YGLWSBuff::name = "YGLWSBuff";

YGLWSBuff::YGLWSBuff(Person *p, double) : Buff(p)
{
    this->number = 0; // 用作增攻数值
    this->duration = 2000;
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
    Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "critical damage correction removed");
        }
        if (targetReduction != 0.0)
        {
            this->p->triggerAction<CriticalDamageModifyAction>(targetReduction);
        }
        this->lastReduction = targetReduction;
        Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(),
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
    Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "critical damage correction removed");
}

// 嗜血毛球
std::string SXMQBuff::name = "SXMQBuff";

SXMQBuff::SXMQBuff(Person *p, double) : Buff(p)
{
    this->number = 0.78; // 用作主动倍率
    this->duration = 2000;
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
        Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "skill fixedValue increased");
    }
    if(skill->getCanTriggerLucky())
    {
        skill->luckyFixedValue += this->number * this->p->getATK() * (1 + this->p->getAttackIncrease());
        Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "skill luckyFixedValue increased");
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
            Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "SXMQ passive attack triggered");
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
    this->duration = 2000;
    this->maxDuration = this->duration;

    p->triggerAction<ProficientPercentModifyAction>(0.2);
    Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "proficientPercent applied");
}

void HYXZBuff::listenerCallback(Skill *const skill) {}

void HYXZBuff::update(const double) {}
bool HYXZBuff::shouldBeRemoved() { return this->duration < 0; }
std::string HYXZBuff::getBuffName() const { return HYXZBuff::name; }
HYXZBuff::~HYXZBuff() 
{
    this->p->triggerAction<ProficientPercentModifyAction>(-0.2);
    Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "proficientPercent removed");
}

// 掠食蜘蛛
std::string LSZZBuff::name = "LSZZBuff";

LSZZBuff::LSZZBuff(Person *p, double) : Buff(p)
{
    this->number = 0.35; // 用作主动倍率
    this->duration = 2000;
    this->maxDuration = this->duration;

    p->triggerAction<AttackIncreaseModifyAction>(this->number);
    Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "attackIncrease applied");
}

void LSZZBuff::update(const double) {}
bool LSZZBuff::shouldBeRemoved() { return this->duration < 0; }
std::string LSZZBuff::getBuffName() const { return LSZZBuff::name; }

LSZZBuff::~LSZZBuff() 
{
    this->p->triggerAction<AttackIncreaseModifyAction>(-this->number);
    Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "attackIncrease removed");
}

// 游子
std::string YZBuff::name = "YZBuff";

YZBuff::YZBuff(Person *p, double) : Buff(p)
{
    this->duration = 2000;
    this->maxDuration = this->duration;

    p->triggerAction<LuckyPercentModifyAction>(0.2);
    Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "luckyPercent applied");
}

void YZBuff::listenerCallback(Skill *const skill) {}

void YZBuff::update(const double) {}
bool YZBuff::shouldBeRemoved() { return this->duration < 0; }
std::string YZBuff::getBuffName() const { return YZBuff::name; }
YZBuff::~YZBuff() 
{
    this->p->triggerAction<LuckyPercentModifyAction>(-0.2);
    Logger::debugBuff(AutoAttack::getTimer(), this->getBuffName(), "luckyPercent removed");
}