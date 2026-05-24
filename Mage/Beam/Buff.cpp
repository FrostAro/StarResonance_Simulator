#include "Buff.h"
#include "Skill.h"
#include "Action.h"
#include "../../core/Person.h"
#include "../../core/AutoAttack.h"
#include "../../core/Listener.hpp"
#include "../../core/Action.h"
#include "../../core/Logger.h"
#include <string>

// 射线部分
// 射线流派天赋
std::string BeamBuildBuff::name = "BeamBuildBuff";

BeamBuildBuff::BeamBuildBuff(Person *p, double)
    : Buff(p)
{
    this->stack = 0;
    this->number = 0.15;
    this->duration = 99999;
    this->maxDuration = this->duration;
    this->lastExtraIncrease = 0;
    this->proficientRatio = 0.085;
    this->energyRatio = 0.3;
    this->triggerNum = 13;
    this->isInherent = true;

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &damageInfo)
        { this->listenerCallback(damageInfo); });
    AttackAction::addListener(std::move(info));

    auto info2 = std::make_unique<ResourceListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback2(n); });
    ResourceConsumeAction::addListener(std::move(info2));
    auto info3 = std::make_unique<ResourceListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback2(n); });
    ResourceRevertAction::addListener(std::move(info3));
}

void BeamBuildBuff::listenerCallback(DamageInfo &info)
{
    // 射线出伤13次减1s灌注cd
    if (info.skillName == Beam::name)
    {
        this->count++;
        if (this->count >= this->triggerNum)
        {
            this->p->triggerAction<CDReduceAction>(100, Flood_Beam::name);
            Logger::debugBuff(AutoAttack::getTimer(),
                              this->getBuffName(),
                              "ReduceCD triggered ");
        }
    }

    if(info.skillName == IceArrow_Beam::name && info.isCritical)
    {
        this->p->triggerAction<CreateSkillAction>(0,FrostBurst::name);
    }
}

void BeamBuildBuff::listenerCallback2(double)
{
    Skill *const skill = this->p->getNowReleasingSkill();
    if (!skill)
        return;
    if (skill->getSkillName() == Beam::name)
    {
        // 减去上次增加的
        skill->damageIncreaseAdd -= this->lastExtraIncrease;
        skill->changeEnergyReduceUP(-this->stack * this->energyRatio);

        // 设置数值
        this->stack = this->p->getResourceNum();
        this->lastExtraIncrease = this->stack * this->number + this->stack * this->p->Proficient * this->proficientRatio;

        // 重新设置射线增伤与能量增耗
        skill->damageIncreaseAdd += this->lastExtraIncrease;
        skill->changeEnergyReduceUP(this->stack * this->energyRatio);

        Logger::debugBuff(AutoAttack::getTimer(),
                          this->getBuffName(),
                          "current energyReduceUP: " + std::to_string(skill->getEnergyReduceUP()));
    }
    else
    {
        this->lastExtraIncrease = 0;
    }
}

void BeamBuildBuff::update(const double) {}
bool BeamBuildBuff::shouldBeRemoved() { return this->duration < 0; }
std::string BeamBuildBuff::getBuffName() const { return BeamBuildBuff::name; }

BeamBuildBuff::~BeamBuildBuff()
{
    AttackAction::deleteListener(this->getBuffID());
    ResourceConsumeAction::deleteListener(this->getBuffID());
    ResourceRevertAction::deleteListener(this->getBuffID());
}

// 自然回能
std::string NaturalEnergyRegenBuff::name = "NaturalEnergyRegenBuff";

NaturalEnergyRegenBuff::NaturalEnergyRegenBuff(Person *p, double) : Buff(p)
{
    this->number = 2; // 自然回能数
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;
    this->triggerInterval = 100 / (1 + this->p->Quickness);// 寒流效果
}

void NaturalEnergyRegenBuff::update(double deltaTime)
{
    this->timer += deltaTime;
    if (this->timer >= this->triggerInterval) 
    {
        this->p->triggerAction<EnergyRevertAction_Beam>(this->number);
        this->timer -= this->triggerInterval;
    }
}

bool NaturalEnergyRegenBuff::shouldBeRemoved() { return this->duration < 0; }
std::string NaturalEnergyRegenBuff::getBuffName() const { return NaturalEnergyRegenBuff::name; }

// 冰凌之约
std::string IcePromiseBuff::name = "IcePromiseBuff";

IcePromiseBuff::IcePromiseBuff(Person *p, double) : Buff(p)
{
    this->number = 2;
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    // 注册创建技能事件的回调
    auto createSkillInfo = std::make_unique<CreateSkillListener>(
        this->getBuffID(), [this](Skill *const skill)
        { this->listenerCallback(skill); });
    CreateSkillAction::addListener(std::move(createSkillInfo));
}

void IcePromiseBuff::listenerCallback(Skill *const skill)
{
    if (!skill)
        return;
    if (skill->getSkillName() == Vortex::name)
    {
        skill->energyAdd *= 3;
    }
}

void IcePromiseBuff::update(double) {}

bool IcePromiseBuff::shouldBeRemoved() { return this->duration < 0; }

IcePromiseBuff::~IcePromiseBuff()
{
    CreateSkillAction::deleteListener(this->getBuffID());
}

std::string IcePromiseBuff::getBuffName() const { return IcePromiseBuff::name; }

// 冰晶共鸣&冰光共鸣&冰箭速射（射线触发冰箭）
std::string FrostCrystalResonanceBuff::name = "FrostCrystalResonanceBuff";

FrostCrystalResonanceBuff::FrostCrystalResonanceBuff(Person *p, double)
    : Buff(p)
{
    this->duration = 99999;
    this->maxDuration = this->duration;
    this->isInherent = true;
    this->triggerInterval = 50;

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &damageInfo)
        { this->listenerCallback(damageInfo); });
    AttackAction::addListener(std::move(info));

    auto info2 = std::make_unique<CreateSkillListener>(
        this->getBuffID(), [this](Skill *const skill)
        { this->listenerCallback2(skill); });
    CreateSkillAction::addListener(std::move(info2));
}

void FrostCrystalResonanceBuff::listenerCallback(DamageInfo &info)
{
    // 冰箭速射part
    
}

void FrostCrystalResonanceBuff::listenerCallback2(Skill *const skill)
{
    if(skill->getSkillName() == IceArrow_Beam::name)
    {
        // 冰光共鸣部分
        if(this->p->findBuffInBuffList(FloodBuff_Beam::name) != -1)
        {
            skill->energyAdd *= 2;
        }
    }
}

void FrostCrystalResonanceBuff::update(const double deltaTime)
{
    if (this->p->getNowReleasingSkill() != nullptr && this->p->getNowReleasingSkill()->getSkillName() == Beam::name)
    {
        this->triggerTimer += deltaTime;
        if (this->triggerTimer >= this->triggerInterval)
        {
            // 触发冰箭
            this->p->triggerAction<CreateSkillAction>(0, IceArrow_Beam::name);
            // 冰箭速射part
            // 灌注期冰箭数量+2
            int index = this->p->findBuffInBuffList(FloodBuff_Beam::name);
            if (index != -1)
            {
                this->p->triggerAction<CreateSkillAction>(0, IceArrow_Beam::name);
                this->p->triggerAction<CreateSkillAction>(0, IceArrow_Beam::name);
                Logger::debugBuff(AutoAttack::getTimer(),
                                  this->getBuffName(),
                                  "more iceArrow triggered ");
            }
            this->triggerTimer -= this->triggerInterval;
        }
    }
    else
    {
        this->triggerTimer = 0;
    }
}
bool FrostCrystalResonanceBuff::shouldBeRemoved() { return this->duration < 0; }
std::string FrostCrystalResonanceBuff::getBuffName() const { return FrostCrystalResonanceBuff::name; }

FrostCrystalResonanceBuff::~FrostCrystalResonanceBuff()
{
    AttackAction::deleteListener(this->getBuffID());
    CreateSkillAction::deleteListener(this->getBuffID());
}

// 寒意留存
std::string ChillPersistenceBuff::name = "ChillPersistenceBuff";

ChillPersistenceBuff::ChillPersistenceBuff(Person *p, double)
    : Buff(p)
{
    this->number = 0.25;
    this->duration = 99999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    auto createSkillInfo = std::make_unique<CreateSkillListener>(
        this->getBuffID(), [this](Skill *const skill)
        { this->listenerCallback(skill); });
    CreateSkillAction::addListener(std::move(createSkillInfo));
}

void ChillPersistenceBuff::listenerCallback(Skill *const skill)
{
    if (!skill)
        return;
    // 灌注期射线耗能-25%
    int index = this->p->findBuffInBuffList(FloodBuff_Beam::name);
    if (skill->getSkillName() == Beam::name && index != -1  && !this->triggered)
    {
        skill->changeEnergyReduceDOWN(this->number);
        this->triggered = true;
        Logger::debugBuff(AutoAttack::getTimer(),
                          this->getBuffName(),
                          "beam energyReduce down 25%");
    }
    if ((skill->getSkillName() != Beam::name || index == -1 ) && this->triggered)
    {
        this->triggered = false;
    }
}

void ChillPersistenceBuff::update(const double) {}
bool ChillPersistenceBuff::shouldBeRemoved() { return this->duration < 0; }
std::string ChillPersistenceBuff::getBuffName() const { return ChillPersistenceBuff::name; }

ChillPersistenceBuff::~ChillPersistenceBuff()
{
    CreateSkillAction::deleteListener(this->getBuffID());
}

// 灌注
std::string FloodBuff_Beam::name = "FloodBuff_Beam";

FloodBuff_Beam::FloodBuff_Beam(Person *p, double)
    : Buff(p)
{
    this->number = 0.6; // 用作回能量
    this->duration = 1500;
    this->maxDuration = this->duration;
    this->energyRevertInterval = 10;

    this->p->triggerAction<QuicknessCountModifyAction>(1040);
    this->p->triggerAction<QuicknessPercentModifyAction>(0.21);
}

void FloodBuff_Beam::update(const double deltaTime) 
{
    this->timer += deltaTime;
    if(this->timer >= this->energyRevertInterval)
    {
        this->p->triggerAction<EnergyRevertAction_Beam>(this->number);
        this->timer -= this->energyRevertInterval;
    }
}

bool FloodBuff_Beam::shouldBeRemoved() { return this->duration < 0; }
std::string FloodBuff_Beam::getBuffName() const { return FloodBuff_Beam::name; }

FloodBuff_Beam::~FloodBuff_Beam()
{
    this->p->triggerAction<QuicknessCountModifyAction>(-1040);
    this->p->triggerAction<QuicknessPercentModifyAction>(-0.21);
}

// 智力冰晶
std::string IntellectCrystalBuff::name = "IntellectCrystalBuff";

IntellectCrystalBuff::IntellectCrystalBuff(Person *p, double)
    : Buff(p)
{
    this->number = 50; // 智力转能量数
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    // 根据智力增加能量上限
    double index = p->getPrimaryAttributes() / this->number;
    if (index > 100)
    {
        index = 100;
    }
    p->max_energy += index;
    this->maxEnergyAdd = index;
    p->present_energy = p->max_energy;

    auto info = std::make_unique<PrimaryAttributeListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback(n); });
    PrimaryAttributesCountModifyAction::addListener(std::move(info));
    auto info2 = std::make_unique<PrimaryAttributeListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback(n); });
    PrimaryAttributesPercentModifyAction::addListener(std::move(info2));
}

void IntellectCrystalBuff::listenerCallback(double)
{
    // 智力修改时重新计算
    this->p->max_energy -= this->maxEnergyAdd;
    double index = p->getPrimaryAttributes() / this->number;
    if (index > 100)
    {
        index = 100;
    }
    p->max_energy += index;
    this->maxEnergyAdd = index;
    Logger::debugBuff(AutoAttack::getTimer(),
                      this->getBuffName(),
                      " triggered ");
}

void IntellectCrystalBuff::update(const double) {}
bool IntellectCrystalBuff::shouldBeRemoved() { return this->duration < 0; }
std::string IntellectCrystalBuff::getBuffName() const { return IntellectCrystalBuff::name; }

IntellectCrystalBuff::~IntellectCrystalBuff()
{
    PrimaryAttributesCountModifyAction::deleteListener(this->getBuffID());
    PrimaryAttributesPercentModifyAction::deleteListener(this->getBuffID());
}

// 玄冰潮汐
std::string IceTideBuff::name = "IceTideBuff";

IceTideBuff::IceTideBuff(Person *p, double)
    : Buff(p)
{
    this->number = 0.2; // 射线回玄冰概率
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &damageInfo)
        { this->listenerCallback(damageInfo); });
    AttackAction::addListener(std::move(info));
}

void IceTideBuff::listenerCallback(DamageInfo &info)
{
    // 射线出伤20%概率回1玄冰
    if(info.skillName != Beam::name)
        return;
    if (this->p->isSuccess(this->number))
    {
        this->number = 0.2;
        if (this->p->getResourceNum() < this->p->getMaxResourceNum())
        {
            this->p->triggerAction<ResourceRevertAction>(1);
            Logger::debugBuff(AutoAttack::getTimer(),
                              this->getBuffName(),
                              "revert ice by beam");
        }
        // 玄冰满了转而获得3s元素增伤
        else
        {
            this->p->triggerAction<CreateBuffAction>(0, ElementIncreaseBuff_IceTide::name);
            Logger::debugBuff(AutoAttack::getTimer(),
                              this->getBuffName(),
                              "element increased ");
        }
    }
    else
    {
        this->number += 0.1;
    }
}

void IceTideBuff::update(const double) {}
bool IceTideBuff::shouldBeRemoved() { return this->duration < 0; }
std::string IceTideBuff::getBuffName() const { return IceTideBuff::name; }

IceTideBuff::~IceTideBuff()
{
    AttackAction::deleteListener(this->getBuffID());
}

// 元素增伤buff（用于玄冰潮汐）
std::string ElementIncreaseBuff_IceTide::name = "ElementIncreaseBuff_IceTide";

ElementIncreaseBuff_IceTide::ElementIncreaseBuff_IceTide(Person *p, double)
    : Buff(p)
{
    this->number = 0.05;
    this->duration = 300;
    this->maxDuration = this->duration;
    this->p->triggerAction<ElementIncreaseModifyAction>(this->number);
}

void ElementIncreaseBuff_IceTide::update(const double) {}
bool ElementIncreaseBuff_IceTide::shouldBeRemoved() { return this->duration < 0; }
std::string ElementIncreaseBuff_IceTide::getBuffName() const { return ElementIncreaseBuff_IceTide::name; }

ElementIncreaseBuff_IceTide::~ElementIncreaseBuff_IceTide()
{
    this->p->triggerAction<ElementIncreaseModifyAction>(-this->number);
}

// 玄冰无界
std::string IceInfiniteBuff::name = "IceInfiniteBuff";

IceInfiniteBuff::IceInfiniteBuff(Person *p, double)
    : Buff(p)
{
    this->number = 0.05; // 每玄冰增伤数
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    // 最大玄冰数+3
    p->maxResourceNum += 3;

    auto info = std::make_unique<EnergyListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback(n); });
    EnergyConsumeAction::addListener(std::move(info));

    auto info2 = std::make_unique<ResourceListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback2(n); });
    ResourceConsumeAction::addListener(std::move(info2));

    auto info3 = std::make_unique<ResourceListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback2(n); });
    ResourceRevertAction::addListener(std::move(info3));
}

void IceInfiniteBuff::listenerCallback(double n)
{
    // 每50能量回1玄冰
    energyCount += n;
    if (energyCount >= 50)
    {
        energyCount -= 50;
        p->triggerAction<ResourceRevertAction>(1);
        Logger::debugBuff(AutoAttack::getTimer(),
                          this->getBuffName(),
                          "revert ice by 50 energy");
    }
}

void IceInfiniteBuff::listenerCallback2(double)
{
    // 玄冰修改时重新计算
    // 每1玄冰给射线增伤5%
    Skill *const skill = this->p->getNowReleasingSkill();
    if (!skill)
        return;
    if (skill->getSkillName() == Beam::name)
    {
        skill->damageIncreaseAdd -= lastChange;
        lastChange = this->p->getResourceNum() * this->number;
        skill->damageIncreaseAdd += lastChange;
        Logger::debugBuff(AutoAttack::getTimer(),
                          this->getBuffName(),
                          "damage increased by ice change");
    }
    else
    {
        lastChange = 0;
    }
}

void IceInfiniteBuff::update(const double) {}
bool IceInfiniteBuff::shouldBeRemoved() { return this->duration < 0; }
std::string IceInfiniteBuff::getBuffName() const { return IceInfiniteBuff::name; }

IceInfiniteBuff::~IceInfiniteBuff()
{
    EnergyConsumeAction::deleteListener(this->getBuffID());
    ResourceConsumeAction::deleteListener(this->getBuffID());
    ResourceRevertAction::deleteListener(this->getBuffID());
}

// 射线绝唱
std::string BeamMagnumOpusBuff::name = "BeamMagnumOpusBuff";

BeamMagnumOpusBuff::BeamMagnumOpusBuff(Person *p, double)
    : Buff(p)
{
    this->number = 0.1; // 触发概率
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &info)
        { this->listenerCallback(info); });
    AttackAction::addListener(std::move(info));
}

void BeamMagnumOpusBuff::listenerCallback(DamageInfo &info)
{
    // 射线出伤10%概率使精通翻倍
    if (info.skillName == Beam::name)
    {
        if (this->p->isSuccess(this->number))
        {
            this->p->triggerAction<CreateBuffAction>(0, DoubleProficientBuff::name);
            this->number = 0.1;
            Logger::debugBuff(AutoAttack::getTimer(),
                              this->getBuffName(),
                              " triggered ");
        }
        else
        {
            this->number += 0.1;
        }
    }
}

void BeamMagnumOpusBuff::update(const double) {}
bool BeamMagnumOpusBuff::shouldBeRemoved() { return this->duration < 0; }
std::string BeamMagnumOpusBuff::getBuffName() const { return BeamMagnumOpusBuff::name; }

BeamMagnumOpusBuff::~BeamMagnumOpusBuff()
{
    AttackAction::deleteListener(this->getBuffID());
}

// 双倍精通（用于射线绝唱）
std::string DoubleProficientBuff::name = "DoubleProficientBuff";

DoubleProficientBuff::DoubleProficientBuff(Person *p, double)
    : Buff(p)
{
    this->number = p->getProficientCount();
    this->duration = 300;
    this->maxDuration = this->duration;

    auto info = std::make_unique<SecondaryAttributeListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback(n); });
    ProficientCountModifyAction::addListener(std::move(info));

    p->triggerAction<ProficientCountModifyAction>(this->number);
    Logger::debugBuff(AutoAttack::getTimer(),this->getBuffName(),"triggered, current proficient: " + std::to_string(this->p->getProficient()));
}

void DoubleProficientBuff::listenerCallback(double n)
{
    if(this->timer < 10)
        return;
    this->p->triggerAction<ProficientCountModifyAction>(-this->number);
    this->number = this->p->getProficientCount();
    this->p->triggerAction<ProficientCountModifyAction>(this->number);
    this->timer = 0;
    Logger::debugBuff(AutoAttack::getTimer(),this->getBuffName(),"triggered, current proficient: " + std::to_string(this->p->getProficient()));
}

void DoubleProficientBuff::update(const double deltaTime) 
{
    this->timer += deltaTime;
}

bool DoubleProficientBuff::shouldBeRemoved() { return this->duration < 0; }
std::string DoubleProficientBuff::getBuffName() const { return DoubleProficientBuff::name; }

DoubleProficientBuff::~DoubleProficientBuff()
{
    ProficientCountModifyAction::deleteListener(this->getBuffID());
    this->p->triggerAction<ProficientCountModifyAction>(-this->number);
}

// 寒风凝聚
std::string FrostwindFocusBuff::name = "FrostwindFocusBuff";

FrostwindFocusBuff::FrostwindFocusBuff(Person *p, double)
    : Buff(p)
{
    this->number = 0.03 * 6;
    this->duration = 1500;
    this->maxDuration = this->duration;

    auto info = std::make_unique<CreateSkillListener>(
        this->getBuffID(), [this](Skill *const skill)
        { this->listenerCallback(skill); });
    CreateSkillAction::addListener(std::move(info));
}

void FrostwindFocusBuff::listenerCallback(Skill *const skill)
{
    // 技能耗能减少
    if(skill->getSkillName() != Beam::name)
        return;
    skill->changeEnergyReduceDOWN(this->number);
    Logger::debugBuff(AutoAttack::getTimer(),
                      this->getBuffName(),
                      " triggered, skill:" + skill->getSkillName() +  ", energyReduceDOWN: " + std::to_string(skill->getEnergyReduceDOWN()));
}

void FrostwindFocusBuff::update(const double) {}
bool FrostwindFocusBuff::shouldBeRemoved() { return this->duration < 0; }
std::string FrostwindFocusBuff::getBuffName() const { return FrostwindFocusBuff::name; }

FrostwindFocusBuff::~FrostwindFocusBuff()
{
    CreateSkillAction::deleteListener(this->getBuffID());
}

// 快速吟唱
std::string SwiftCastBuff::name = "SwiftCastBuff";

SwiftCastBuff::SwiftCastBuff(Person *p, double)
    : Buff(p)
{
    this->number = 0.15; // 灌注期增加的施法速度
    this->duration = 99999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    // 1000急速已经包含在了面板里面
    //p->triggerAction<QuicknessCountModifyAction>(1000);
}

void SwiftCastBuff::update(const double)
{
    // 灌注期施法速度+15%
    int index = this->p->findBuffInBuffList(FloodBuff_Beam::name);
    if (!triggered && index != -1)
    {
        this->p->triggerAction<CastingSpeedPercentModifyAction>(this->number);
        this->triggered = true;
    }

    if (triggered && index == -1)
    {
        this->p->triggerAction<CastingSpeedPercentModifyAction>(-this->number);
        this->triggered = false;
    }
}

bool SwiftCastBuff::shouldBeRemoved() { return this->duration < 0; }
std::string SwiftCastBuff::getBuffName() const { return SwiftCastBuff::name; }

SwiftCastBuff::~SwiftCastBuff() {}

// 冰晶之力
std::string FrostCrystalPowerBuff::name = "FrostCrystalPowerBuff";

FrostCrystalPowerBuff::FrostCrystalPowerBuff(Person *p, double)
    : Buff(p)
{
    this->number = 0.08; // 增加的元素增伤
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    auto info = std::make_unique<EnergyListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback(n); });
    EnergyConsumeAction::addListener(std::move(info));

    auto info2 = std::make_unique<EnergyListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback(n); });
    EnergyRevertAction::addListener(std::move(info2));
}

void FrostCrystalPowerBuff::listenerCallback(double)
{
    // 能量高于50%时获得8%元素加成
    if (this->p->getPresentEnergy() >= this->p->getMaxEnergy() / 2 && !triggered)
    {
        this->p->triggerAction<ElementIncreaseModifyAction>(this->number);
        this->triggered = true;
    }

    if (this->p->getPresentEnergy() < this->p->getMaxEnergy() / 2 && triggered)
    {
        this->p->triggerAction<ElementIncreaseModifyAction>(-this->number);
        this->triggered = false;
    }
}

void FrostCrystalPowerBuff::update(const double) {}

bool FrostCrystalPowerBuff::shouldBeRemoved() { return this->duration < 0; }
std::string FrostCrystalPowerBuff::getBuffName() const { return FrostCrystalPowerBuff::name; }

FrostCrystalPowerBuff::~FrostCrystalPowerBuff()
{
    EnergyRevertAction::deleteListener(this->getBuffID());
    EnergyConsumeAction::deleteListener(this->getBuffID());
}

// 涌能法则
std::string EnergySurgeLawBuff::name = "EnergySurgeLawBuff";

EnergySurgeLawBuff::EnergySurgeLawBuff(Person *p, double)
    : Buff(p)
{
    this->number = 0.08; // 增加的元素增伤
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &info)
        { this->listenerCallback(info); });
    AttackAction::addListener(std::move(info));
}

void EnergySurgeLawBuff::listenerCallback(DamageInfo &info)
{
    if (info.skillName == IceArrow_Beam::name)
    {
        this->count += 2;
    }

    // 触发50支冰箭使得下次冻结寒风伤害*3，获得涌能法则回能buff
    if (info.skillName == FrostWind::name && this->count >= 50)
    {
        info.damageNum *= 3;
        this->p->triggerAction<CreateBuffAction>(0, EnergyRevertBuff_EnergySurgeLaw::name);
        this->count -= 50;
        Logger::debugBuff(AutoAttack::getTimer(),
                          this->getBuffName(),
                          "triggered");
    }
}

void EnergySurgeLawBuff::update(const double) {}

bool EnergySurgeLawBuff::shouldBeRemoved() { return this->duration < 0; }
std::string EnergySurgeLawBuff::getBuffName() const { return EnergySurgeLawBuff::name; }

EnergySurgeLawBuff::~EnergySurgeLawBuff()
{
    AttackAction::deleteListener(this->getBuffID());
}

// 能量回复（用于涌能法则）
std::string EnergyRevertBuff_EnergySurgeLaw::name = "EnergyRevertBuff_EnergySurgeLaw";

EnergyRevertBuff_EnergySurgeLaw::EnergyRevertBuff_EnergySurgeLaw(Person *p, double)
    : Buff(p)
{
    this->number = 0.003; // 回复能量比率
    this->duration = 1000;
    this->maxDuration = this->duration;

    this->triggerInterval = 10;
    this->triggerInterval /= (1 + this->p->getCastingSpeed());
}

void EnergyRevertBuff_EnergySurgeLaw::update(const double deltaTime)
{
    this->timer += deltaTime;
    if (this->timer >= this->triggerInterval)
    {
        this->p->triggerAction<EnergyRevertAction_Beam>(this->number * this->p->getMaxEnergy());
        Logger::debugBuff(AutoAttack::getTimer(),
                          this->getBuffName(),
                          "triggered");
        this->timer -= triggerInterval;
    }
}

bool EnergyRevertBuff_EnergySurgeLaw::shouldBeRemoved() { return this->duration < 0; }
std::string EnergyRevertBuff_EnergySurgeLaw::getBuffName() const { return EnergyRevertBuff_EnergySurgeLaw::name; }

// 大招冰伤
std::string UltiIncreaseBuff_Beam::name = "UltiIncreaseBuff_Beam";

UltiIncreaseBuff_Beam::UltiIncreaseBuff_Beam(Person *p, double) : Buff(p)
{
    this->number = 0.45; // 冰伤
    this->duration = 1000;
    this->maxDuration = this->duration;
    this->triggerInterval = 10;

    this->p->triggerAction<ElementIncreaseModifyAction>(this->number);
}

void UltiIncreaseBuff_Beam::listenerCallback(double) {}
void UltiIncreaseBuff_Beam::update(const double deltaTime)
{
    this->timer += deltaTime;
    if (this->timer >= this->triggerInterval)
    {
        this->p->triggerAction<EnergyRevertAction_Beam>(2.2);
        this->timer -= this->triggerInterval;
    }
}

bool UltiIncreaseBuff_Beam::shouldBeRemoved() { return this->duration < 0; }
std::string UltiIncreaseBuff_Beam::getBuffName() const { return UltiIncreaseBuff_Beam::name; }

UltiIncreaseBuff_Beam::~UltiIncreaseBuff_Beam()
{
    this->p->triggerAction<ElementIncreaseModifyAction>(-this->number);
}

// 装备套装效果
std::string EquipmentSetEffectBuff_Beam::name = "EquipmentSetEffectBuff_Beam";

EquipmentSetEffectBuff_Beam::EquipmentSetEffectBuff_Beam(Person *p, double) : Buff(p)
{
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    auto info = std::make_unique<CreateSkillListener>(
        this->getBuffID(), [this](Skill *const skill)
        { this->listenerCallback(skill); });
    CreateSkillAction::addListener(std::move(info));

    p->triggerAction<ProficientPercentModifyAction>(0.1);
}

void EquipmentSetEffectBuff_Beam::listenerCallback(Skill *const skill)
{
    if (skill->getSkillName() == IceArrow_Beam::name || skill->getSkillName() == FrostBurst::name)
    {
        skill->damageIncreaseAdd += 0.15;
    }
    if (skill->getSkillName() == Beam::name)
    {
        skill->damageIncreaseAdd += 0.15;
    }
}

void EquipmentSetEffectBuff_Beam::update(const double) {}

bool EquipmentSetEffectBuff_Beam::shouldBeRemoved() { return this->duration < 0; }
std::string EquipmentSetEffectBuff_Beam::getBuffName() const { return EquipmentSetEffectBuff_Beam::name; }

EquipmentSetEffectBuff_Beam::~EquipmentSetEffectBuff_Beam()
{
    CreateSkillAction::deleteListener(this->getBuffID());
    this->p->triggerAction<ProficientPercentModifyAction>(-0.1);
}

// 射线心相仪：迷幻梦境（征服者）
std::string IllusoryDreamBuff::name = "IllusoryDreamBuff";

IllusoryDreamBuff::IllusoryDreamBuff(Person *p, double) : Buff(p)
{
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;
    this->triggerCount = 12;
    this->simulateAttackTriggerInterval = 50;

    // 无视防御效果，粗记为2%
    this->p->chanageDamageReduce(-0.02);
    // 精炼攻击增加10%
    this->number = this->p->getRefineATK();
    this->p->triggerAction<RefineATKCountModifyAction>(this->number * 0.1);

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &info)
        { this->listenerCallback(info); });
    AttackAction::addListener(std::move(info));
}

void IllusoryDreamBuff::listenerCallback(DamageInfo &info)
{
    if (info.skillName == Beam::name)
    {
        this->count++;
    }
}

void IllusoryDreamBuff::update(const double deltaTime)
{
    if (this->p->getNowReleasingSkill() == nullptr)
    {
        this->timer += deltaTime;
    }
    else
    {
        this->timer = 0;
    }

    if (this->timer >= this->simulateAttackTriggerInterval)
    {
        this->count++;
        this->timer -= this->simulateAttackTriggerInterval;
    }

    if (this->count >= this->triggerCount)
    {
        this->p->triggerAction<CreateBuffAction>(1, ATKIncreaseBuff_IllusoryDream::name);
        this->count = 0;
        Logger::debugBuff(AutoAttack::getTimer(),
                          this->getBuffName(),
                          " triggered ");
    }
}

bool IllusoryDreamBuff::shouldBeRemoved() { return this->duration < 0; }
std::string IllusoryDreamBuff::getBuffName() const { return IllusoryDreamBuff::name; }

IllusoryDreamBuff::~IllusoryDreamBuff()
{
    this->p->triggerAction<RefineATKCountModifyAction>(this->number * -0.1);
    AttackAction::deleteListener(this->getBuffID());
}

// 攻击力增加（用于迷幻梦境）
std::string ATKIncreaseBuff_IllusoryDream::name = "ATKIncreaseBuff_IllusoryDream";

ATKIncreaseBuff_IllusoryDream::ATKIncreaseBuff_IllusoryDream(Person *p, double n) : Buff(p)
{
    this->number = 50;
    this->duration = 800;
    this->maxDuration = this->duration;
    this->isStackable = true;
    this->maxStack = 13;
    this->stack = n;

    this->p->triggerAction<AttackCountModifyAction>(this->number * this->stack);
}

void ATKIncreaseBuff_IllusoryDream::update(const double)
{
    // 如果出现了层数变动
    if (this->lastStack != this->stack)
    {
        this->p->triggerAction<AttackCountModifyAction>(this->number * (this->stack - this->lastStack));
        this->lastStack = this->stack;
        Logger::debugBuff(AutoAttack::getTimer(),
                        this->getBuffName(),
                        "stack change and calculate increase again, lastStack: " + std::to_string(this->lastStack) + ", stack: " + std::to_string(this->stack));
    }
}

bool ATKIncreaseBuff_IllusoryDream::shouldBeRemoved() { return this->duration < 0; }
std::string ATKIncreaseBuff_IllusoryDream::getBuffName() const { return ATKIncreaseBuff_IllusoryDream::name; }

ATKIncreaseBuff_IllusoryDream::~ATKIncreaseBuff_IllusoryDream()
{
    this->p->triggerAction<AttackCountModifyAction>(-this->number * this->stack);
}

// 浮动额外副属性百分比
std::string FloatingExtraSecondaryAttributesBuff_Beam::name = "FloatingExtraSecondaryAttributesBuff_Beam";

FloatingExtraSecondaryAttributesBuff_Beam::FloatingExtraSecondaryAttributesBuff_Beam(Person *p, double)
    : Buff(p)
{
    this->number = 0.035; // 百分比
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    // 寻找属性最大值
    double temp = 0;
    if (this->p->getCriticalCount() > temp)
    {
        this->lastAttribute = secondaryAttributesEnum::CRITICAL;
        temp = this->p->getCriticalCount();
    }
    if (this->p->getQuicknessCount() > temp)
    {
        this->lastAttribute = secondaryAttributesEnum::QUICKNESS;
        temp = this->p->getQuicknessCount();
    }
    if (this->p->getLuckyCount() > temp)
    {
        this->lastAttribute = secondaryAttributesEnum::LUCKY;
        temp = this->p->getLuckyCount();
    }
    if (this->p->getProficientCount() > temp)
    {
        this->lastAttribute = secondaryAttributesEnum::PROFICIENT;
        temp = this->p->getProficientCount();
    }
    if (this->p->getAlmightyCount() > temp)
    {
        this->lastAttribute = secondaryAttributesEnum::ALMIGHTY;
        temp = this->p->getAlmightyCount();
    }
    switch (lastAttribute)
    {
    case secondaryAttributesEnum::CRITICAL:
        this->p->triggerAction<CriticalPercentModifyAction>(this->number);
        this->p->triggerAction<CriticalCountModifyAction>(2000);
        break;
    case secondaryAttributesEnum::QUICKNESS:
        this->p->triggerAction<QuicknessPercentModifyAction>(this->number);
        this->p->triggerAction<QuicknessCountModifyAction>(2000);
        break;
    case secondaryAttributesEnum::LUCKY:
        this->p->triggerAction<LuckyPercentModifyAction>(this->number);
        this->p->triggerAction<LuckyCountModifyAction>(2000);
        break;
    case secondaryAttributesEnum::PROFICIENT:
        this->p->triggerAction<ProficientPercentModifyAction>(this->number);
        this->p->triggerAction<ProficientCountModifyAction>(2000);
        break;
    case secondaryAttributesEnum::ALMIGHTY:
        this->p->triggerAction<AlmightyPercentModifyAction>(this->number);
        this->p->triggerAction<AlmightyCountModifyAction>(2000);
        break;
    default:
        break;
    }

    auto info1 = std::make_unique<SecondaryAttributeListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback(n); });
    CriticalCountModifyAction::addListener(std::move(info1));
    auto info2 = std::make_unique<SecondaryAttributeListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback(n); });
    QuicknessCountModifyAction::addListener(std::move(info2));
    auto info3 = std::make_unique<SecondaryAttributeListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback(n); });
    LuckyCountModifyAction::addListener(std::move(info3));
    auto info4 = std::make_unique<SecondaryAttributeListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback(n); });
    ProficientCountModifyAction::addListener(std::move(info4));
    auto info5 = std::make_unique<SecondaryAttributeListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback(n); });
    AlmightyCountModifyAction::addListener(std::move(info5));
}

void FloatingExtraSecondaryAttributesBuff_Beam::listenerCallback(double n)
{
    switch (lastAttribute)
    {
    case secondaryAttributesEnum::CRITICAL:
        this->p->triggerAction<CriticalPercentModifyAction>(-this->number);
        this->p->changeCriticalCount(-2000);
        break;
    case secondaryAttributesEnum::QUICKNESS:
        this->p->triggerAction<QuicknessPercentModifyAction>(-this->number);
        this->p->changeQuicknessCount(-2000);
        break;
    case secondaryAttributesEnum::LUCKY:
        this->p->triggerAction<LuckyPercentModifyAction>(-this->number);
        this->p->changeLuckyCount(-2000);
        break;
    case secondaryAttributesEnum::PROFICIENT:
        this->p->triggerAction<ProficientPercentModifyAction>(-this->number);
        this->p->changeProficientCount(-2000);
        break;
    case secondaryAttributesEnum::ALMIGHTY:
        this->p->triggerAction<AlmightyPercentModifyAction>(-this->number);
        this->p->changeAlmightyCount(-2000);
        break;
    default:
        break;
    }

    // 寻找属性最大值
    double temp = 0;
    if (this->p->getCriticalCount() > temp)
    {
        this->lastAttribute = secondaryAttributesEnum::CRITICAL;
        temp = this->p->getCriticalCount();
    }
    if (this->p->getQuicknessCount() > temp)
    {
        this->lastAttribute = secondaryAttributesEnum::QUICKNESS;
        temp = this->p->getQuicknessCount();
    }
    if (this->p->getLuckyCount() > temp)
    {
        this->lastAttribute = secondaryAttributesEnum::LUCKY;
        temp = this->p->getLuckyCount();
    }
    if (this->p->getProficientCount() > temp)
    {
        this->lastAttribute = secondaryAttributesEnum::PROFICIENT;
        temp = this->p->getProficientCount();
    }
    if (this->p->getAlmightyCount() > temp)
    {
        this->lastAttribute = secondaryAttributesEnum::ALMIGHTY;
        temp = this->p->getAlmightyCount();
    }
    switch (lastAttribute)
    {
    case secondaryAttributesEnum::CRITICAL:
        this->p->triggerAction<CriticalPercentModifyAction>(this->number);
        //this->p->triggerAction<CriticalCountModifyAction>(2000);
        this->p->changeCriticalCount(2000);
        break;
    case secondaryAttributesEnum::QUICKNESS:
        this->p->triggerAction<QuicknessPercentModifyAction>(this->number);
        //this->p->triggerAction<QuicknessCountModifyAction>(2000);
        this->p->changeQuicknessCount(2000);
        break;
    case secondaryAttributesEnum::LUCKY:
        this->p->triggerAction<LuckyPercentModifyAction>(this->number);
        //this->p->triggerAction<LuckyCountModifyAction>(2000);
        this->p->changeLuckyCount(2000);
        break;
    case secondaryAttributesEnum::PROFICIENT:
        this->p->triggerAction<ProficientPercentModifyAction>(this->number);
        //this->p->triggerAction<ProficientCountModifyAction>(2000);
        this->p->changeProficientCount(2000);
        break;
    case secondaryAttributesEnum::ALMIGHTY:
        this->p->triggerAction<AlmightyPercentModifyAction>(this->number);
        //this->p->triggerAction<AlmightyCountModifyAction>(2000);
        this->p->changeAlmightyCount(2000);
        break;
    default:
        break;
    }

    Logger::debugBuff(AutoAttack::getTimer(),
                      this->getBuffName(),
                      "secondary attributes floated");
}

void FloatingExtraSecondaryAttributesBuff_Beam::update(const double deltaTime) {}
bool FloatingExtraSecondaryAttributesBuff_Beam::shouldBeRemoved() { return this->duration < 0; }
std::string FloatingExtraSecondaryAttributesBuff_Beam::getBuffName() const { return FloatingExtraSecondaryAttributesBuff_Beam::name; }

FloatingExtraSecondaryAttributesBuff_Beam::~FloatingExtraSecondaryAttributesBuff_Beam()
{
    CriticalCountModifyAction::deleteListener(this->getBuffID());
    QuicknessCountModifyAction::deleteListener(this->getBuffID());
    LuckyCountModifyAction::deleteListener(this->getBuffID());
    ProficientCountModifyAction::deleteListener(this->getBuffID());
    AlmightyCountModifyAction::deleteListener(this->getBuffID());
}

// 冰令脉冲
std::string FrostDecreePulseBuff::name = "FrostDecreePulseBuff";

FrostDecreePulseBuff::FrostDecreePulseBuff(Person *p, double n) : Buff(p)
{
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;
    this->triggerCount = 50;

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo& info)
        { this->listenerCallback(info); });
    AttackAction::addListener(std::move(info));
}

void FrostDecreePulseBuff::listenerCallback(DamageInfo& info)
{
    if(info.skillName == Beam::name)
    {
        this->count += 1;
    }
    if(this->count >= this->triggerCount)
    {
        this->p->triggerAction<CreateSkillAction>(0,FrostDecreePulse::name);
        this->count = 0;
    }
}

void FrostDecreePulseBuff::update(const double) {}
bool FrostDecreePulseBuff::shouldBeRemoved() { return this->duration < 0; }
std::string FrostDecreePulseBuff::getBuffName() const { return FrostDecreePulseBuff::name; }

FrostDecreePulseBuff::~FrostDecreePulseBuff()
{
    AttackAction::deleteListener(this->getBuffID());
}

// 冷却瞬息
std::string InstantCooldownBuff_Beam::name = "InstantCooldownBuff_Beam";

InstantCooldownBuff_Beam::InstantCooldownBuff_Beam(Person *p, double n) : Buff(p)
{
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;
    this->triggerNum = 35;

    auto info = std::make_unique<EnergyListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback(n); });
    EnergyRevertAction::addListener(std::move(info));
}

void InstantCooldownBuff_Beam::listenerCallback(double n)
{
    this->count += n;
    if(this->count >= this->triggerNum)
    {
        this->p->triggerAction<CDReduceAction>(1.3,FrostWind::name);
        this->count -= triggerNum;
    }
}

void InstantCooldownBuff_Beam::update(const double) {}
bool InstantCooldownBuff_Beam::shouldBeRemoved() { return this->duration < 0; }
std::string InstantCooldownBuff_Beam::getBuffName() const { return InstantCooldownBuff_Beam::name; }

InstantCooldownBuff_Beam::~InstantCooldownBuff_Beam()
{
    EnergyRevertAction::deleteListener(this->getBuffID());
}

// 广域冰箭
std::string ExtensiveArrow::name = "ExtensiveArrow";

ExtensiveArrow::ExtensiveArrow(Person *p, double n) : Buff(p)
{
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;
    this->triggerNum = 10;       // 专精技能10次伤害触发1次冰箭
    this->triggerInterval = 50; // 0.5s冷却，防止高频触发

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo& info)
        { this->listenerCallback(info); });
    AttackAction::addListener(std::move(info));
}

void ExtensiveArrow::listenerCallback(DamageInfo& info)
{
    if(info.skillName == WaterSpout::name 
        || info.skillName == FrostWind::name || info.skillName == Vortex::name)
    {
        this->count += 1;
    }
    if(this->count >= this->triggerNum && this->timer >= this->triggerInterval)
    {
        this->count -= this->triggerNum;
        this->timer = 0;
        this->p->triggerAction<CreateSkillAction>(0,IceArrow_Beam::name);
        Logger::debugBuff(AutoAttack::getTimer(),
                          this->getBuffName(),
                          "ice arrow triggered by ExtensiveArrow");
    }
}

void ExtensiveArrow::update(const double deltaTime)
{
    this->timer += deltaTime;
}

bool ExtensiveArrow::shouldBeRemoved() { return this->duration < 0; }
std::string ExtensiveArrow::getBuffName() const { return ExtensiveArrow::name; }

ExtensiveArrow::~ExtensiveArrow()
{
    AttackAction::deleteListener(this->getBuffID());
}

// 龙卷真实因子
std::string WaterSpoutRealBuff::name = "WaterSpoutRealBuff";

WaterSpoutRealBuff::WaterSpoutRealBuff(Person *p, double n) : RealFactor(p)
{
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;
    this->triggerNum = 720;
    this->number = 0.0573; // 龙卷伤害增加

    auto info = std::make_unique<FactorEnergyListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback(n); });
    AddFactorEnergyAction::addListener(std::move(info));

    auto info2 = std::make_unique<CreateSkillListener>(
        this->getBuffID(), [this](Skill *const skill)
        { this->listenerCallback2(skill); });
    CreateSkillAction::addListener(std::move(info2));
}

void WaterSpoutRealBuff::listenerCallback(double n)
{
    this->changeEnergy(n);
    if(this->presentEnergy >= this->triggerNum)
    {
        this->canTrigger = true;
    }
}

void WaterSpoutRealBuff::listenerCallback2(Skill* const skill)
{
    if(skill->getSkillName() == WaterSpout::name && this->canTrigger)
    {
        //this->p->triggerAction<CDRefreshAction>(0,WaterSpout::name);
        skill->damageTriggerInterval /= 2;
        skill->dreamIncreaseAdd += this->number / 2;
        this->changeEnergy(-this->triggerNum);
        this->canTrigger = false;
    }
}

void WaterSpoutRealBuff::update(const double) {}
bool WaterSpoutRealBuff::shouldBeRemoved() { return this->duration < 0; }
std::string WaterSpoutRealBuff::getBuffName() const { return WaterSpoutRealBuff::name; }

WaterSpoutRealBuff::~WaterSpoutRealBuff()
{
    AddFactorEnergyAction::deleteListener(this->getBuffID());
    CreateSkillAction::deleteListener(this->getBuffID());
}

// 9冰真实因子
std::string IceRealBuff::name = "IceRealBuff";

IceRealBuff::IceRealBuff(Person *p, double n) : RealFactor(p)
{
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;
    this->triggerNum = 35;

    auto info = std::make_unique<FactorEnergyListener>(
        this->getBuffID(), [this](double n)
        { this->listenerCallback(n); });
    AddFactorEnergyAction::addListener(std::move(info));
}

void IceRealBuff::listenerCallback(double n)
{
    this->changeEnergy(n);
    if(this->getPresentEnergy() >= this->triggerNum)
    {
        this->p->triggerAction<CreateBuffAction>(0, NineIceBuff::name);
        this->changeEnergy(-this->triggerNum);
    }
}

void IceRealBuff::update(const double) {}
bool IceRealBuff::shouldBeRemoved() { return this->duration < 0; }
std::string IceRealBuff::getBuffName() const { return IceRealBuff::name; }

IceRealBuff::~IceRealBuff()
{
    AddFactorEnergyAction::deleteListener(this->getBuffID());
}

// 9冰
std::string NineIceBuff::name = "NineIceBuff";

NineIceBuff::NineIceBuff(Person *p, double n) : Buff(p)
{
    this->duration = 1500;
    this->number = 0.0161; // 9冰能量减少
    this->maxDuration = this->duration;

    p->maxResourceNum += 3;

    auto info2 = std::make_unique<CreateSkillListener>(
        this->getBuffID(), [this](Skill *const skill)
        { this->listenerCallback(skill); });
    CreateSkillAction::addListener(std::move(info2));
}

void NineIceBuff::listenerCallback(Skill* const skill)
{
    if(skill->getSkillName() != Beam::name)
        return;
    skill->changeEnergyReduceDOWN(this->number * 9);
    Logger::debugBuff(AutoAttack::getTimer(),
                      this->getBuffName(),
                      " triggered, skill:" + skill->getSkillName() +  ", energyReduceDOWN: " + std::to_string(skill->getEnergyReduceDOWN()));
}

void NineIceBuff::update(const double) {}
bool NineIceBuff::shouldBeRemoved() { return this->duration < 0; }
std::string NineIceBuff::getBuffName() const { return NineIceBuff::name; }

NineIceBuff::~NineIceBuff()
{
    CreateSkillAction::deleteListener(this->getBuffID());
    this->p->maxResourceNum -= 3;
    if(this->p->resourceNum > this->p->maxResourceNum)
    {
        this->p->resourceNum = this->p->maxResourceNum;
    }
}

// 冰箭幸运真实因子
std::string IceArrowLuckyRealBuff::name = "IceArrowLuckyRealBuff";

IceArrowLuckyRealBuff::IceArrowLuckyRealBuff(Person *p, double n) : RealFactor(p)
{
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    auto info1 = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo& info)
        { this->listenerCallback(info); });
    AttackAction::addListener(std::move(info1));

    auto info2 = std::make_unique<CreateSkillListener>(
        this->getBuffID(), [this](Skill *const skill)
        { this->listenerCallback2(skill); });
    CreateSkillAction::addListener(std::move(info2));
}

void IceArrowLuckyRealBuff::listenerCallback(DamageInfo& info)
{
    // 灌注期伤害增加+幸运最终伤害增加
    // 默认灌注期全程触发
    double floodIncrease = 0.33;
    double finalIncrease = 0.505;

    info.luckyNum *= (1 + finalIncrease);

    if(this->p->findBuffInBuffList(FloodBuff_Beam::name) == -1)
        return;
    
    info.luckyNum *= (1 + floodIncrease);
}

void IceArrowLuckyRealBuff::listenerCallback2(Skill *const skill)
{
    if(skill == nullptr)
        return;
    if(this->p->findBuffInBuffList(FloodBuff_Beam::name) == -1)
        return;
    if(skill->getSkillName() == IceArrow_Beam::name)
    {
        skill->setCanTriggerLucky(true);
    }
}

void IceArrowLuckyRealBuff::update(const double) {}
bool IceArrowLuckyRealBuff::shouldBeRemoved() { return this->duration < 0; }
std::string IceArrowLuckyRealBuff::getBuffName() const { return IceArrowLuckyRealBuff::name; }

IceArrowLuckyRealBuff::~IceArrowLuckyRealBuff()
{
    AttackAction::deleteListener(this->getBuffID());
    CreateSkillAction::deleteListener(this->getBuffID());
}

// 无尽思维
std::string InfiniteMindBuff::name = "InfiniteMindBuff";

InfiniteMindBuff::InfiniteMindBuff(Person *p, double n) : Buff(p)
{
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;


    this->p->triggerAction<PrimaryAttributesCountModifyAction>(150);
    // 专精技能cd缩短10%
    for(auto& skill : p->getSkillCDListRef())
    {
        for(auto& i : skill->getSkillType())
        {
            if(i == Skill::skillTypeEnum::SPECIALIZED)
            {
                skill->MaxCD *= 0.9;
                break;
            }
        }
    }

    p->triggerAction<ProficientPercentModifyAction>(0.08);
}

void InfiniteMindBuff::update(const double) 
{
    static bool triggered = false;
    if(this->p->findBuffInBuffList(UltiIncreaseBuff_Beam::name) != -1 && !triggered)
    {
        this->p->triggerAction<ProficientPercentModifyAction>(0.08);
        triggered = true;
    }
    if(this->p->findBuffInBuffList(UltiIncreaseBuff_Beam::name) == -1 && triggered)
    {
        this->p->triggerAction<ProficientPercentModifyAction>(-0.08);
        triggered = false;
    }
}
bool InfiniteMindBuff::shouldBeRemoved() { return this->duration < 0; }
std::string InfiniteMindBuff::getBuffName() const { return InfiniteMindBuff::name; }
InfiniteMindBuff::~InfiniteMindBuff()
{
    this->p->triggerAction<ProficientPercentModifyAction>(-0.08);
    this->p->triggerAction<PrimaryAttributesCountModifyAction>(-150);
}

// 职业专属因子（G7）
// 极性：智力、
// 灵感：射线、灌注、冰箭
std::string OccupationalFactorBuff_Beam::name = "OccupationalFactorBuff_Beam";

OccupationalFactorBuff_Beam::OccupationalFactorBuff_Beam(Person *p, double) : Factor(p)
{
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    auto info = std::make_unique<CreateSkillListener>(
        this->getBuffID(), [this](Skill *const skill)
        { this->listenerCallback(skill); });
    CreateSkillAction::addListener(std::move(info));
}

void OccupationalFactorBuff_Beam::listenerCallback(Skill *const skill)
{
    double iceArrowIncrease = 0.211;
    double beamIncrease = 0.097;
    if (skill->getSkillName() == IceArrow_Beam::name)
    {
        if(this->p->findBuffInBuffList(FloodBuff_Beam::name) != -1)
        {
            this->count += 6;
        }
        else
        {
            this->count += 2;
        }
        if(this->count >= this->arrowTriggerNum)
        {
            this->p->triggerAction<AddFactorEnergyAction>(8);
            this->count -= this->arrowTriggerNum;
            Logger::debugBuff(AutoAttack::getTimer(),
                      this->getBuffName(),
                      "Factor energy reverted: 8");
        }
        skill->dreamIncreaseAdd += iceArrowIncrease;
    }
    if(skill->getSkillName() == FrostBurst::name)
    {
        skill->dreamIncreaseAdd += iceArrowIncrease;
    }
    // if (skill->getSkillName() == WaterSpout::name)
    // {
    //     skill->dreamIncreaseAdd += 0.25;
    // }

    if (skill->getSkillName() == Beam::name)
    {
        skill->dreamIncreaseAdd += beamIncrease;    
    }

    if (skill->getSkillName() == Flood_Beam::name)
    {
        this->p->triggerAction<AddFactorEnergyAction>(400);  
        Logger::debugBuff(AutoAttack::getTimer(),
                      this->getBuffName(),
                      "Factor energy reverted: 400");
    }
}

void OccupationalFactorBuff_Beam::update(const double deltaTime) 
{
    this->timer += deltaTime;
    if(this->timer >= this->beamTriggerInterval)
    {
        this->p->triggerAction<AddFactorEnergyAction>(20);
        this->timer -= this->beamTriggerInterval;
        Logger::debugBuff(AutoAttack::getTimer(),
                      this->getBuffName(),
                      "Factor energy reverted: 20");
    }
}

bool OccupationalFactorBuff_Beam::shouldBeRemoved() { return this->duration < 0; }
std::string OccupationalFactorBuff_Beam::getBuffName() const { return OccupationalFactorBuff_Beam::name; }

OccupationalFactorBuff_Beam::~OccupationalFactorBuff_Beam()
{
    CreateSkillAction::deleteListener(this->getBuffID());
    this->p->triggerAction<PrimaryAttributesPercentModifyAction>(-0.0106);
    this->p->triggerAction<PrimaryAttributesCountModifyAction>(-63);
}

// 幻想冲击
std::string FantasyImpactBuff_Beam::name = "FantasyImpactBuff_Beam";

FantasyImpactBuff_Beam::FantasyImpactBuff_Beam(Person *p, double)
    : Buff(p),
      triggerTimer(0),
      triggerInterval(1000),
      triggerStack(20),
      extremeLuckTriggerStack(10),
      extraTriggerStack(20)
{
    this->stack = 0;
    this->duration = 99999;
    this->maxDuration = this->duration;

    this->isInherent = true;

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &damageInfo)
        { this->listenerCallback(damageInfo); });
    AttackAction::addListener(std::move(info));
}

void FantasyImpactBuff_Beam::listenerCallback(DamageInfo &info)
{
    if (info.isLucky && info.skillName != FantasyImpact_Beam::name)
    { // 如果触发幸运
        this->stack += 1;
        if (this->triggerTimer < this->triggerInterval)
        { // 如果计时器未达到触发间隔
            this->triggerTimer += 30 * AutoAttack::getDeltaTime();
        }
        if (this->stack >= (this->triggerStack + this->extraTriggerStack) &&
            this->triggerTimer >= this->triggerInterval)
        {
            this->p->triggerAction<CreateSkillAction>(0, FantasyImpact_Beam::name);
            // 触发幻想冲击
            // 时阶加伤效果写在对应skill中
            this->triggerTimer = 0;
            this->stack = 0;
        }
        // 极运相关逻辑
        if (static_cast<int>(this->stack) % this->extremeLuckTriggerStack == 0)
        {
            this->p->triggerAction<CreateBuffAction>(0, ExtremeLuckBuff_Beam::name);
        }
    }
}

void FantasyImpactBuff_Beam::update(const double deltaTime)
{
        this->triggerTimer += deltaTime;
}

bool FantasyImpactBuff_Beam::shouldBeRemoved() { return this->duration < 0; }
std::string FantasyImpactBuff_Beam::getBuffName() const { return FantasyImpactBuff_Beam::name; }

FantasyImpactBuff_Beam::~FantasyImpactBuff_Beam()
{
    AttackAction::deleteListener(this->getBuffID());
}

// 极运
std::string ExtremeLuckBuff_Beam::name = "ExtremeLuckBuff_Beam";

ExtremeLuckBuff_Beam::ExtremeLuckBuff_Beam(Person *p, double) : Buff(p)
{
    this->number = 0.1; // 用作增加属性值
    this->stack = 0;
    this->duration = 500;
    this->maxDuration = this->duration;

    this->p->triggerAction<PrimaryAttributesPercentModifyAction>(this->number);
}

void ExtremeLuckBuff_Beam::listenerCallback(DamageInfo&) {}
void ExtremeLuckBuff_Beam::update(double) {}
bool ExtremeLuckBuff_Beam::shouldBeRemoved() { return this->duration < 0; }
std::string ExtremeLuckBuff_Beam::getBuffName() const { return ExtremeLuckBuff_Beam::name; }

ExtremeLuckBuff_Beam::~ExtremeLuckBuff_Beam()
{
    // this->p->changePrimaryAttributesByPersent(-this->number);
    this->p->triggerAction<PrimaryAttributesPercentModifyAction>(-this->number);
}

// 系数调整
std::string CoefficientAdjustmentBuff_Beam::name = "CoefficientAdjustmentBuff_Beam";

CoefficientAdjustmentBuff_Beam::CoefficientAdjustmentBuff_Beam(Person *p, double) : Buff(p)
{
    this->isInherent = true;
    this->duration = 999999;
    this->maxDuration = this->duration;

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &damageInfo)
        { this->listenerCallback(damageInfo); });
    AttackAction::addListener(std::move(info));
}

void CoefficientAdjustmentBuff_Beam::listenerCallback(DamageInfo& info) 
{
    double fantasyImpactAdjustment = 1;
    double luckyAdjustment = 2.1;
    if(info.skillName == FantasyImpact_Beam::name)
    {
        info.damageNum *= fantasyImpactAdjustment;
    }
    info.luckyNum *= luckyAdjustment;
}

void CoefficientAdjustmentBuff_Beam::update(double) {}
bool CoefficientAdjustmentBuff_Beam::shouldBeRemoved() { return this->duration < 0; }
std::string CoefficientAdjustmentBuff_Beam::getBuffName() const { return CoefficientAdjustmentBuff_Beam::name; }

CoefficientAdjustmentBuff_Beam::~CoefficientAdjustmentBuff_Beam() 
{
    AttackAction::deleteListener(this->getBuffID());
}