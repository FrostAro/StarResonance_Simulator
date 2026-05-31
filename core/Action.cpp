#include "Action.h"
#include "AutoAttack.h"
#include "Creators.hpp"
#include "Logger.h"
#include "Person.h"
#include <algorithm>
#include <memory>

std::string Action::getActionName() { return "Action"; }

// 攻击事件
std::string AttackAction::name = "AttackAction";

AttackAction::AttackAction(const Skill* const skill)
    : skill(skill) {}

void AttackAction::execute(const double, Person *p)
{
    auto damageInfo = p->Damage(skill);
    // 遍历监听，触发回调
    for (const auto &listener : AttackAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(damageInfo);
        }
    }
    // 加入伤害信息列表
    p->pushDamgeInfo(damageInfo);
}

// 资源消耗
std::string ResourceConsumeAction::name = "ResourceConsumeAction";

void ResourceConsumeAction::execute(const double n, Person *p)
{
    p->consumeResource(static_cast<int>(n));
    // 遍历监听，触发回调
    for (const auto &listener : ResourceConsumeAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 玄冰回复
std::string ResourceRevertAction::name = "ResourceRevertAction";

void ResourceRevertAction::execute(const double n, Person *p)
{
    p->revertResource(static_cast<int>(n));
    Logger::debugAction(AutoAttack::getTimer(),
                        this->getActionName(),
                        "ResourceRevertAction executed, resourceNumber: " +
                            std::to_string(p->resourceNum));
    // 遍历监听，触发回调
    for (const auto &listener : ResourceRevertAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 能量消耗
std::string EnergyConsumeAction::name = "EnergyConsumeAction";

void EnergyConsumeAction::execute(double n, Person *p)
{
    if(skill)
    {
        n *= (1 - p->energyReduceDOWN - this->skill->getEnergyReduceDOWN()) 
                * (1 + p->energyReduceUP + this->skill->getEnergyReduceUP());
    }
    else{
        n *= (1 - p->energyReduceDOWN) * (1 + p->energyReduceUP);
    }
    p->consumeEnergy(n);
    Logger::debugAction(AutoAttack::getTimer(), 
                                this->getActionName(), 
                                "triggered, consumed energy: " + std::to_string(n)
                                        + ", currentEnengy:" + std::to_string(p->present_energy));
    Logger::debugAction(AutoAttack::getTimer(), 
                                this->getActionName(), 
                                "skill.energyReduceDOWN:" + std::to_string(this->skill ? this->skill->getEnergyReduceDOWN() : 0) +
                                        + ", p.energyReduceDOWN:" + std::to_string(p->energyReduceDOWN));
    // 遍历监听，触发回调
    for (const auto &listener : EnergyConsumeAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 能量回复
std::string EnergyRevertAction::name = "EnergyRevertAction";

void EnergyRevertAction::execute(double n, Person *p)
{
    if(skill)
    {
        n *= (1 + p->energyAddIncrease + this->skill->getEnergyAddIncrease());
    }
    else
    {
        n *= (1 + p->energyAddIncrease);
    }
    p->revertEnergy(n);
    Logger::debugAction(AutoAttack::getTimer(),
                 this->getActionName(), 
                 "triggered, reverted energy: " + std::to_string(n)
                        + ", currentEnengy:" + std::to_string(p->present_energy));
    // 遍历监听，触发回调
    for (const auto &listener : EnergyRevertAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// CD事件
// CD减少
std::string CDReduceAction::name = "CDReduceAction";

CDReduceAction::CDReduceAction(std::string skillName)
    : skillName(skillName) {}

void CDReduceAction::execute(const double n, Person *p)
{
    p->reduceSkillCD(this->skillName, n);
    // 遍历监听，触发回调
    for (const auto &listener : CDReduceAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// CD刷新
std::string CDRefreshAction::name = "CDRefreshAction";

CDRefreshAction::CDRefreshAction(std::string skillName)
    : skillName(skillName) {}

void CDRefreshAction::execute(const double n, Person *p)
{

    Skill *skill = nullptr;
    int index = p->findSkillInSkillCDList(this->skillName);
    skill = p->getSkillCDListRef().at(index).get();
    if (skill)
    {
        // 设置CD为0
        skill->getCurrentCD_Ref() = 0;
        // 若为充能技能，设置充能CD为0
        if (skill->getCanCharge())
        {
            skill->getCurrentChargedCD_Ref() = 0;
        }
        // 如果层数未满，则恢复一层层数
        // 若恢复一层后层数仍未满，则重置CD
        if (skill->getStackRef() < skill->getMaxStack())
        {
            skill->getStackRef() += 1;
        }
        if (skill->getStackRef() < skill->getMaxStack())
        {
            skill->getCurrentCD_Ref() = skill->getMaxCD();
        }
        // 遍历监听，触发回调
        for (const auto &listener : CDRefreshAction::listeners)
        {
            if (listener && listener->callback)
            {
                listener->trigger(n);
            }
        }
    }
}

// 释放技能
std::string CreateSkillAction::name = "CreateSkillAction";

CreateSkillAction::CreateSkillAction(std::string skillName)
    : skillName(skillName) {}

void CreateSkillAction::execute(double, Person *p)
{
    auto it = SkillCreator::createSkill(this->skillName, p);
    if (it)
    {
        // 遍历监听，触发回调
        // 方便回调函数对创建的技能进行修改
        for (const auto &listener : CreateSkillAction::listeners)
        {
            if (listener && listener->callback)
            {
                listener->trigger(it.get());
            }
        }
        p->createSkill(std::move(it));

        int a = p->findSkillInSkillCDList(this->skillName);
        if (a != -1)
        {
            Logger::debugAction(AutoAttack::getTimer(),
                                this->getActionName(),
                                "Skill Created: " + this->skillName +
                                    ", Stack: " +
                                    std::to_string(
                                        p->getSkillCDListRef().at(a)->getStackRef()) +
                                    "/" +
                                    std::to_string(
                                        p->getSkillCDListRef().at(a)->getMaxStack()));
        }
        else
        {
            Logger::debugAction(AutoAttack::getTimer(),
                                this->getActionName(),
                                "Skill Created: " + this->skillName);
        }
    }
    else
    {
        Logger::debugAction(AutoAttack::getTimer(),this->getActionName(),"Failed to create skill: " + this->skillName + ", because no skill founded");
    }
}

// 创建Buff
std::string CreateBuffAction::name = "CreateBuffAction";

CreateBuffAction::CreateBuffAction(std::string buffName)
    : buffName(buffName) {}

void CreateBuffAction::execute(double n, Person *p)
{
    // 先检查是否已存在同名buff
    int existingIndex = p->findBuffInBuffList(this->buffName);

    if (existingIndex != -1)
    { // 已存在同名buff
        auto &existingBuff = p->getBuffListRef().at(existingIndex);
        // 检查是否可叠加
        if (existingBuff->getIsStackable())
        {
            if (existingBuff->getStack() < existingBuff->getMaxStack())
            {
                existingBuff->addStack(n);
                Logger::debugAction(AutoAttack::getTimer(),
                                    this->getActionName(),
                                    "Buff Stack Increased: " + this->buffName +
                                        ", buffID: " + std::to_string(existingBuff->getBuffID()) +
                                        ", Stack: " + std::to_string(existingBuff->getStack()));
            }
            existingBuff->resetDuration();
            Logger::debugAction(AutoAttack::getTimer(),
                                this->getActionName(),
                                "Buff Refreshed: " + this->buffName +
                                    ", buffID: " + std::to_string(existingBuff->getBuffID()));

            return; // 无需创建新buff
        }

        // 检查是否允许多种同类存在
        if (!existingBuff->getAllowDuplicates())
        {
            // 不允许多种同类存在，只刷新持续时间
            existingBuff->resetDuration();

            Logger::debugAction(AutoAttack::getTimer(),
                                this->getActionName(),
                                "Buff Refreshed: " + this->buffName +
                                    ", buffID: " + std::to_string(existingBuff->getBuffID()));
            return; // 无需创建新buff
        }
        // 如果允许重复，则继续创建新buff
    }

    // 未存在重复buff
    // 创建新buff（首次创建或允许重复时）
    auto it = BuffCreator::createBuff(this->buffName, p, n);
    if (it == nullptr)
    {
        Logger::debugAction(AutoAttack::getTimer(),
                            this->getActionName(),
                            "Failed to create buff: " + this->buffName);
        return;
    }

    // 在移动前先获取buffID
    int newBuffID = it->getBuffID();

    // 遍历监听，触发回调
    // 方便回调函数对创建的buff进行修改
    for (const auto &listener : CreateBuffAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(it.get());
        }
    }

    // 添加到buff列表
    p->createBuff(std::move(it));
    Logger::debugAction(AutoAttack::getTimer(),
                        this->getActionName(),
                        "Buff Created: " + this->buffName +
                            ", buffID: " + std::to_string(newBuffID));
}

// 增加因子能量
std::string AddFactorEnergyAction::name = "AddFactorEnergyAction";

void AddFactorEnergyAction::execute(double n, Person *p)
{
    for (const auto &listener : AddFactorEnergyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 属性更改事件
// 暴击数值
std::string CriticalCountModifyAction::name = "CriticalCountModifyAction";
CriticalCountModifyAction::CriticalCountModifyAction() = default;

void CriticalCountModifyAction::execute(double n, Person *p)
{
    p->changeCriticalCount(n);

    for (const auto &listener : CriticalCountModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 暴击百分比
std::string CriticalPercentModifyAction::name = "CriticalPercentModifyAction";
CriticalPercentModifyAction::CriticalPercentModifyAction() = default;

void CriticalPercentModifyAction::execute(double n, Person *p)
{
    p->changeCritialPersent(n);

    for (const auto &listener : CriticalPercentModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 急速数值
std::string QuicknessCountModifyAction::name = "QuicknessCountModifyAction";
QuicknessCountModifyAction::QuicknessCountModifyAction() = default;

void QuicknessCountModifyAction::execute(double n, Person *p)
{
    p->changeQuicknessCount(n);

    // 遍历监听，触发回调
    for (const auto &listener : QuicknessCountModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 急速百分比
std::string QuicknessPercentModifyAction::name = "QuicknessPercentModifyAction";
QuicknessPercentModifyAction::QuicknessPercentModifyAction() = default;

void QuicknessPercentModifyAction::execute(double n, Person *p)
{
    p->changeQuicknessPersent(n);
    // 遍历监听，触发回调
    for (const auto &listener : QuicknessPercentModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 幸运数值
std::string LuckyCountModifyAction::name = "LuckyCountModifyAction";
LuckyCountModifyAction::LuckyCountModifyAction() = default;

void LuckyCountModifyAction::execute(double n, Person *p)
{
    p->changeLuckyCount(n);

    // 遍历监听，触发回调
    for (const auto &listener : LuckyCountModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 幸运百分比
std::string LuckyPercentModifyAction::name = "LuckyPercentModifyAction";
LuckyPercentModifyAction::LuckyPercentModifyAction() = default;

void LuckyPercentModifyAction::execute(double n, Person *p)
{
    p->changeLuckyPersent(n);

    // 遍历监听，触发回调
    for (const auto &listener : LuckyPercentModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 精通数值
std::string ProficientCountModifyAction::name = "ProficientCountModifyAction";
ProficientCountModifyAction::ProficientCountModifyAction() = default;

void ProficientCountModifyAction::execute(double n, Person *p)
{
    p->changeProficientCount(n);

    // 遍历监听，触发回调
    for (const auto &listener : ProficientCountModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 精通百分比
std::string ProficientPercentModifyAction::name = "ProficientPercentModifyAction";
ProficientPercentModifyAction::ProficientPercentModifyAction() = default;

void ProficientPercentModifyAction::execute(double n, Person *p)
{
    p->changeProficientPersent(n);

    // 遍历监听，触发回调
    for (const auto &listener : ProficientPercentModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 全能数值
std::string AlmightyCountModifyAction::name = "AlmightyCountModifyAction";
AlmightyCountModifyAction::AlmightyCountModifyAction() = default;

void AlmightyCountModifyAction::execute(double n, Person *p)
{
    p->changeAlmightyCount(n);

    // 遍历监听，触发回调
    for (const auto &listener : AlmightyCountModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 全能百分比
std::string AlmightyPercentModifyAction::name = "AlmightyPercentModifyAction";
AlmightyPercentModifyAction::AlmightyPercentModifyAction() = default;

void AlmightyPercentModifyAction::execute(double n, Person *p)
{
    p->changeAlmightyPersent(n);

    // 遍历监听，触发回调
    for (const auto &listener : AlmightyPercentModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 主属性数值
std::string PrimaryAttributesCountModifyAction::name = "PrimaryAttributesCountModifyAction";
PrimaryAttributesCountModifyAction::PrimaryAttributesCountModifyAction() = default;

void PrimaryAttributesCountModifyAction::execute(double n, Person *p)
{
    p->changePrimaryAttributesByCount(n);
    // 遍历监听，触发回调
    for (const auto &listener : PrimaryAttributesCountModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 主属性百分比
std::string PrimaryAttributesPercentModifyAction::name = "PrimaryAttributesPercentModifyAction";
PrimaryAttributesPercentModifyAction::PrimaryAttributesPercentModifyAction() = default;

void PrimaryAttributesPercentModifyAction::execute(double n, Person *p)
{
    p->changePrimaryAttributesByPersent(n);
    // 遍历监听，触发回调
    for (const auto &listener : PrimaryAttributesPercentModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 施法速度百分比
std::string CastingSpeedPercentModifyAction::name = "CastingSpeedPercentModifyAction";
CastingSpeedPercentModifyAction::CastingSpeedPercentModifyAction() = default;

void CastingSpeedPercentModifyAction::execute(double n, Person *p)
{
    p->addCastingSpeed(n);
    // 遍历监听，触发回调
    for (const auto &listener : CastingSpeedPercentModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 攻击速度百分比
std::string AttackSpeedPercentModifyAction::name = "AttackSpeedPercentModifyAction";
AttackSpeedPercentModifyAction::AttackSpeedPercentModifyAction() = default;

void AttackSpeedPercentModifyAction::execute(double n, Person *p)
{
    p->addAttackSpeed(n);
    // 遍历监听，触发回调
    for (const auto &listener : AttackSpeedPercentModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 攻击增加数值
std::string AttackCountModifyAction::name = "AttackCountModifyAction";
AttackCountModifyAction::AttackCountModifyAction() = default;

void AttackCountModifyAction::execute(double n, Person *p)
{
    p->changeATKCount(n);
    // 遍历监听，触发回调
    for (const auto &listener : AttackCountModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 攻击增伤百分比
std::string AttackIncreaseModifyAction::name = "AttackIncreaseModifyAction";
AttackIncreaseModifyAction::AttackIncreaseModifyAction() = default;

void AttackIncreaseModifyAction::execute(double n, Person *p)
{
    p->changeAattackIncrease(n);
    // 遍历监听，触发回调
    for (const auto &listener : AttackIncreaseModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 精炼攻击增加数值
std::string RefineATKCountModifyAction::name = "RefineATKCountModifyAction";
RefineATKCountModifyAction::RefineATKCountModifyAction() = default;

void RefineATKCountModifyAction::execute(double n, Person *p)
{
    p->changeRefineATKCount(n);
    // 遍历监听，触发回调
    for (const auto &listener : RefineATKCountModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 伤害增伤百分比
std::string DamageIncreaseModifyAction::name = "DamageIncreaseModifyAction";
DamageIncreaseModifyAction::DamageIncreaseModifyAction() = default;

void DamageIncreaseModifyAction::execute(double n, Person *p)
{
    p->changeDamageIncrease(n);
    // 遍历监听，触发回调
    for (const auto &listener : DamageIncreaseModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 元素增伤百分比
std::string ElementIncreaseModifyAction::name = "ElementIncreaseModifyAction";
ElementIncreaseModifyAction::ElementIncreaseModifyAction() = default;

void ElementIncreaseModifyAction::execute(double n, Person *p)
{
    p->changeElementIncreaseByElementIncrease(n);
    // 遍历监听，触发回调
    for (const auto &listener : ElementIncreaseModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 全能增伤百分比
std::string AlmightyIncreaseModifyAction::name = "AlmightyIncreaseModifyAction";
AlmightyIncreaseModifyAction::AlmightyIncreaseModifyAction() = default;

void AlmightyIncreaseModifyAction::execute(double n, Person *p)
{
    p->changeAlmightyIncrease(n);
    // 遍历监听，触发回调
    for (const auto &listener : AlmightyIncreaseModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 爆伤百分比
std::string CriticalDamageModifyAction::name = "CriticalDamageModifyAction";
CriticalDamageModifyAction::CriticalDamageModifyAction() = default;

void CriticalDamageModifyAction::execute(double n, Person *p)
{
    p->changeCriticalDamage(n);
    // 遍历监听，触发回调
    for (const auto &listener : CriticalDamageModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

// 梦境增伤百分比
std::string DreamIncreaseModifyAction::name = "DreamIncreaseModifyAction";
DreamIncreaseModifyAction::DreamIncreaseModifyAction() = default;

void DreamIncreaseModifyAction::execute(double n, Person *p)
{
    p->changeDreamIncrease(n);
    // 遍历监听，触发回调
    for (const auto &listener : DreamIncreaseModifyAction::listeners)
    {
        if (listener && listener->callback)
        {
            listener->trigger(n);
        }
    }
}

