#include "AutoAttack.h"
#include "Skill.h"
#include "Buff.h"
#include "../../core/Person.h"
#include "../../FightingFantasy/Skill.h"
#include "../../core/Logger.h"
#include <utility>

// 法师：冰矛 autoAttack（数据驱动基类，幻想技能名参数化）
AutoAttack_Mage_Icicle_Base::AutoAttack_Mage_Icicle_Base(Person *p, std::string fantasySkillName)
    : AutoAttack(p), m_fantasySkillName(std::move(fantasySkillName))
{
    addJudgingConditionsForCantCreateSkill(
        [p](const PriorSkillInfo &i)
        {
            // 冰矛释放条件：资源大于0
            return (p->getResourceNum() < 1 && i.skillName == Spear::name);
        },
        ErrorInfo::errorTypeEnum::Resource_Not_Enough);
}

void AutoAttack_Mage_Icicle_Base::checkAndTriggerLargeOutBurst()
{
    // 更新技能CD，buffCD，释放技能判断，爆发判断
    int mukuChiefIndex = this->p->findSkillInSkillCDList(MukuChief::name);
    int fantasyIndex = this->p->findSkillInSkillCDList(this->m_fantasySkillName);
    int ultimateIndex = this->p->findSkillInSkillCDList(Ultimate_Icicle::name);
    int floodIndex = this->p->findSkillInSkillCDList(Flood_Icicle::name);

    // 检查索引有效性
    bool canLargeOutBurst = false;

    if (!this->isOutBurst && this->nextOutBurstType == 1)
    { // 大爆发判定:非爆发状态，下一个爆发为大爆发
        // 检查所有索引都有效
        if (mukuChiefIndex != -1 && fantasyIndex != -1 &&
            ultimateIndex != -1 && floodIndex != -1)
        {

            // 检查 CD 是否为 0
            canLargeOutBurst =
                ((this->p->getSkillCDListRef().at(mukuChiefIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuChiefIndex)->getStackValue() > 0) && this->p->getSkillCDListRef().at(fantasyIndex)->getCurrentCD_Value() == 0 && this->p->getSkillCDListRef().at(ultimateIndex)->getCurrentCD_Value() == 0 && this->p->getSkillCDListRef().at(floodIndex)->getCurrentCD_Value() == 0);
        }
    }

    if (canLargeOutBurst)
    {
        this->largeOutBurst();
    }
}

void AutoAttack_Mage_Icicle_Base::checkAndTriggerSmallOutBurst()
{
    // 更新技能CD，buffCD，释放技能判断，爆发判断
    int mukuChiefIndex = this->p->findSkillInSkillCDList(MukuChief::name);
    int floodIndex = this->p->findSkillInSkillCDList(Flood_Icicle::name);

    // 检查索引有效性
    bool canSmallOutBurst = false;

    if (!this->isOutBurst && this->nextOutBurstType == 0)
    { // 小爆发判定:非爆发状态，下一个爆发为小爆发
        if (mukuChiefIndex != -1 && floodIndex != -1)
        {
            canSmallOutBurst =
                ((this->p->getSkillCDListRef().at(mukuChiefIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuChiefIndex)->getStackValue() > 0) && this->p->getSkillCDListRef().at(floodIndex)->getCurrentCD_Value() == 0);
        }
    }

    if (canSmallOutBurst)
    {
        this->smallOutBurst();
    }
}

void AutoAttack_Mage_Icicle_Base::checkAndFinishOutBurst()
{
    // 结束爆发状态
    if (this->p->findBuffInBuffList(FloodBuff_Icicle::name) == -1 && this->isOutBurst)
    {
        // 如果处于爆发状态时未搜寻到FloodBuff，标志着退出爆发状态
        this->finishOutBurst();
    }
}

void AutoAttack_Mage_Icicle_Base::checkAndSetOutBurstState()
{
    if (this->p->findBuffInBuffList(FloodBuff_Icicle::name) != -1 && !this->isOutBurst)
    {
        // 如果未处于爆发状态时搜寻到FloodBuff创建之后，标志着进入爆发状态
        this->isOutBurst = true;
        this->signalForRepeatedlyOutBurst = false;
        Logger::debugAutoAttack(this->timer,
                                  "Set Out Burst State");
    }
}

void AutoAttack_Mage_Icicle_Base::windowPeriodLogic()
{
    // 空窗期
    if (!this->isOutBurst && !this->signalForRepeatedlyOutBurst)
    {
        // 爆发结束后的逻辑，如果存在无尽苦寒：即刚从爆发状态结束，无尽苦寒刚由FloodBuff创建。
        // 则提升陨星优先级，否则重置陨星优先级
        int endlessColdIndex = this->p->findBuffInBuffList(EndlessColdBuff::name);
        if (endlessColdIndex != -1)
        {
            this->changePriority(Meteorite::name, 1);
        }
        else
        {
            for (auto &i : this->skillPriority)
            {
                if (i.skillName == Meteorite::name && i.priority > 0)
                {
                    this->resetCertainPriority(Meteorite::name);
                }
            }
        }
        // 资源满了就打一发矛
        if (this->p->getResourceNum() >= 4)
        {
            this->autoAddPriorSkillList(Spear::name);
        }
    }
}

void AutoAttack_Mage_Icicle_Base::update(int deltaTime)
{
    // 大爆发
    checkAndTriggerLargeOutBurst();

    // 小爆发
    checkAndTriggerSmallOutBurst();

    // 检查进入爆发状态
    checkAndSetOutBurstState();

    // 检查结束爆发状态
    checkAndFinishOutBurst();

    // 空窗期逻辑
    windowPeriodLogic();

    // 更新时间，创建技能
    this->createSkillByAuto();
    this->updatePerson(deltaTime);
}

void AutoAttack_Mage_Icicle_Base::largeOutBurst()
{
    if (!this->signalForRepeatedlyOutBurst)
    { // 标志位，防止重复触发
        this->maniAddPriorSkillList(MukuChief::name);
        this->maniAddPriorSkillList(this->m_fantasySkillName);
        this->maniAddPriorSkillList(Ultimate_Icicle::name);
        this->maniAddPriorSkillList(Flood_Icicle::name);
        this->maniAddPriorSkillList(Spear::name);
        this->maniAddPriorSkillList(Spear::name);
        this->maniAddPriorSkillList(Spear::name);
        this->maniAddPriorSkillList(WaterDrop::name);
        this->maniAddPriorSkillList(WaterDrop::name);
        this->nextOutBurstType = 0; // 将下一波爆发标志为小爆发

        this->changePriority(WaterDrop::name, 10);
        this->changePriority(Spear::name, 100);
        this->changePriority(Meteorite::name, 1);

        this->signalForRepeatedlyOutBurst = true;
        Logger::debugAutoAttack(this->timer,
                                  "Large Out Burst");
    }
}

void AutoAttack_Mage_Icicle_Base::smallOutBurst()
{
    if (!this->signalForRepeatedlyOutBurst)
    { // 标志位，防止重复触发
        this->maniAddPriorSkillList(MukuChief::name);
        this->maniAddPriorSkillList(Flood_Icicle::name);
        this->maniAddPriorSkillList(Spear::name);
        this->maniAddPriorSkillList(Spear::name);
        this->maniAddPriorSkillList(Spear::name);
        this->maniAddPriorSkillList(WaterDrop::name);
        this->maniAddPriorSkillList(WaterDrop::name);
        this->nextOutBurstType = 1; // 将下一波爆发标志为大爆发

        this->changePriority(WaterDrop::name, 100);
        this->changePriority(Spear::name, 10);
        this->changePriority(Meteorite::name, 1);

        this->signalForRepeatedlyOutBurst = true;

        Logger::debugAutoAttack(this->timer,
                                  "Small Out Burst");
    }
}

// 具体幻想变体：仅传入幻想技能名
AutoAttack_Mage_Icicle_MukuScout::AutoAttack_Mage_Icicle_MukuScout(Person *p)
    : AutoAttack_Mage_Icicle_Base(p, MukuScout::name) {}

AutoAttack_Mage_Icicle_BYS::AutoAttack_Mage_Icicle_BYS(Person *p)
    : AutoAttack_Mage_Icicle_Base(p, BYS::name) {}

AutoAttack_Mage_Icicle_YGLWS::AutoAttack_Mage_Icicle_YGLWS(Person *p)
    : AutoAttack_Mage_Icicle_Base(p, YGLWS::name) {}
