#include "Skill.h"
#include "Action.h"
#include "../../core/Action.h"
#include "../../core/Person.h"
#include "Buff.h"
#include "../../core/Logger.h"
#include "../../core/AutoAttack.h"

// 射线
std::string Beam::name = "Beam";

Beam::Beam(Person* p) 
        : FacilitationSkill()
{
    this->canTriggerLucky = true;

    this->multiplying = 0.42L;
    this->fixedValue = 150;
    this->baseMultiplying = this->multiplying;
    this->baseFixedValue = this->fixedValue;
    this->damageTriggerInterval = 30;
    this->damageTriggerInterval = this->damageTriggerInterval / (1 + p->castingSpeed);
    this->MaxCD = 0;
    this->CD = 0;
    this->releasingTime = 85;

    this->damageIncreaseAdd = 0.16; 

    this->energyReduce = 12 * (this->damageTriggerInterval / 30);
    this->duration = 999999;
    this->noEnergyConsumeTime = 5;

    this->setSkillType();
}

void Beam::setSkillType()
{
    this->skillTypeList.push_back(skillTypeEnum::PARTICULAR);
    this->skillTypeList.push_back(skillTypeEnum::WITCHCRAFT);
    this->skillTypeList.push_back(skillTypeEnum::FAR);
}

void Beam::trigger(Person* p)
{
    this->count++;
    p->triggerAction<AttackAction>(0, this);
    if(this->count > this->noEnergyConsumeTime)
    {
        p->triggerAction<EnergyConsumeAction>(this->energyReduce,this);
    }
    if(addMultiplyingTimes < 10)
    {
        this->multiplying += this->baseMultiplying * 0.08;
        this->fixedValue += this->baseFixedValue * 0.08;
        addMultiplyingTimes += 1;
    }

    Logger::debugSkill(AutoAttack::getTimer(),this->getSkillName(),"beam attacked, beam count: " + std::to_string(this->count));
    Logger::debugSkill(AutoAttack::getTimer(),this->getSkillName(),"beam consumed energy, person current energy: " + std::to_string(p->getPresentEnergy()));
}

std::string Beam::getSkillName() const{ return Beam::name; }

bool Beam::canEndFacilitation(Person* p){ 
    return p->present_energy < this->energyReduce * (1 + this->energyReduceUP) * (1 - this->energyReduceDOWN)
                                                || this->duration <= 0; }


// 涡流
std::string Vortex::name = "Vortex";

Vortex::Vortex(Person* p) : ContinuousSkill()
{
    this->multiplying = 0.2L;
    this->fixedValue = 120;
    this->damageTriggerInterval = 50;
    this->duration = 1000;
    this->MaxCD = 1500;
    this->releasingTime = 20;

    this->energyAdd = 2;

    this->setSkillType();
}

void Vortex::setSkillType()
{
    this->skillTypeList.push_back(skillTypeEnum::SPECIALIZED);  
    this->skillTypeList.push_back(skillTypeEnum::WITCHCRAFT);
    this->skillTypeList.push_back(skillTypeEnum::FAR);
}

void Vortex::trigger(Person* p)
{
    p->triggerAction<AttackAction>(0,this);
    p->triggerAction<EnergyRevertAction_Beam>(this->energyAdd,this);
}

std::string Vortex::getSkillName() const{ return Vortex::name; }

// 水龙卷
// 龙卷默认在释放射线后释放，不实现单独释放逻辑
std::string WaterSpout::name = "WaterSpout";

WaterSpout::WaterSpout(Person* p) : ContinuousSkill()
{
    this->isNoReleasing = true;

    this->multiplying = 1.4448L;
    this->fixedValue = 514;
    this->damageTriggerInterval = 45;
    this->damageTriggerInterval = this->damageTriggerInterval / (1 + p->castingSpeed);
    this->MaxCD = 3000;
    this->duration = 2000;

    this->setSkillType();
}

void WaterSpout::setSkillType()
{
    this->skillTypeList.push_back(skillTypeEnum::SPECIALIZED);  
    this->skillTypeList.push_back(skillTypeEnum::WITCHCRAFT);
    this->skillTypeList.push_back(skillTypeEnum::FAR);
}

void WaterSpout::trigger(Person* p)
{
    p->triggerAction<AttackAction>(0,this);

    // 如果射线释放完，结束水龙卷
    if(p->getNowReleasingSkill() == nullptr || p->getNowReleasingSkill()->getSkillName() != Beam::name)
    {
        this->duration = 0;
        // 最后的爆炸伤害
        
    }
}

std::string WaterSpout::getSkillName() const{ return WaterSpout::name; }

// 冻结寒风
std::string FrostWind::name = "FrostWind";

FrostWind::FrostWind(Person* p) : InstantSkill()
{
    this->canTriggerLucky = true;
    this->multiplying = 1.75L;
    this->fixedValue = 1050;
    this->MaxCD = 3000;

    // 无视吟唱时间，默认满玄冰释放
    this->releasingTime = 10;

    this->setSkillType();
}

void FrostWind::setSkillType()
{
    this->skillTypeList.push_back(skillTypeEnum::SPECIALIZED);  
    this->skillTypeList.push_back(skillTypeEnum::WITCHCRAFT);
    this->skillTypeList.push_back(skillTypeEnum::FAR);
}

void FrostWind::trigger(Person* p)
{
    p->triggerAction<EnergyRevertAction_Beam>(5 * p->getMaxResourceNum());

    p->triggerAction<AttackAction>(0,this);
    p->triggerAction<ResourceConsumeAction>(6);
    // 触发涌能法则
    p->triggerAction<CreateBuffAction>(0,EnergySurgeLawBuff::name);
    // 触发寒风凝聚
    p->triggerAction<CreateBuffAction>(0,FrostwindFocusBuff::name);
    // 触发冰晶坠落
    p->triggerAction<CreateSkillAction>(0,CrystalsHail::name);
}

std::string FrostWind::getSkillName() const{ return FrostWind::name; }

// 灌注
std::string Flood_Beam::name = "Flood_Beam";

Flood_Beam::Flood_Beam(Person* p) : InstantSkill()
{
    this->MaxCD = 4500;

    // 无视吟唱时间，默认满玄冰释放
    this->releasingTime = 75;

    this->setSkillType();
}

void Flood_Beam::setSkillType()
{
    this->skillTypeList.push_back(skillTypeEnum::SPECIALIZED);  
    this->skillTypeList.push_back(skillTypeEnum::WITCHCRAFT);
    this->skillTypeList.push_back(skillTypeEnum::FAR);
}

void Flood_Beam::trigger(Person* p)
{
    // 触发射线的灌注buff
    p->triggerAction<CreateBuffAction>(0,FloodBuff_Beam::name);
}

std::string Flood_Beam::getSkillName() const{ return Flood_Beam::name; }

// 冰晶坠落
std::string CrystalsHail::name = "CrystalsHail";

CrystalsHail::CrystalsHail(Person* p) : InstantSkill()
{
    this->multiplying = 6;
    this->energyAdd = 30;
    this->isNoReleasing = true;
    this->duration = 10;
    this->damageTriggerInterval = 10;

    this->setSkillType();
}

void CrystalsHail::setSkillType()
{  
    this->skillTypeList.push_back(skillTypeEnum::WITCHCRAFT);
    this->skillTypeList.push_back(skillTypeEnum::FAR);
}

void CrystalsHail::trigger(Person* p)
{
    p->triggerAction<AttackAction>(0,this);
    p->triggerAction<EnergyRevertAction_Beam>(this->energyAdd,this);
    p->triggerAction<CreateBuffAction>(0,FrostwindFocusBuff::name);
}

std::string CrystalsHail::getSkillName() const{ return CrystalsHail::name; }

// 冰令脉冲
std::string FrostDecreePulse::name = "FrostDecreePulse";

FrostDecreePulse::FrostDecreePulse(Person* p) : InstantSkill()
{
    this->duration = 1;
    this->damageTriggerInterval = 1;
    this->multiplying = 5;
    this->isNoReleasing = true;

    this->setSkillType();
}

void FrostDecreePulse::setSkillType()
{ 
    this->skillTypeList.push_back(skillTypeEnum::WITCHCRAFT);
    this->skillTypeList.push_back(skillTypeEnum::FAR);
}

void FrostDecreePulse::trigger(Person* p)
{
    p->triggerAction<AttackAction>(0,this);
}

std::string FrostDecreePulse::getSkillName() const { return FrostDecreePulse::name; }

// 冰箭
std::string IceArrow_Beam::name = "IceArrow_Beam";

IceArrow_Beam::IceArrow_Beam(Person *p) : InstantSkill()
{
    this->energyAdd = 2;
    this->multiplying = 0.28;
    this->fixedValue = 0;
    this->duration = 53;
    this->damageTriggerInterval = 53;
    this->damageTriggerTimer = 0;
    this->MaxCD = 0;
    this->CD = 0;
    this->chargeCD = 0;
    this->MaxchargeCD = 0;
    this->isNoReleasing = true;

    this->criticalAdd = 1;
    this->criticalIncreaseAdd = 0.1;
    this->finalIncreaseAdd = 1;

    this->IceArrow_Beam::setSkillType();
    p->triggerAction<CreateSkillAction>(0,FrostBurst::name);
    p->triggerAction<ResourceRevertAction>(1);
}

void IceArrow_Beam::setSkillType()
{
    this->skillTypeList.push_back(skillTypeEnum::NORMAL);
    this->skillTypeList.push_back(skillTypeEnum::WITCHCRAFT);
}

void IceArrow_Beam::trigger(Person *p)
{

    p->triggerAction<AttackAction>(0, this); // 冰箭本体伤害
    p->triggerAction<AttackAction>(0, this); // 冰箭本体伤害
    p->triggerAction<EnergyRevertAction_Beam>(this->energyAdd,this);  // 冰光共鸣
}

std::string IceArrow_Beam::getSkillName() const
{
    return IceArrow_Beam::name;
}

// 霜爆（冰箭爆炸）
std::string FrostBurst::name = "FrostBurst";

FrostBurst::FrostBurst(Person *p) : InstantSkill()
{
    this->multiplying = 0.56;
    this->fixedValue = 0;
    this->duration = 53;
    this->damageTriggerInterval = 53;
    this->damageTriggerTimer = 0;
    this->MaxCD = 0;
    this->CD = 0;
    this->chargeCD = 0;
    this->MaxchargeCD = 0;
    this->isNoReleasing = true;

    this->criticalAdd = 1;
    this->criticalIncreaseAdd = 0.1;

    this->FrostBurst::setSkillType();
}

void FrostBurst::setSkillType()
{
    this->skillTypeList.push_back(skillTypeEnum::NORMAL);
    this->skillTypeList.push_back(skillTypeEnum::WITCHCRAFT);
}

void FrostBurst::trigger(Person *p)
{
    p->triggerAction<AttackAction>(0, this); // 冰箭爆炸伤害
}

std::string FrostBurst::getSkillName() const
{
    return FrostBurst::name;
}

// 大招
std::string Ultimate_Beam::name = "Ultimate_Beam";

Ultimate_Beam::Ultimate_Beam(Person *) : InstantSkill()
{
    this->canTriggerLucky = true;

    this->multiplying = 12.60;
    this->fixedValue = 5400;
    this->MaxCD = 5400;
    this->CD = 0;
    this->chargeCD = 0;
    this->MaxchargeCD = 0;
    this->releasingTime = 10;

    this->setSkillType();
}

void Ultimate_Beam::setSkillType()
{
    this->skillTypeList.push_back(skillTypeEnum::WITCHCRAFT);
    this->skillTypeList.push_back(skillTypeEnum::SPECIALIZED);
}

void Ultimate_Beam::trigger(Person *p)
{
    p->triggerAction<CreateBuffAction>(0, UltiIncreaseBuff_Beam::name);
    p->triggerAction<AttackAction>(0, this);
}

std::string Ultimate_Beam::getSkillName() const
{
    return Ultimate_Beam::name;
}


