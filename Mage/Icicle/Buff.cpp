#include "Buff.h"
#include "Skill.h"
#include "Action.h"
#include "../../core/Person.h"
#include "../../core/AutoAttack.h"
#include "../../core/Listener.hpp"
#include "../../core/Action.h"
#include "../../core/Logger.h"
#include <string>

// 冰矛暴伤
std::string SpearCritialBuff::name = "SpearCritialBuff";

SpearCritialBuff::SpearCritialBuff(Person *p) : Buff(p)
{
    this->number = 0.015; // 用作每层给予的爆伤数值
    this->maxStack = 10;
    this->duration = 600;
    this->maxDuration = this->duration;
    this->isStackable = true;

    // 构造buff时同步链接监听
    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &damageInfo)
        { this->listenerCallback(damageInfo); });

    AttackAction::addListener(std::move(info));
}

SpearCritialBuff::SpearCritialBuff(Person *p, const double n) : Buff(p)
{
    this->stack = n;
    this->maxStack = 10;
    this->number = 0.015;
    this->duration = 1000;
    this->maxDuration = this->duration;
    this->isStackable = true;

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &damageInfo)
        { this->listenerCallback(damageInfo); });

    AttackAction::addListener(std::move(info));
}

void SpearCritialBuff::listenerCallback(DamageInfo &info)
{
    if (info.isCritical &&
        (info.skillName == Spear::name || info.skillName == PierceSpear::name))
    {
        this->addStack(1);

        // 根据层数重置爆伤增加值，并重新赋值
        // this->p->changeCriticalDamage(-(this->lastStack * this->number));
        this->p->triggerAction<CriticalDamageModifyAction>(-(this->lastStack * this->number));
        // this->p->changeCriticalDamage(this->stack * this->number);
        this->p->triggerAction<CriticalDamageModifyAction>(this->stack * this->number);

        this->resetDuration();
        Logger::debugBuff(AutoAttack::getTimer(),
                          this->getBuffName(),
                          " - Stack - " +
                              std::to_string(this->stack) +
                              " / " +
                              std::to_string(this->maxStack));
    }
}

void SpearCritialBuff::update(const double) {}
bool SpearCritialBuff::shouldBeRemoved() { return this->duration < 0; }
std::string SpearCritialBuff::getBuffName() const { return SpearCritialBuff::name; }

SpearCritialBuff::~SpearCritialBuff()
{
    // 析构时同步删除监听
    AttackAction::deleteListener(this->getBuffID());
}

// 冰矛暴击回复玄冰
std::string SpearCritialToRevertIceBuff::name = "SpearCritialToRevertIceBuff";

SpearCritialToRevertIceBuff::SpearCritialToRevertIceBuff(Person *p, double)
    : Buff(p)
{
    this->number = 0.3; // 用作触发概率
    this->duration = 99999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &damageInfo)
        { this->listenerCallback(damageInfo); });

    AttackAction::addListener(std::move(info));
}

void SpearCritialToRevertIceBuff::listenerCallback(
    DamageInfo &info) const
{
    if (info.isCritical && (info.skillName == Spear::name))
    {
        if (this->p->isSuccess(this->number))
        {
            Logger::debugBuff(AutoAttack::getTimer(),
                              this->getBuffName(),
                              " - triggered ");
            this->p->triggerAction<CreateSkillAction>(0, IceArrow_Icicle::name);
        }
    }
}

void SpearCritialToRevertIceBuff::update(const double) {}
bool SpearCritialToRevertIceBuff::shouldBeRemoved() { return false; }
std::string SpearCritialToRevertIceBuff::getBuffName() const { return SpearCritialToRevertIceBuff::name; }

SpearCritialToRevertIceBuff::~SpearCritialToRevertIceBuff()
{
    AttackAction::deleteListener(this->getBuffID());
}

// 玄冰计数
std::string IceCountBuff::name = "IceCountBuff";

IceCountBuff::IceCountBuff(Person *p) : Buff(p)
{
    this->maxStack = 99999;
    this->duration = 99999;
    this->maxDuration = this->duration;
    this->isStackable = true;
    this->isInherent = true;

    auto info = std::make_unique<ResourceListener>(
        this->getBuffID(), [this](const double n)
        { this->listenerCallback(n); });

    ResourceConsumeAction::addListener(std::move(info));
}

IceCountBuff::IceCountBuff(Person *p, double n) : Buff(p)
{
    this->stack = n;
    this->maxStack = 99999;
    this->duration = 99999;
    this->maxDuration = this->duration;
    this->isStackable = true;
    this->isInherent = true;

    auto info = std::make_unique<ResourceListener>(
        this->getBuffID(), [this](const double n)
        { this->listenerCallback(n); });

    ResourceConsumeAction::addListener(std::move(info));
}

IceCountBuff::~IceCountBuff()
{
    ResourceConsumeAction::deleteListener(this->getBuffID());
}

void IceCountBuff::listenerCallback(const double n) { this->addStack(n); }

void IceCountBuff::update(const double)
{
    if (this->p->findBuffInBuffList<FloodBuff_Icicle>() == -1 && this->stack >= 20)
    {
        Logger::debugBuff(AutoAttack::getTimer(),
                          this->getBuffName(),
                          " - triggered ");
        this->p->triggerAction<CreateBuffAction>(this->stack,
                                                 EndlessColdBuff::name);
        this->stack = 0;
    }
}

bool IceCountBuff::shouldBeRemoved() { return false; }
std::string IceCountBuff::getBuffName() const { return IceCountBuff::name; }

// 灌注
std::string FloodBuff_Icicle::name = "FloodBuffBuff_Icicle";

FloodBuff_Icicle::FloodBuff_Icicle(Person *p, double)
    : Buff(p),
      iceNumber(p->getResourceNum())
{
    this->number = 6; // 用作回能量
    this->duration = 1500;
    this->maxDuration = this->duration;

    // p->changeCriticalDamage(0.15);
    this->p->triggerAction<CriticalDamageModifyAction>(0.15);
    p->triggerAction<CriticalPercentModifyAction>(0.03);
    p->elementATK += 100;
    p->triggerAction<CreateBuffAction>(0, DoubleSpearBuff::name);
}

void FloodBuff_Icicle::listenerCallback(double) {}

void FloodBuff_Icicle::update(double)
{
    if (static_cast<int>(duration) % 100 == 0)
    {
        this->p->triggerAction<EnergyRevertAction>(this->number);
    }
}

FloodBuff_Icicle::~FloodBuff_Icicle()
{
    this->p->triggerAction<CriticalDamageModifyAction>(-0.15);
    this->p->triggerAction<CriticalPercentModifyAction>(-0.03);
    this->p->elementATK -= 100;
    this->p->resourceNum = this->iceNumber;
}

bool FloodBuff_Icicle::shouldBeRemoved() { return this->duration < 0; }
std::string FloodBuff_Icicle::getBuffName() const { return FloodBuff_Icicle::name; }

// 双倍冰矛
std::string DoubleSpearBuff::name = "DoubleSpearBuff";

DoubleSpearBuff::DoubleSpearBuff(Person *p, double) : Buff(p)
{
    this->duration = 99999;
    this->maxDuration = this->duration;
}

void DoubleSpearBuff::listenerCallback(double) {}
void DoubleSpearBuff::update(const double) {}
bool DoubleSpearBuff::shouldBeRemoved()
{
    return this->duration < 0 ||
           this->p->findBuffInBuffList(FloodBuff_Icicle::name) == -1;
}
std::string DoubleSpearBuff::getBuffName() const { return DoubleSpearBuff::name; }

// 玄冰回复
std::string IceRevertBuff::name = "IceRevertBuff";

IceRevertBuff::IceRevertBuff(Person *p, double) : Buff(p)
{
    this->number = 1; // 用作每次回复数
    this->duration = 1200;
    this->maxDuration = this->duration;
    this->allowDuplicates = true;
}

void IceRevertBuff::listenerCallback(double) {}

void IceRevertBuff::update(const double)
{
    // 在第一次更新时检查UltiIncreaseBuff
    static bool checked = false;
    if (!checked)
    {
        int a = this->p->findBuffInBuffList(UltiIncreaseBuff_Icicle::name);
        if (a == -1)
        {
            this->duration = 900;
            this->maxDuration = this->duration;
        }
        checked = true;
    }

    if (this->duration > 0)
    {
        if (static_cast<int>(duration) % 300 == 0)
        {
            this->p->triggerAction<ResourceRevertAction>(this->number);
        }
    }
    if (this->duration == 0)
    {
        this->p->triggerAction<ResourceRevertAction>(this->number);
    }
}
bool IceRevertBuff::shouldBeRemoved() { return this->duration < 0; }
std::string IceRevertBuff::getBuffName() const { return IceRevertBuff::name; }

// 能量消耗计数
std::string ConsumedEnergyCountBuff::name = "ConsumedEnergyCountBuff";

// ConsumedEnergyCountBuff::ConsumedEnergyCountBuff(Person *p) : Buff(p)
// {
//     this->number = 100; // 用作减少的冷却数值
//     this->maxStack = 99999;
//     this->duration = 99999;
//     this->maxDuration = this->duration;
//     this->isStackable = true;
//     this->isInherent = true;

//     auto info = std::make_unique<EnergyListener>(
//         this->getBuffID(), [this](double)
//         { this->listenerCallback(0); });
//     EnergyConsumeAction::addListener(std::move(info));
// }

ConsumedEnergyCountBuff::ConsumedEnergyCountBuff(Person *p, const double n)
    : Buff(p)
{
    this->stack = n;
    this->duration = 99999;
    this->maxDuration = this->duration;
    this->isStackable = true;
    this->isInherent = true;

    auto info = std::make_unique<EnergyListener>(
        this->getBuffID(), [this](const double n)
        { this->listenerCallback(n); });
    EnergyConsumeAction::addListener(std::move(info));
}

void ConsumedEnergyCountBuff::listenerCallback(const double n) { this->addStack(n); }

void ConsumedEnergyCountBuff::update(double)
{
    if (static_cast<int>(stack) % 25 == 0 && this->stack != 0)
    {
        this->p->triggerAction<CDReduceAction>(this->number, WaterDrop::name);
        Logger::debugBuff(AutoAttack::getTimer(),
                          this->getBuffName(),
                          " - Stack - " +
                              std::to_string(this->stack) +
                              " / " +
                              std::to_string(this->maxStack));
    }
}

ConsumedEnergyCountBuff::~ConsumedEnergyCountBuff()
{
    EnergyConsumeAction::deleteListener(this->getBuffID());
}

bool ConsumedEnergyCountBuff::shouldBeRemoved() { return this->duration < 0; }
std::string ConsumedEnergyCountBuff::getBuffName() const { return ConsumedEnergyCountBuff::name; }

// 无尽苦寒
std::string EndlessColdBuff::name = "EndlessColdBuff";

EndlessColdBuff::EndlessColdBuff(Person *p, double n) : Buff(p)
{
    this->stack = n;
    this->number = 1.5; // 用作根据玄冰计数层数来对陨星附加的额外增伤
    this->maxStack = 40;
    this->duration = 1000;
    this->maxDuration = this->duration;
    this->isStackable = true;
}

void EndlessColdBuff::listenerCallback(double) {}
void EndlessColdBuff::update(double)
{
    this->p->triggerAction<CDRefreshAction>(0, Meteorite::name);
    // Logger::debugBuff(AutoAttack::getTimer(),
    //                     this->getBuffName(),
    //                     " - triggered ");
}

bool EndlessColdBuff::shouldBeRemoved() { return this->duration < 0; }
std::string EndlessColdBuff::getBuffName() const { return EndlessColdBuff::name; }

// 大招冰伤
std::string UltiIncreaseBuff_Icicle::name = "UltiIncreaseBuff_Icicle";

UltiIncreaseBuff_Icicle::UltiIncreaseBuff_Icicle(Person *p, double) : Buff(p)
{
    this->number = 0.45; // 用于冰伤增加值
    this->duration = 1000;
    this->maxDuration = this->duration;

    this->p->triggerAction<ElementIncreaseModifyAction>(this->number);
}

void UltiIncreaseBuff_Icicle::listenerCallback(double) {}
void UltiIncreaseBuff_Icicle::update(const double) {}
bool UltiIncreaseBuff_Icicle::shouldBeRemoved() { return this->duration < 0; }
std::string UltiIncreaseBuff_Icicle::getBuffName() const { return UltiIncreaseBuff_Icicle::name; }

UltiIncreaseBuff_Icicle::~UltiIncreaseBuff_Icicle()
{
    this->p->triggerAction<ElementIncreaseModifyAction>(-this->number);
}

// 刷新陨星
std::string MeteoriteRefreshBuff::name = "MeteoriteRefreshBuff";

MeteoriteRefreshBuff::MeteoriteRefreshBuff(Person *p, double) : Buff(p)
{
    this->duration = 99999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &damageInfo)
        { this->listenerCallback(damageInfo); });

    AttackAction::addListener(std::move(info));
}

void MeteoriteRefreshBuff::listenerCallback(DamageInfo &info) const
{
    int a = this->p->findSkillInSkillCDList(Meteorite::name);
    if (a != -1)
    {
        if (info.isCritical && info.isLucky &&
            this->p->getSkillCDListRef().at(a)->getStackRef() <
                this->p->getSkillCDListRef().at(a)->getMaxStack() &&
            (info.skillName == Spear::name))
        {
            this->p->triggerAction<CDRefreshAction>(0, Meteorite::name);
            Logger::debugBuff(AutoAttack::getTimer(),
                              this->getBuffName(),
                              " - triggered by skill: " + info.skillName);
            Logger::debugBuff(AutoAttack::getTimer(),
                              this->getBuffName(),
                              " - Meteorite: Stack - " +
                                  std::to_string(
                                      this->p->getSkillCDListRef().at(a)->getStackRef()) +
                                  " / " +
                                  std::to_string(
                                      this->p->getSkillCDListRef().at(a)->getMaxStack()));
        }
    }
}

void MeteoriteRefreshBuff::update(const double) {}
bool MeteoriteRefreshBuff::shouldBeRemoved() { return this->duration < 0; }
std::string MeteoriteRefreshBuff::getBuffName() const { return MeteoriteRefreshBuff::name; }

MeteoriteRefreshBuff::~MeteoriteRefreshBuff()
{
    AttackAction::deleteListener(this->getBuffID());
}

// 冰霜彗星
std::string FrostCometBuff::name = "FrostCometBuff";

FrostCometBuff::FrostCometBuff(Person *p, double) : Buff(p)
{
    this->number = 0.21; // 用作触发概率
    this->duration = 99999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &damageInfo)
        { this->listenerCallback(damageInfo); });
    AttackAction::addListener(std::move(info));
}

void FrostCometBuff::listenerCallback(DamageInfo &info) const
{
    if (info.skillName == Spear::name)
    {
        if (this->p->isSuccess(this->number))
        {
            Logger::debugBuff(AutoAttack::getTimer(),
                              this->getBuffName(),
                              " - triggered by skill: " + info.skillName);
            this->p->triggerAction<CreateSkillAction>(0, FrostComet::name);
        }
    }
}

void FrostCometBuff::update(const double) {}
bool FrostCometBuff::shouldBeRemoved() { return this->duration < 0; }
std::string FrostCometBuff::getBuffName() const { return FrostCometBuff::name; }

FrostCometBuff::~FrostCometBuff()
{
    AttackAction::deleteListener(this->getBuffID());
}

// 陨星连携
std::string MeteoriteSynergyBuff::name = "MeteoriteSynergyBuff";

MeteoriteSynergyBuff::MeteoriteSynergyBuff(Person *p, double) : Buff(p)
{
    this->duration = 99999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &damageInfo)
        { this->listenerCallback(damageInfo); });

    AttackAction::addListener(std::move(info));
}

void MeteoriteSynergyBuff::listenerCallback(DamageInfo &info) const
{
    // 1. 先检查基本条件（不涉及技能列表查找）
    if (!info.isCritical || !info.isLucky)
        return;

    // 2. 检查技能类型（使用安全的字符串比较）
    const std::string &skillName = info.skillName;
    if (skillName != Spear::name)
        return;

    // 3. 查找技能
    int meteoriteIndex = this->p->findSkillInSkillCDList(Meteorite::name);
    if (meteoriteIndex == -1)
        return;

    // 4. 检查技能指针有效性
    auto &meteoriteSkill = this->p->getSkillCDListRef().at(meteoriteIndex);
    if (!meteoriteSkill)
        return;

    // 5. 检查技能状态（层数、冷却等）
    if (meteoriteSkill->getStackRef() < meteoriteSkill->getMaxStack())
        return;

    // 6. 触发创建技能
    this->p->triggerAction<CreateSkillAction>(0, SynergyMeteorite::name);
}

void MeteoriteSynergyBuff::update(const double) {}
bool MeteoriteSynergyBuff::shouldBeRemoved() { return this->duration < 0; }
std::string MeteoriteSynergyBuff::getBuffName() const
{
    return MeteoriteSynergyBuff::name;
}

MeteoriteSynergyBuff::~MeteoriteSynergyBuff()
{
    AttackAction::deleteListener(this->getBuffID());
}

// 平a回冰       //未释放技能时每3秒回1玄冰
std::string SimulateNormalAttackToRevertIceBuff::name =
    "SimulateNormalAttackToRevertIceBuff";

SimulateNormalAttackToRevertIceBuff::SimulateNormalAttackToRevertIceBuff(
    Person *p, double)
    : Buff(p),
      revertTimer(0)
{
    this->number = 400; // 用作触发间隔
    this->duration = 99999;
    this->maxDuration = this->duration;
    this->isInherent = true;
}

void SimulateNormalAttackToRevertIceBuff::listenerCallback(double) {}
bool SimulateNormalAttackToRevertIceBuff::shouldBeRemoved()
{
    return this->duration < 0;
}
std::string SimulateNormalAttackToRevertIceBuff::getBuffName() const
{
    return SimulateNormalAttackToRevertIceBuff::name;
}

void SimulateNormalAttackToRevertIceBuff::update(const double deltaTime)
{
    if (!this->p->getIsReleasingSkill())
    {
        this->revertTimer += deltaTime;
    }
    if (this->revertTimer >= this->number)
    {
        this->p->triggerAction<CreateSkillAction>(0, IceArrow_Icicle::name);
        this->revertTimer = 0;
        Logger::debugBuff(AutoAttack::getTimer(),
                          this->getBuffName(),
                          " triggered ");
    }
}

// 贯穿冰矛
std::string PierceSpearBuff::name = "PierceSpearBuff";

PierceSpearBuff::PierceSpearBuff(Person *p, double) : Buff(p)
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

void PierceSpearBuff::listenerCallback(DamageInfo &info)
{
    if (info.skillName == Spear::name)
    {
        this->stack += 1;
    }
    if (this->stack >= 10)
    {
        this->p->triggerAction<CreateSkillAction>(0, PierceSpear::name);
        this->stack = 0;

        Logger::debugBuff(AutoAttack::getTimer(),
                          this->getBuffName(),
                          " triggered ");
    }
}

void PierceSpearBuff::update(const double) {}

bool PierceSpearBuff::shouldBeRemoved() { return this->duration < 0; }
std::string PierceSpearBuff::getBuffName() const { return PierceSpearBuff::name; }

PierceSpearBuff::~PierceSpearBuff()
{
    AttackAction::deleteListener(this->getBuffID());
}

// 装备套装效果:冰矛
std::string EquipmentSetEffectBuff_Icicle::name = "EquipmentSetEffectBuff_Icicle";

EquipmentSetEffectBuff_Icicle::EquipmentSetEffectBuff_Icicle(Person *p, double) : Buff(p)
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

void EquipmentSetEffectBuff_Icicle::listenerCallback(DamageInfo &info)
{
    if (info.skillName == Spear::name)
    {
        if (info.isCritical && info.isLucky)
        {
            this->stack += 2;
        }
        else if (info.isCritical || info.isLucky)
        {
            this->stack += 1;
        }
    }
}

void EquipmentSetEffectBuff_Icicle::update(const double)
{
    if (this->stack >= 25)
    {
        this->stack -= 25;
        this->p->triggerAction<CreateBuffAction>(0, IceRevertBuff::name);
    }
}

bool EquipmentSetEffectBuff_Icicle::shouldBeRemoved() { return this->duration < 0; }
std::string EquipmentSetEffectBuff_Icicle::getBuffName() const
{
    return EquipmentSetEffectBuff_Icicle::name;
}

EquipmentSetEffectBuff_Icicle::~EquipmentSetEffectBuff_Icicle()
{
    AttackAction::deleteListener(this->getBuffID());
}

// 幻想冲击——冰矛心相仪
std::string FantasyImpactBuff::name = "FantasyImpactBuff";

FantasyImpactBuff::FantasyImpactBuff(Person *p, double)
    : Buff(p),
      triggerTimer(0),
      triggerInterval(1000),
      triggerStack(20),
      extremeLuckTriggerStack(10)
{
    this->stack = 0;
    this->duration = 99999;
    this->maxDuration = this->duration;
    // 时阶 +5s 内置cd
    this->triggerInterval += 500;
    this->isInherent = true;

    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &damageInfo)
        { this->listenerCallback(damageInfo); });
    AttackAction::addListener(std::move(info));
}

void FantasyImpactBuff::listenerCallback(DamageInfo &info)
{
    if (info.isLucky && info.skillName != FantasyImpact::name)
    { // 如果触发幸运
        this->stack += 1;
        if (this->triggerTimer < this->triggerInterval)
        { // 如果计时器未达到触发间隔
            this->triggerTimer += 30 * AutoAttack::getDeltaTime();
        }
        if (static_cast<int>(this->stack) % this->triggerStack == 0 &&
            this->triggerTimer >= this->triggerInterval)
        {
            this->p->triggerAction<CreateSkillAction>(0, FantasyImpact::name);
            // 触发幻想冲击
            // 时阶加伤效果写在对应skill中
            this->triggerTimer -= this->triggerInterval;
        }
        // 极运相关逻辑
        if (static_cast<int>(this->stack) % this->extremeLuckTriggerStack == 0)
        {
            this->p->triggerAction<CreateBuffAction>(0, ExtremeLuckFactor::name);
        }
    }
}

void FantasyImpactBuff::update(const double deltaTime)
{
        this->triggerTimer += deltaTime;
}

bool FantasyImpactBuff::shouldBeRemoved() { return this->duration < 0; }
std::string FantasyImpactBuff::getBuffName() const { return FantasyImpactBuff::name; }

FantasyImpactBuff::~FantasyImpactBuff()
{
    AttackAction::deleteListener(this->getBuffID());
}

// 极运
std::string ExtremeLuckFactor::name = "ExtremeLuckFactor";

ExtremeLuckFactor::ExtremeLuckFactor(Person *p, double) : Factor(p)
{
    this->number = 0.1; // 用作增加属性值
    this->stack = 0;
    this->duration = 500;
    this->maxDuration = this->duration;

    this->p->triggerAction<PrimaryAttributesPercentModifyAction>(this->number);
}

void ExtremeLuckFactor::listenerCallback(DamageInfo&) {}
void ExtremeLuckFactor::update(double) {}
bool ExtremeLuckFactor::shouldBeRemoved() { return this->duration < 0; }
std::string ExtremeLuckFactor::getBuffName() const { return ExtremeLuckFactor::name; }

ExtremeLuckFactor::~ExtremeLuckFactor()
{
    // this->p->changePrimaryAttributesByPersent(-this->number);
    this->p->triggerAction<PrimaryAttributesPercentModifyAction>(-this->number);
}

// 职业因子（数值部分）
std::string OccupationalFactor_Icicle::name = "OccupationalFactor_Icicle";

OccupationalFactor_Icicle::OccupationalFactor_Icicle(Person *p, double) : Factor(p)
{
    this->stack = 0;
    this->duration = 999999;
    this->maxDuration = this->duration;
    this->isInherent = true;

    // 注册攻击事件的回调
    auto attackInfo = std::make_unique<DamageListener>(
        this->getBuffID(), [this](DamageInfo &damageInfo)
        { this->listenerCallback(damageInfo); });
    AttackAction::addListener(std::move(attackInfo));

    // 注册创建技能事件的回调
    auto createSkillInfo = std::make_unique<CreateSkillListener>(
        this->getBuffID(), [this](Skill *const skill)
        { this->listenerCallback2(skill); });
    CreateSkillAction::addListener(std::move(createSkillInfo));
}

void OccupationalFactor_Icicle::listenerCallback(DamageInfo &info)
{
    // 检查当前是否处于灌注期
    int index = p->findBuffInBuffList(FloodBuff_Icicle::name);
    if (index != -1)
    {
        // 幸运伤害增加58.3%
        info.luckyNum *= (1 + 0.583);
    }
}

void OccupationalFactor_Icicle::listenerCallback2(Skill *const skill)
{
    if (skill->getSkillName() == FrostComet::name || skill->getSkillName() == PierceSpear::name)
    {
        skill->dreamIncreaseAdd += 0.35;
    }
}

void OccupationalFactor_Icicle::update(double) {}
bool OccupationalFactor_Icicle::shouldBeRemoved() { return this->duration < 0; }
std::string OccupationalFactor_Icicle::getBuffName() const { return OccupationalFactor_Icicle::name; }

OccupationalFactor_Icicle::~OccupationalFactor_Icicle()
{
    AttackAction::deleteListener(this->getBuffID());
    CreateSkillAction::deleteListener(this->getBuffID());
}

// 冷却瞬息
std::string InstantCooldownBuff_Icicle::name = "InstantCooldownBuff";

InstantCooldownBuff_Icicle::InstantCooldownBuff_Icicle(Person *p, double n) : Buff(p)
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

void InstantCooldownBuff_Icicle::listenerCallback(double n)
{
    this->count += n;
    if(this->count >= this->triggerNum)
    {
        this->p->triggerAction<CDReduceAction>(1.3,Meteorite::name);
        this->count -= triggerNum;
    }
}

void InstantCooldownBuff_Icicle::update(const double) {}
bool InstantCooldownBuff_Icicle::shouldBeRemoved() { return this->duration < 0; }
std::string InstantCooldownBuff_Icicle::getBuffName() const { return InstantCooldownBuff_Icicle::name; }

InstantCooldownBuff_Icicle::~InstantCooldownBuff_Icicle()
{
    EnergyRevertAction::deleteListener(this->getBuffID());
}

// 浮动额外副属性值
std::string FloatingExtraSecondaryAttributesBuff_Icicle::name = "FloatingExtraSecondaryAttributesBuff";

FloatingExtraSecondaryAttributesBuff_Icicle::FloatingExtraSecondaryAttributesBuff_Icicle(Person *p, double)
    : Buff(p)
{
    this->number = 800; // 数值
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
        this->p->changeCriticalCount(this->number);
        break;
    case secondaryAttributesEnum::QUICKNESS:
        this->p->changeQuicknessCount(this->number);
        break;
    case secondaryAttributesEnum::LUCKY:
        this->p->changeLuckyCount(this->number);
        break;
    case secondaryAttributesEnum::PROFICIENT:
        this->p->changeProficientCount(this->number);
        break;
    case secondaryAttributesEnum::ALMIGHTY:
        this->p->changeAlmightyCount(this->number);
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

void FloatingExtraSecondaryAttributesBuff_Icicle::listenerCallback(double n)
{
    switch (lastAttribute)
    {
    case secondaryAttributesEnum::CRITICAL:
        this->p->changeCriticalCount(-this->number);
        break;
    case secondaryAttributesEnum::QUICKNESS:
        this->p->changeQuicknessCount(-this->number);
        break;
    case secondaryAttributesEnum::LUCKY:
        this->p->changeLuckyCount(-this->number);
        break;
    case secondaryAttributesEnum::PROFICIENT:
        this->p->changeProficientCount(-this->number);
        break;
    case secondaryAttributesEnum::ALMIGHTY:
        this->p->changeAlmightyCount(-this->number);
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
        this->p->changeCriticalCount(this->number);
        break;
    case secondaryAttributesEnum::QUICKNESS:
        this->p->changeQuicknessCount(this->number);
        break;
    case secondaryAttributesEnum::LUCKY:
        this->p->changeLuckyCount(this->number);
        break;
    case secondaryAttributesEnum::PROFICIENT:
        this->p->changeProficientCount(this->number);
        break;
    case secondaryAttributesEnum::ALMIGHTY:
        this->p->changeAlmightyCount(this->number);
        break;
    default:
        break;
    }

    Logger::debugBuff(AutoAttack::getTimer(),
                      this->getBuffName(),
                      "secondary attributes floated");
}

void FloatingExtraSecondaryAttributesBuff_Icicle::update(const double deltaTime) {}
bool FloatingExtraSecondaryAttributesBuff_Icicle::shouldBeRemoved() { return this->duration < 0; }
std::string FloatingExtraSecondaryAttributesBuff_Icicle::getBuffName() const { return FloatingExtraSecondaryAttributesBuff_Icicle::name; }

FloatingExtraSecondaryAttributesBuff_Icicle::~FloatingExtraSecondaryAttributesBuff_Icicle()
{
    CriticalCountModifyAction::deleteListener(this->getBuffID());
    QuicknessCountModifyAction::deleteListener(this->getBuffID());
    LuckyCountModifyAction::deleteListener(this->getBuffID());
    ProficientCountModifyAction::deleteListener(this->getBuffID());
    AlmightyCountModifyAction::deleteListener(this->getBuffID());
}