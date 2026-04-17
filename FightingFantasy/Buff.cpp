#include "Buff.h"
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
                        " - before person Crit Count + 4480, persent: " +
                        std::to_string(p->Critical));

    //p->changeCriticalCount(11200);
    this->p->triggerAction<CriticalCountModifyAction>(11200);

    Logger::debugBuff(AutoAttack::getTimer(),
                        this->getBuffName(),
                        " - after person Crit Count + 4480, persent: " +
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
    this->p->triggerAction<CriticalCountModifyAction>(-11200);
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
}

void MukuScoutBuff::listenerCallback(const DamageInfo &) {}
void MukuScoutBuff::update(const double) {}
bool MukuScoutBuff::shouldBeRemoved() { return this->duration < 0; }
std::string MukuScoutBuff::getBuffName() const { return MukuScoutBuff::name; }

MukuScoutBuff::~MukuScoutBuff()
{
    //this->p->changeAattackIncrease(-this->number);
    this->p->triggerAction<AttackIncreaseModifyAction>(-this->number);
}

// 博伊斯
std::string BYSBuff::name = "BYSBuff";

BYSBuff::BYSBuff(Person *p, double) : Buff(p)
{
    this->number = 0; // 用作增攻数值
    this->duration = 2000;
    this->maxDuration = this->duration;
    this->p->triggerAction<AttackIncreaseModifyAction>(0.26);
    this->p->triggerAction<AttackSpeedPercentModifyAction>(0.1);
}

void BYSBuff::listenerCallback(const DamageInfo &) {}

void BYSBuff::update(const double) {}
bool BYSBuff::shouldBeRemoved() { return this->duration < 0; }
std::string BYSBuff::getBuffName() const { return BYSBuff::name; }

BYSBuff::~BYSBuff()
{
    this->p->triggerAction<AttackIncreaseModifyAction>(-0.26);
    this->p->triggerAction<AttackSpeedPercentModifyAction>(-0.1);
}

// 伊戈雷乌斯
std::string YGLWSBuff::name = "YGLWSBuff";

YGLWSBuff::YGLWSBuff(Person *p, double) : Buff(p)
{
    this->number = 0; // 用作增攻数值
    this->duration = 2000;
    this->maxDuration = this->duration;
    this->p->triggerAction<CriticalCountModifyAction>(14000);
    this->p->triggerAction<CriticalPercentModifyAction>(0.14);
}

void YGLWSBuff::listenerCallback(const DamageInfo &) {}
void YGLWSBuff::update(const double) 
{
    static double lastCritical = 0;
    lastCritical = this->p->getCritical();
    if(this->p->getCritical() > 0.6)
    {
        double lastAdd = (lastCritical - 0.6) * 0.3;
        this->p->triggerAction<CriticalPercentModifyAction>(-lastAdd);
        double add = (this->p->getCritical() - 0.6) * 0.3; 
        this->p->triggerAction<CriticalPercentModifyAction>(add);
    }
}
bool YGLWSBuff::shouldBeRemoved() { return this->duration < 0; }
std::string YGLWSBuff::getBuffName() const { return YGLWSBuff::name; }

YGLWSBuff::~YGLWSBuff()
{
    this->p->triggerAction<CriticalCountModifyAction>(-14000);
    this->p->triggerAction<CriticalPercentModifyAction>(-0.14);
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
        skill->fixedValue += this->number * this->p->getATK();
    }
    if(skill->getCanTriggerLucky())
    {
        skill->luckyFiexedValue += this->number * this->p->getATK();
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
    this->duration = 9999999;
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
}

void HYXZBuff::listenerCallback(Skill *const skill) {}

void HYXZBuff::update(const double) {}
bool HYXZBuff::shouldBeRemoved() { return this->duration < 0; }
std::string HYXZBuff::getBuffName() const { return HYXZBuff::name; }
HYXZBuff::~HYXZBuff() 
{
    this->p->triggerAction<ProficientPercentModifyAction>(-0.2);
}