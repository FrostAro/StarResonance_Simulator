#include "Skill.h"
#include "../FightingFantasy/Buff.h"
#include "../core/Action.h"
#include "../core/Person.h"

// 姆克头目
std::string MukuChief::name = "MukuChief";

MukuChief::MukuChief(Person *p) : FightingFantasy(), InstantSkill()
{

    this->canCharge = true;

    this->multiplying = 0;
    this->fixedValue = 0;

    this->MaxCD = 6000;
    this->MaxchargeCD = 100;
    this->CD = 0;
    this->chargeCD = 0;
    this->maxStack = 2;
    this->stack = this->maxStack;
    this->releasingTime = 10;
    this->releasingTime /= (1 + p->attackSpeed);

    this->MukuChief::setSkillType();
}

void MukuChief::setSkillType()
{
    this->skillTypeList.push_back(skillTypeEnum::NORMAL);
}

void MukuChief::trigger(Person *p)
{
    p->triggerAction<CreateBuffAction>(0, MukuChiefBuff::name);
}

std::string MukuChief::getSkillName() const
{
    return MukuChief::name;
}

void MukuChief::setPassiveEffect(Person *p)
{
    //p->changeCriticalDamage(0.25);
    p->triggerAction<CriticalDamageModifyAction>(0.25);
}

void MukuChief::removePassiveEffect(Person *p)
{
    //p->changeCriticalDamage(-0.25);
    p->triggerAction<CriticalDamageModifyAction>(-0.25);
}

// 姆克尖兵
std::string MukuScout::name = "MukuScout";

MukuScout::MukuScout(Person *p) : FightingFantasy(), InstantSkill()
{
    this->multiplying = 0;
    this->fixedValue = 0;

    this->MaxCD = 8000;
    this->MaxchargeCD = 100;
    this->CD = 0;
    this->chargeCD = 0;
    this->releasingTime = 10;
    this->releasingTime /= (1 + p->attackSpeed);

    this->MukuScout::setSkillType();
}

void MukuScout::setSkillType()
{
    this->skillTypeList.push_back(skillTypeEnum::NORMAL);
}

void MukuScout::trigger(Person *p)
{
    p->triggerAction<CreateBuffAction>(0, MukuScoutBuff::name);
}

std::string MukuScout::getSkillName() const
{
    return MukuScout::name;
}

void MukuScout::setPassiveEffect(Person *p)
{
    p->triggerAction<AttackIncreaseModifyAction>(0.15);
}

void MukuScout::removePassiveEffect(Person *p)
{
    p->triggerAction<AttackIncreaseModifyAction>(-0.15);
}

// 博伊斯
std::string BYS::name = "BYS";

BYS::BYS(Person *p) : FightingFantasy(), InstantSkill()
{

    this->canCharge = true;
    this->maxStack = 2;
    this->stack = this->maxStack;

    this->multiplying = 0;
    this->fixedValue = 0;

    this->MaxCD = 8000;
    this->MaxchargeCD = 100;
    this->CD = 0;
    this->chargeCD = 0;
    this->releasingTime = 10;
    this->releasingTime /= (1 + p->attackSpeed);

    this->BYS::setSkillType();
}

void BYS::setSkillType()
{
    this->skillTypeList.push_back(skillTypeEnum::NORMAL);
}

void BYS::trigger(Person *p)
{
    p->triggerAction<CreateBuffAction>(0, BYSBuff::name);
}

std::string BYS::getSkillName() const
{
    return BYS::name;
}

void BYS::setPassiveEffect(Person *p)
{
    p->triggerAction<PrimaryAttributesPercentModifyAction>(0.15);
}

void BYS::removePassiveEffect(Person *p)
{
    p->triggerAction<PrimaryAttributesPercentModifyAction>(-0.15);
}

// 伊戈雷乌斯
std::string YGLWS::name = "YGLWS";

YGLWS::YGLWS(Person *p) : FightingFantasy(), InstantSkill()
{

    this->canCharge = true;
    this->maxStack = 2;
    this->stack = this->maxStack;

    this->multiplying = 0;
    this->fixedValue = 0;

    this->MaxCD = 6000;
    this->MaxchargeCD = 100;
    this->CD = 0;
    this->chargeCD = 0;
    this->releasingTime = 10;
    this->releasingTime /= (1 + p->attackSpeed);

    this->YGLWS::setSkillType();
}

void YGLWS::setSkillType()
{
    this->skillTypeList.push_back(skillTypeEnum::NORMAL);
}

void YGLWS::trigger(Person *p)
{
    p->triggerAction<CreateBuffAction>(0, YGLWSBuff::name);
}

std::string YGLWS::getSkillName() const
{
    return YGLWS::name;
}

void YGLWS::setPassiveEffect(Person *p)
{
    p->triggerAction<CriticalDamageModifyAction>(0.35);
}

void YGLWS::removePassiveEffect(Person *p)
{
    p->triggerAction<CriticalDamageModifyAction>(-0.35);
}

// 嗜血毛球
std::string SXMQ::name = "SXMQ";

SXMQ::SXMQ(Person *p) : FightingFantasy(), InstantSkill()
{
    this->maxStack = 1;
    this->stack = this->maxStack;

    this->multiplying = 0;
    this->fixedValue = 0;

    this->MaxCD = 8000;
    this->MaxchargeCD = 100;
    this->CD = 0;
    this->chargeCD = 0;
    this->releasingTime = 10;
    this->releasingTime /= (1 + p->attackSpeed);

    this->SXMQ::setSkillType();
    p->triggerAction<CreateBuffAction>(0,SXMQBuff_Passive::name);
}

void SXMQ::setSkillType()
{
    this->skillTypeList.push_back(skillTypeEnum::NORMAL);
}

void SXMQ::trigger(Person *p)
{
    p->triggerAction<CreateBuffAction>(0, SXMQBuff::name);
}

std::string SXMQ::getSkillName() const
{
    return SXMQ::name;
}

void SXMQ::setPassiveEffect(Person *p) {}
void SXMQ::removePassiveEffect(Person *p) {}


// 幻妖蟹蛛
std::string HYXZ::name = "HYXZ";

HYXZ::HYXZ(Person *p) : FightingFantasy(), InstantSkill()
{

    this->maxStack = 1;
    this->stack = this->maxStack;

    this->multiplying = 0;
    this->fixedValue = 0;

    this->MaxCD = 8000;
    this->MaxchargeCD = 100;
    this->CD = 0;
    this->chargeCD = 0;
    this->releasingTime = 10;
    this->releasingTime /= (1 + p->attackSpeed);

    this->HYXZ::setSkillType();
}

void HYXZ::setSkillType()
{
    this->skillTypeList.push_back(skillTypeEnum::NORMAL);
}

void HYXZ::trigger(Person *p)
{
    p->triggerAction<CreateBuffAction>(0, HYXZBuff::name);
}

std::string HYXZ::getSkillName() const
{
    return HYXZ::name;
}

void HYXZ::setPassiveEffect(Person *p)
{
    p->triggerAction<ProficientCountModifyAction>(8960);
}

void HYXZ::removePassiveEffect(Person *p)
{
    p->triggerAction<ProficientCountModifyAction>(-8960);
}