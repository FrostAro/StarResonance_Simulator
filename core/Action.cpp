#include "Action.h"
#include "AutoAttack.h"
#include "Creators.hpp"
#include "Logger.h"
#include "Person.h"
#include <algorithm>
#include <memory>

std::string Action::name = "Action";

std::string Action::getActionName() { return this->name; }

// 攻击事件
std::string AttackAction::name = "AttackAction";
std::vector<std::unique_ptr<DamageListener>> AttackAction::listeners = {};

AttackAction::AttackAction(const Skill* const skill)
    : skill(skill) {}

void AttackAction::addListener(std::unique_ptr<DamageListener> listener)
{
    listeners.push_back(std::move(listener));
}

void AttackAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<DamageListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string AttackAction::getActionName() { return AttackAction::name; }

// 资源消耗
std::string ResourceConsumeAction::name = "ResourceConsumeAction";
std::vector<std::unique_ptr<ResourceListener>>
    ResourceConsumeAction::listeners = {};

void ResourceConsumeAction::addListener(
    std::unique_ptr<ResourceListener> info)
{
    listeners.push_back(std::move(info));
}

void ResourceConsumeAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<ResourceListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string ResourceConsumeAction::getActionName()
{
    return ResourceConsumeAction::name;
}

// 玄冰回复
std::string ResourceRevertAction::name = "ResourceRevertAction";
std::vector<std::unique_ptr<ResourceListener>> ResourceRevertAction::listeners =
    {};

void ResourceRevertAction::addListener(std::unique_ptr<ResourceListener> info)
{
    listeners.push_back(std::move(info));
}

void ResourceRevertAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<ResourceListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string ResourceRevertAction::getActionName()
{
    return ResourceRevertAction::name;
}

// 能量消耗
std::string EnergyConsumeAction::name = "EnergyConsumeAction";
std::vector<std::unique_ptr<EnergyListener>> EnergyConsumeAction::listeners =
    {};

void EnergyConsumeAction::addListener(std::unique_ptr<EnergyListener> info)
{
    listeners.push_back(std::move(info));
}

void EnergyConsumeAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<EnergyListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string EnergyConsumeAction::getActionName()
{
    return EnergyConsumeAction::name;
}

// 能量回复
std::string EnergyRevertAction::name = "EnergyRevertAction";
std::vector<std::unique_ptr<EnergyListener>> EnergyRevertAction::listeners = {};

void EnergyRevertAction::addListener(std::unique_ptr<EnergyListener> info)
{
    listeners.push_back(std::move(info));
}

void EnergyRevertAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<EnergyListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string EnergyRevertAction::getActionName()
{
    return EnergyRevertAction::name;
}

// CD事件
// CD减少
std::string CDReduceAction::name = "CDReduceAction";
std::vector<std::unique_ptr<CDListener>> CDReduceAction::listeners = {};

CDReduceAction::CDReduceAction(std::string skillName)
    : skillName(skillName) {}

void CDReduceAction::addListener(std::unique_ptr<CDListener> info)
{
    listeners.push_back(std::move(info));
}

void CDReduceAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<CDListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string CDReduceAction::getActionName() { return CDReduceAction::name; }

// CD刷新
std::string CDRefreshAction::name = "CDRefreshAction";
std::vector<std::unique_ptr<CDListener>> CDRefreshAction::listeners = {};

CDRefreshAction::CDRefreshAction(std::string skillName)
    : skillName(skillName) {}

void CDRefreshAction::addListener(std::unique_ptr<CDListener> info)
{
    listeners.push_back(std::move(info));
}

void CDRefreshAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<CDListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string CDRefreshAction::getActionName() { return CDRefreshAction::name; }

// 释放技能
std::string CreateSkillAction::name = "CreateSkillAction";
std::vector<std::unique_ptr<CreateSkillListener>> CreateSkillAction::listeners =
    {};

CreateSkillAction::CreateSkillAction(std::string skillName)
    : skillName(skillName) {}

void CreateSkillAction::addListener(std::unique_ptr<CreateSkillListener> info)
{
    listeners.push_back(std::move(info));
}

void CreateSkillAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<CreateSkillListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string CreateSkillAction::getActionName()
{
    return CreateSkillAction::name;
}

// 创建Buff
std::string CreateBuffAction::name = "CreateBuffAction";
std::vector<std::unique_ptr<CreateBuffListener>> CreateBuffAction::listeners =
    {};

CreateBuffAction::CreateBuffAction(std::string buffName)
    : buffName(buffName) {}

void CreateBuffAction::addListener(std::unique_ptr<CreateBuffListener> info)
{
    listeners.push_back(std::move(info));
}

void CreateBuffAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<CreateBuffListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string CreateBuffAction::getActionName() { return CreateBuffAction::name; }

// 增加因子能量
std::string AddFactorEnergyAction::name = "AddFactorEnergyAction";
std::vector<std::unique_ptr<FactorEnergyListener>> AddFactorEnergyAction::listeners =
    {};

void AddFactorEnergyAction::addListener(std::unique_ptr<FactorEnergyListener> info)
{
    listeners.push_back(std::move(info));
}

void AddFactorEnergyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<FactorEnergyListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string AddFactorEnergyAction::getActionName() { return AddFactorEnergyAction::name; }

// 属性更改事件
// 暴击数值
std::string CriticalCountModifyAction::name = "CriticalCountModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> CriticalCountModifyAction::listeners =
    {};

CriticalCountModifyAction::CriticalCountModifyAction()
    : Action() {}

void CriticalCountModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void CriticalCountModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string CriticalCountModifyAction::getActionName() { return CriticalCountModifyAction::name; }

// 暴击百分比
std::string CriticalPercentModifyAction::name = "CriticalPercentModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> CriticalPercentModifyAction::listeners =
    {};

CriticalPercentModifyAction::CriticalPercentModifyAction()
    : Action() {}

void CriticalPercentModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void CriticalPercentModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string CriticalPercentModifyAction::getActionName() { return CriticalPercentModifyAction::name; }

// 急速数值
std::string QuicknessCountModifyAction::name = "QuicknessCountModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> QuicknessCountModifyAction::listeners = {};

QuicknessCountModifyAction::QuicknessCountModifyAction()
    : Action() {}

void QuicknessCountModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void QuicknessCountModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string QuicknessCountModifyAction::getActionName() { return QuicknessCountModifyAction::name; }

// 急速百分比
std::string QuicknessPercentModifyAction::name = "QuicknessPercentModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> QuicknessPercentModifyAction::listeners = {};

QuicknessPercentModifyAction::QuicknessPercentModifyAction()
    : Action() {}

void QuicknessPercentModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void QuicknessPercentModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string QuicknessPercentModifyAction::getActionName() { return QuicknessPercentModifyAction::name; }

// 幸运数值
std::string LuckyCountModifyAction::name = "LuckyCountModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> LuckyCountModifyAction::listeners = {};

LuckyCountModifyAction::LuckyCountModifyAction()
    : Action() {}

void LuckyCountModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void LuckyCountModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string LuckyCountModifyAction::getActionName() { return LuckyCountModifyAction::name; }

// 幸运百分比
std::string LuckyPercentModifyAction::name = "LuckyPercentModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> LuckyPercentModifyAction::listeners = {};

LuckyPercentModifyAction::LuckyPercentModifyAction()
    : Action() {}

void LuckyPercentModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void LuckyPercentModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string LuckyPercentModifyAction::getActionName() { return LuckyPercentModifyAction::name; }

// 精通数值
std::string ProficientCountModifyAction::name = "ProficientCountModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> ProficientCountModifyAction::listeners = {};

ProficientCountModifyAction::ProficientCountModifyAction()
    : Action() {}

void ProficientCountModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void ProficientCountModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string ProficientCountModifyAction::getActionName() { return ProficientCountModifyAction::name; }

// 精通百分比
std::string ProficientPercentModifyAction::name = "ProficientPercentModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> ProficientPercentModifyAction::listeners = {};

ProficientPercentModifyAction::ProficientPercentModifyAction()
    : Action() {}

void ProficientPercentModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void ProficientPercentModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string ProficientPercentModifyAction::getActionName() { return ProficientPercentModifyAction::name; }

// 全能数值
std::string AlmightyCountModifyAction::name = "AlmightyCountModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> AlmightyCountModifyAction::listeners = {};

AlmightyCountModifyAction::AlmightyCountModifyAction()
    : Action() {}

void AlmightyCountModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void AlmightyCountModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string AlmightyCountModifyAction::getActionName() { return AlmightyCountModifyAction::name; }

// 全能百分比
std::string AlmightyPercentModifyAction::name = "AlmightyPercentModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> AlmightyPercentModifyAction::listeners = {};

AlmightyPercentModifyAction::AlmightyPercentModifyAction()
    : Action() {}

void AlmightyPercentModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void AlmightyPercentModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string AlmightyPercentModifyAction::getActionName() { return AlmightyPercentModifyAction::name; }

// 主属性数值
std::string PrimaryAttributesCountModifyAction::name = "PrimaryAttributesCountModifyAction";
std::vector<std::unique_ptr<PrimaryAttributeListener>> PrimaryAttributesCountModifyAction::listeners = {};

PrimaryAttributesCountModifyAction::PrimaryAttributesCountModifyAction()
    : Action() {}

void PrimaryAttributesCountModifyAction::addListener(std::unique_ptr<PrimaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void PrimaryAttributesCountModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<PrimaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string PrimaryAttributesCountModifyAction::getActionName() { return PrimaryAttributesCountModifyAction::name; }

// 主属性百分比
std::string PrimaryAttributesPercentModifyAction::name = "PrimaryAttributesPercentModifyAction";
std::vector<std::unique_ptr<PrimaryAttributeListener>> PrimaryAttributesPercentModifyAction::listeners = {};

PrimaryAttributesPercentModifyAction::PrimaryAttributesPercentModifyAction()
    : Action() {}

void PrimaryAttributesPercentModifyAction::addListener(std::unique_ptr<PrimaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void PrimaryAttributesPercentModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<PrimaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string PrimaryAttributesPercentModifyAction::getActionName() { return PrimaryAttributesPercentModifyAction::name; }

// 施法速度百分比
std::string CastingSpeedPercentModifyAction::name = "CastingSpeedPercentModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> CastingSpeedPercentModifyAction::listeners = {};

CastingSpeedPercentModifyAction::CastingSpeedPercentModifyAction()
    : Action() {}

void CastingSpeedPercentModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void CastingSpeedPercentModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string CastingSpeedPercentModifyAction::getActionName() { return CastingSpeedPercentModifyAction::name; }

// 攻击速度百分比
std::string AttackSpeedPercentModifyAction::name = "AttackSpeedPercentModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> AttackSpeedPercentModifyAction::listeners = {};

AttackSpeedPercentModifyAction::AttackSpeedPercentModifyAction()
    : Action() {}

void AttackSpeedPercentModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void AttackSpeedPercentModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string AttackSpeedPercentModifyAction::getActionName() { return AttackSpeedPercentModifyAction::name; }

// 攻击增加数值
std::string AttackCountModifyAction::name = "AttackCountModifyAction";
std::vector<std::unique_ptr<PrimaryAttributeListener>> AttackCountModifyAction::listeners = {};

AttackCountModifyAction::AttackCountModifyAction()
    : Action() {}

void AttackCountModifyAction::addListener(std::unique_ptr<PrimaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void AttackCountModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<PrimaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string AttackCountModifyAction::getActionName() { return AttackCountModifyAction::name; }

// 攻击增伤百分比
std::string AttackIncreaseModifyAction::name = "AttackIncreaseModifyAction";
std::vector<std::unique_ptr<PrimaryAttributeListener>> AttackIncreaseModifyAction::listeners = {};

AttackIncreaseModifyAction::AttackIncreaseModifyAction()
    : Action() {}

void AttackIncreaseModifyAction::addListener(std::unique_ptr<PrimaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void AttackIncreaseModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<PrimaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string AttackIncreaseModifyAction::getActionName() { return AttackIncreaseModifyAction::name; }

// 精炼攻击增加数值
std::string RefineATKCountModifyAction::name = "RefineATKCountModifyAction";
std::vector<std::unique_ptr<PrimaryAttributeListener>> RefineATKCountModifyAction::listeners = {};

RefineATKCountModifyAction::RefineATKCountModifyAction()
    : Action() {}

void RefineATKCountModifyAction::addListener(std::unique_ptr<PrimaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void RefineATKCountModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<PrimaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string RefineATKCountModifyAction::getActionName() { return RefineATKCountModifyAction::name; }

// 伤害增伤百分比
std::string DamageIncreaseModifyAction::name = "DamageIncreaseModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> DamageIncreaseModifyAction::listeners = {};

DamageIncreaseModifyAction::DamageIncreaseModifyAction()
    : Action() {}

void DamageIncreaseModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void DamageIncreaseModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string DamageIncreaseModifyAction::getActionName() { return DamageIncreaseModifyAction::name; }

// 元素增伤百分比
std::string ElementIncreaseModifyAction::name = "ElementIncreaseModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> ElementIncreaseModifyAction::listeners = {};

ElementIncreaseModifyAction::ElementIncreaseModifyAction()
    : Action() {}

void ElementIncreaseModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void ElementIncreaseModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string ElementIncreaseModifyAction::getActionName() { return ElementIncreaseModifyAction::name; }

// 全能增伤百分比
std::string AlmightyIncreaseModifyAction::name = "AlmightyIncreaseModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> AlmightyIncreaseModifyAction::listeners = {};

AlmightyIncreaseModifyAction::AlmightyIncreaseModifyAction()
    : Action() {}

void AlmightyIncreaseModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void AlmightyIncreaseModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string AlmightyIncreaseModifyAction::getActionName() { return AlmightyIncreaseModifyAction::name; }

// 爆伤百分比
std::string CriticalDamageModifyAction::name = "CriticalDamageModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> CriticalDamageModifyAction::listeners = {};

CriticalDamageModifyAction::CriticalDamageModifyAction()
    : Action() {}

void CriticalDamageModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void CriticalDamageModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string CriticalDamageModifyAction::getActionName() { return CriticalDamageModifyAction::name; }

// 梦境增伤百分比
std::string DreamIncreaseModifyAction::name = "DreamIncreaseModifyAction";
std::vector<std::unique_ptr<SecondaryAttributeListener>> DreamIncreaseModifyAction::listeners = {};

DreamIncreaseModifyAction::DreamIncreaseModifyAction()
    : Action() {}

void DreamIncreaseModifyAction::addListener(std::unique_ptr<SecondaryAttributeListener> info)
{
    listeners.push_back(std::move(info));
}

void DreamIncreaseModifyAction::deleteListener(int buffID)
{
    const auto it =
        std::find_if(listeners.begin(), listeners.end(),
                     [buffID](const std::unique_ptr<SecondaryAttributeListener> &item)
                     {
                         return item->buffID == buffID;
                     });

    if (it != listeners.end())
    {
        listeners.erase(it);
    }
}

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

std::string DreamIncreaseModifyAction::getActionName() { return DreamIncreaseModifyAction::name; }