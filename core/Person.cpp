#include "Person.h"
#include <cstddef>
// #include <random>
#include <ctime>
#include <memory>
#include <cassert>
#include "Action.h"
#include "AutoAttack.h"
#include "Buff.h"
#include "Skill.h"
#include "Logger.h"
#include "Creators.hpp"
#include "Statistics.h"

Person::Person(){};

Person::Person(const double PrimaryAttributes, const double critical, const double quickness, const double lucky, const double Proficient, const double almighty,
               const int atk, const int refindatk, const int elementatk, const double attackSpeed, const double castingSpeed,
               const double criticaldamage_set, const double increase_set, const double elementincrease_set, const int totalTime)
    : propertyTransformationCoeffcient_General(200000),     // 一般属性转化系数
      propertyTransformationCoeffcient_Almighty(89600),    // 全能属性转化系数
      totalTime(totalTime),                                // 总模拟时间
      proficientRatio(0),                                  // 精通转化率（子类设置）
      almightyRatio(0),                                    // 全能转化率（子类设置）
      castingSpeedRatio(1),                                // 施法速度转化率（子类设置）
      attackSpeedRatio(1),                                 // 攻击速度转化率（子类设置）
      primaryAttributeRatio(0),                            // 主属性转化率（子类设置）
      max_energy(100),                                     // 最大能量值
      present_energy(max_energy),                          // 当前能量值（初始满）
      Critical(critical / 100),                            // 暴击率（百分比转小数）
      Lucky(lucky / 100),                                  // 幸运率（百分比转小数）
      Almighty(almighty / 100),                            // 全能值（百分比转小数）
      Proficient(Proficient / 100),                        // 精通值（百分比转小数）
      Quickness(quickness / 100),                          // 急速值（百分比转小数）
      refineATK(refindatk),                                // 精炼攻击力
      elementATK(elementatk),                              // 元素攻击力
      primaryAttributes(PrimaryAttributes),                // 主属性数值
      resourceNum(0),                                      // 初始玄冰资源数量
      maxResourceNum(0),                                   // 最大玄冰资源数量
      castingSpeedExtra(castingSpeed / 100),               // 施法速度加成
      attackSpeedExtra(attackSpeed / 100),                 // 攻击速度加成
      criticicalDamage(0.5),                               // 基础暴击伤害加成（+50%）
      damageReduce(0),                                     // 基础减伤率
      criticaldamage_set(criticaldamage_set),              // 额外暴击伤害（调试用）
      increase_set(increase_set),                          // 额外增伤（调试用）
      elementincrease_set(elementincrease_set),            // 额外元素增伤（调试用）
      luckyDamageIncrease(Lucky),                          // 幸运伤害增幅（初始等于幸运值）
      randomEngine(std::random_device{}())             // 初始化随机数引擎
      //uniformDist(0.1,1.0)
{
    // 计算属性数值（将百分比转换为属性点数）
    this->CriticalCount = getCriticalCount(this->Critical - 0.05);
    this->QuicknessCount = getQuicknessCount(this->Quickness);
    this->LuckyCount = getLuckyCount(this->Lucky - 0.05);
    this->ProficientCount = getProficientCount(this->Proficient - 0.06);
    this->AlmightyCount = getAlmightyCount(this->Almighty);

    // 设置幸运倍率
    setLuckyMultiplying();

    

    // buffList.reserve(15);  // 预留buff列表空间（可选优化）
    recalcSpeedFromQuickness();  // 根据初始急速值计算施法速度和攻击速度
}

Person::~Person()
{
    clearSkills();  // 清理所有技能
    clearBuffs();   // 清理所有buff
    this->pointerListForAction.clear();  // 清理动作指针列表
    this->noReleasingSkillList.clear();  // 清理无前摇技能列表
    this->skillCDList.clear();  // 清理技能冷却列表
}

DamageInfo Person::Damage(const Skill *skill)
{
    // 攻击力×倍率×(M)×减伤区+(精炼攻击+元素攻击)×倍率×(N)+固定值】×增伤×元素伤害×全能增幅×梦境增伤x技能最终伤害加成x爆伤期望
    if (skill)
    {
        double damage =
            /*基础攻击区*/ ((this->ATK * (1 + this->attackIncrease) * skill->getMutiplying() * (1 + skill->multiplyingIncrease) * (1 - this->damageReduce))
                            /*精炼攻击与元素攻击区*/
                            + ((this->refineATK + this->elementATK) * skill->getMutiplying() * (1 + skill->multiplyingIncrease))
                            /*技能固定值*/
                            + skill->getFixedValue())
            /*增伤区*/
            * (1 + this->damageIncrease + skill->damageIncreaseAdd)
            /*增效区*/
            * (1 + this->enhenceIncrease)
            /*元素增伤区*/
            * (1 + this->elementIncrease + skill->elementIncreaseAdd)
            /*全能增伤区*/
            * (1 + this->almightyIncrease + skill->almightyIncreaseAdd)
            /*梦境增伤区*/
            * (1 + this->dreamIncrease + skill->dreamIncreaseAdd)
            /*技能最终伤害提升区*/
            * (1 + skill->finalIncreaseAdd);

        // 暴击期望
        double crit = 0;
        if(this->Critical + skill->criticalAdd > 1.0)
            crit = 1.0;
        else if(this->Critical + skill->criticalAdd < 0.0)
            crit = 0.0;
        else
            crit = this->Critical + skill->criticalAdd;
        if(!skill->getCanTriggerCrit())
            crit = 0.0;
        damage *= 1 + (this->criticicalDamage + skill->criticalIncreaseAdd) * crit;
        // 实际暴击模拟
        // double isCrit = this->isSuccess(this->Critical + skill->criticalAdd);
        // if (isCrit)
        // {
        //     damage *= 1 + this->criticicalDamage + skill->criticalIncreaseAdd;
        // }

        double luckyDamage = 0;
        DamageInfo info;
        if (skill->getCanTriggerLucky())
        {
            luckyDamage = this->luckyDamage(skill);
            info = DamageInfo(skill->getSkillName(), damage,
                        luckyDamage, this->isSuccess(this->Critical + skill->criticalAdd), this->isSuccess(this->Lucky));
        }
        else
        {
            info = DamageInfo(skill->getSkillName(), damage,
                            luckyDamage, this->isSuccess(this->Critical + skill->criticalAdd), false);
        }
        // if(info.skillName == Spear::name)
        // {
        //     std::cout << "[DEBUG,timer=" << AutoAttack::getTimer() << "]: Damage - skill: " << info.skillName << " damaged" << std::endl;
        // }
        return info;
    }
    return DamageInfo();  // 技能为空，返回空伤害信息
}

double Person::luckyDamage(const Skill *skill) const
{
    // 幸运伤害计算公式(概率)
    //double damage = ((this->ATK * this->luckyMultiplying * (1 + this->attackIncrease) * (1 - this->damageReduce) + (this->refineATK + this->elementATK) * this->luckyMultiplying)) * (1 + this->elementIncrease) * (1 + this->damageIncrease + this->luckyDamageIncrease) * (1 + this->almightyIncrease);
    // (期望)
    double damage = ((this->getATK() * (this->luckyMultiplying + this->luckyMultiplyingExtra) * (1 + this->attackIncrease) * (1 - this->damageReduce) + (this->refineATK + this->elementATK) * (this->luckyMultiplying + this->luckyMultiplyingExtra)) + skill->getLuckyFiexedValue()) 
                        * (1 + this->elementIncrease) 
                        * (1 + this->damageIncrease + this->Lucky + this->luckyDamageIncrease + skill->getLuckyIncreaseAdd()) 
                        * (1 + this->almightyIncrease)
                        * (1 + this->dreamIncrease + this->luckyDreamIncrease)
                        * (1 + this->enhenceIncrease)
                        * (1 + this->luckyFinalIncrease)
                        * this->Lucky;

    // 幸运伤害也可暴击
    damage *= 1 + (this->criticicalDamage + skill->criticalIncreaseAdd) * this->Critical;
    return damage;
}

void Person::updateSkills(int deltaTime)
{
    this->updateNowReleasingSkill(deltaTime);   // 更新当前正在释放的技能
    this->updateContinuousList(deltaTime);      // 更新持续性技能列表
}

void Person::updateNowReleasingSkill(int deltaTime)
{
    if (!this->nowReleasingSkill) // 当前无释放技能
        return;

    if (this->nowReleasingSkill->getReleasingTime() > 0)
    { // 如果技能前摇未结束
        this->nowReleasingSkill->reduceReleasingTime(deltaTime);
        return;
    }
    // 前摇结束，可以进行下一步逻辑
    // 如果是立即触发技能，前摇结束即可直接use()

    if (this->nowReleasingSkill->getIsContinuous())
    { // 若是持续性技能则加入技能列表，如涡流
        continuousSkillList.push_back(std::move(this->nowReleasingSkill));
        this->nowReleasingSkill.reset();
        this->nowReleasingSkill = nullptr;
        //  向autoAttack传递技能释放完成消息，提醒autoAttack可以进行下一次技能释放
        this->autoAttackPtr->updateSkillFinish();
        return;
    }

    if (this->nowReleasingSkill->getIsFacilitation())
    { // 若是引导性技能，则检查是否结束引导
        FacilitationSkill *facilitationSkillPtr = static_cast<FacilitationSkill *>(this->nowReleasingSkill.get());
        // 添加调试断言来确保设计正确性
        // std::cout << "now is releasing beam" << std::endl;
        // std::cout << "current energy:" << this->present_energy << std::endl;
        assert(dynamic_cast<FacilitationSkill *>(this->nowReleasingSkill.get()) != nullptr && 
            "getIsFacilitation() is true but type is not FacilitationSkill!");
        

        if (facilitationSkillPtr->canEndFacilitation(this))
        {
            // 结束引导，清空当前技能
            this->nowReleasingSkill.reset();
            this->nowReleasingSkill = nullptr;
            //  向autoAttack传递技能释放完成消息，提醒autoAttack可以进行下一次技能释放
            Logger::debugAction(AutoAttack::getTimer(),
                                "FacilitationSkill",
                                " - facilitation skill finish ");
            this->autoAttackPtr->updateSkillFinish();
            return;
        }
    }

    // 调用立即触发技能与持续引导技能的use()
    this->nowReleasingSkill->use(this);

    //  清空当前释放技能
    if (this->nowReleasingSkill->getIsInstant())
    { // 非引导性技能释放完成后清空
        this->nowReleasingSkill.reset();
        this->nowReleasingSkill = nullptr;
        //  向autoAttack传递技能释放完成消息，提醒autoAttack可以进行下一次技能释放
        this->autoAttackPtr->updateSkillFinish();
    }
}

void Person::updateContinuousList(int deltaTime)
{
    // 更新所有技能的计时器、冷却等
    for (int i = 0; i < static_cast<int>(continuousSkillList.size()); ++i)
    {
        Skill *skill = continuousSkillList[i].get();
        if (!skill)
            continue;
        // 早期返回：跳过不需要更新的技能
        if (skill->getDuration() < 0)
            continue;

        skill->reduceDuration(deltaTime);

        // 触发伤害
        // 根据技能类型触发不同逻辑
        skill->use(this);
    }
    // 清理已完成技能
    cleanupFinishedSkills();
}

// 通过指针删除技能
bool Person::removeSkill(Skill *skill)
{
    auto it = std::find_if(continuousSkillList.begin(), continuousSkillList.end(),
                           [skill](const std::unique_ptr<Skill> &ptr)
                           {
                               return ptr.get() == skill;
                           });

    if (it != continuousSkillList.end())
    {
        // 找到对应的索引
        const size_t index = std::distance(continuousSkillList.begin(), it);

        // 从continuousSkillList中移除
        if (index < skillInfoList.size())
        {
            // skillListInfo.erase(skillListInfo.begin() + static_cast<int>(index));
            continuousSkillList.erase(it);
        }

        if (nowReleasingSkill.get() == skill)
        { // 无意义逻辑，但是加上以防万一
            nowReleasingSkill.reset();
            nowReleasingSkill = nullptr;
        }

        // 从 skillList 中移除
        return true;
    }
    return false; // 未找到该技能
}

void Person::cleanupFinishedSkills()
{
    for (auto it = continuousSkillList.begin(); it != continuousSkillList.end();)
    {
        if (!(*it))
        {
            it = continuousSkillList.erase(it);
        }
        else if ((*it)->duration < 0)
        {
            // Logger::debugSkill(AutoAttack::getTimer(),
            //                     (*it)->getSkillName(),
            //                     " - is being removed.");
            it = continuousSkillList.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Person::updateSkillsCD(const int deltaTime)
{
    for (auto &skill : this->skillCDList)
    {
        // 处理技能冷却
        if (skill->getStackRef() < skill->getMaxStack())
        {
            // 处理基础冷却
            processCooling(skill.get(), deltaTime);

            // 处理充能冷却（如果适用）
            if (skill->getCanCharge() && skill->getStackRef() < skill->getMaxStack())
            {
                processChargedCooling(skill.get(), deltaTime);
            }
        }

        // 更新可用状态
        // skill->canUse = (skill->getStack() > 0);
    }
}

void Person::processCooling(Skill *skill, const int deltaTime)
{
    skill->getCurrentCD_Ref() -= deltaTime;
    const int a = this->findSkillInSkillCDList(skill->getSkillName());

    // 处理可能的多次充能（当deltaTime很大时）
    while (skill->getCurrentCD_Ref() <= 0 && skill->getStackRef() < skill->getMaxStack())
    {
        if (a != -1 && static_cast<size_t>(a) < this->skillCDList.size())
        {
            if (this->skillCDList.at(a))
            {
                this->skillCDList.at(a)->getStackRef() += 1;
            }
        }
        skill->getCurrentCD_Ref() += skill->getMaxCD();
    }

    // 如果层数已满，重置计时器
    if (skill->getStackRef() >= skill->getMaxStack())
    {
        skill->getCurrentCD_Ref() = 0;
    }
}

void Person::processChargedCooling(Skill *skill, int deltaTime)
{
    if (skill->getMaxChargedCD() <= 0)
        return; // 没有充能CD

    skill->getCurrentChargedCD_Ref() -= deltaTime;

    // 如果层数已满，重置计时器
    if (skill->getStackRef() >= skill->getMaxStack())
    {
        skill->getCurrentChargedCD_Ref() = 0;
    }
}

// 清空所有技能
void Person::clearSkills()
{
    continuousSkillList.clear();
}

void Person::updateBuffs(int deltaTime)
{
    if (!this->buffList.empty())
    {
        // 使用索引而不是迭代器，避免迭代器失效
        for (size_t i = 0; i < buffList.size(); ++i)
        {
            // 检查指针是否有效
            if (!buffList[i])
            {
                continue;
            }

            buffList[i]->update(deltaTime);
            buffList[i]->reduceDuration(deltaTime);
        }
        cleanupFinishedBuffs();
    }
}

// 清空所有buff
void Person::clearBuffs()
{
    buffList.clear();
}

void Person::cleanupFinishedBuffs()
{
    for (auto it = buffList.begin(); it != buffList.end();)
    {
        if (!(*it))
        {
            it = buffList.erase(it);
        }
        else if ((*it)->shouldBeRemoved())
        {
            Logger::debugBuff(AutoAttack::getTimer(),
                                (*it)->getBuffName(),
                                " - is being removed.");
            it = buffList.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Person::updateAction(int  deltaTime)
{
    unsigned short maxProcessPerTick = this->autoAttackPtr->getMaxProcessPerTick() * deltaTime; // 每tick最多处理事件数
    int processedCount = 0;      // 已处理的事件数

    // AttackAction中skill*原始指针的问题，在使用原始指针时，指向对象已经被释放
    while (processedCount < maxProcessPerTick && !this->actionQueue.empty())
    {
        // 获取队头元素（拷贝或移动）
        auto frontAction = std::move(this->actionQueue.front()); // 移动，避免拷贝
        this->actionQueue.pop();

        // 检查 actionPtr 是否为空
        if (frontAction.actionPtr == nullptr)
        {
            continue; // 跳过空指针
        }

        // 执行操作
        frontAction.actionPtr->execute(frontAction.number, this);
        processedCount++;
    }

    // 注意：deltaTime 参数目前未使用，如果需要按时间限制处理数量，可以考虑使用它
}

bool Person::isSuccess(const double probability) const
{
    if (probability <= 0.0) return false;
    if (probability >= 1.0) return true;

    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(randomEngine) < probability;

    // 生成 1-10000 的随机数，实现 0.01% 的精度
    // int randomValue = rand() % 10000 + 1;                  // 1-10000
    // int threshold = static_cast<int>(probability * 10000); // 将概率转换为 1-10000 的阈值

    // return randomValue <= threshold;
}

bool Person::consumeResource(const int n)
{
    this->resourceNum -= n;
    if(this->resourceNum < 0)
    {
        this->resourceNum = 0;
    }
    return true;
}

bool Person::revertResource(const int n)
{
    this->resourceNum += n;

    if (this->resourceNum > this->maxResourceNum)
    {
        this->resourceNum = this->maxResourceNum;
    }
    return true;
}

bool Person::consumeEnergy(const double n)
{
    this->present_energy -= n;
    if(this->present_energy < 0)
    {
        this->present_energy = 0;
    }
    return true;
}

bool Person::revertEnergy(const double n)
{
    this->present_energy += n;
    if(this->present_energy > this->max_energy)
    {
        this->present_energy = this->max_energy;
    }
    return true;
}

bool Person::reduceSkillCD(std::string skillName, double n)
{
    int a = this->findSkillInSkillCDList(skillName);
    if (a != -1)
    {
        this->skillCDList.at(a)->getCurrentCD_Ref() -= n;
        if (this->skillCDList.at(a)->getCurrentCD_Ref() < 0)
        {
            this->skillCDList.at(a)->getCurrentCD_Ref() = 0;
        }
        return true;
    }
    return false;
}

void Person::addErrorInfoInList(const ErrorInfo &errorinfo)
{
    this->errorInfoList.push_back(errorinfo);
}

int Person::findSkillInSkillCDList(const std::string skillName) const
{
    int i = 0;
    for (auto &skill : this->skillCDList)
    {
        if (skill->getSkillName() == skillName)
        {
            return i;
        }
        i++;
    }
    return -1;  // 未找到
}

int Person::findBuffInBuffList(const std::string buffName) const
{
    int i = 0;
    for (auto &buff : this->buffList)
    {
        if (buff->getBuffName() == buffName)
        {
            return i;
        }
        i++;
    }
    return -1;  // 未找到
}

void Person::createSkill(std::unique_ptr<Skill> newSkill)
{
    if (newSkill->getIsNoReleasing())
    { // 若为无前摇技能，直接用无前摇技能创建函数创建
        this->createNoReleasingSkill(std::move(newSkill));
        return;
    }
    this->nowReleasingSkill = std::move(newSkill);
    // 向autoAttack发送技能释放消息
    this->autoAttackPtr->updateSkillRelease();

    int a = this->findSkillInSkillCDList(this->nowReleasingSkill->getSkillName());
    if (a != -1)
    { // 如果为CD技能，则更新CD和层数
        // 更新技能的层数和CD
        this->skillCDList.at(a)->getStackRef() -= 1;
        if (this->skillCDList.at(a)->getMaxStack() > 1)
        {
            if (this->skillCDList.at(a)->getStackRef() == this->skillCDList.at(a)->getMaxStack() - 1)
            {
                // 若释放前为满层，则重置CD
                this->skillCDList.at(a)->getCurrentCD_Ref() = this->skillCDList.at(a)->getMaxCD();
            }
            // 未满层则不重置CD只重置充能CD
        }
        // 如果有充能CD，则重置充能CD
        if (this->nowReleasingSkill->getCanCharge())
        {
            this->skillCDList.at(a)->getCurrentChargedCD_Ref() = this->skillCDList.at(a)->getMaxChargedCD();
        }
    }

    auto name = this->nowReleasingSkill->getSkillName();
    // 再将其添加到 skillListInfo 中
    skillInfoList.push_back(name);
}

void Person::createNoReleasingSkill(std::unique_ptr<Skill> newSkill)
{
    // 先获取 skillName
    const auto skillName = newSkill->getSkillName();
    // 放入 continuousSkillList
    continuousSkillList.push_back(std::move(newSkill));
    // 再将其添加到 skillListInfo 中
    skillInfoList.push_back(skillName);
}

void Person::createBuff(std::unique_ptr<Buff> buff)
{
    buffList.push_back(std::move(buff));
}

void Person::initializeIncrease()
{
    setAattackIncrease();
    setElementIncrease();
    setAlmightyIncrease();
    setDamageIncrease();
    setCriticalDamage();
    setLuckyFinalIncrease();
    setLuckyDreamIncrease();
    changeCriticalDamage(criticaldamage_set);
    changeDamageIncrease(increase_set);
    changeElementIncreaseByElementIncrease(elementincrease_set);
}

double Person::setAattackIncrease()
{
    this->attackIncrease = 0;
    return this->attackIncrease;
}

double Person::changeAattackIncrease(const double attackIncrease)
{
    this->attackIncrease += attackIncrease;
    return this->attackIncrease;
}

double Person::setElementIncrease()
{
    this->elementIncrease = this->Proficient * this->proficientRatio;
    return this->elementIncrease;
}

double Person::changeElementIncreaseByProficient(const double proficient)
{
    this->elementIncrease -= this->getProficient() * this->proficientRatio * (1 + this->proficientAmplification * this->getProficient());
    this->elementIncrease += this->getProficient() * this->proficientRatio * (1 + this->proficientAmplification * this->getProficient());
    return this->elementIncrease;
}

double Person::changeElementIncreaseByElementIncrease(const double elementIncrease)
{
    this->elementIncrease += elementIncrease;
    return this->elementIncrease;
}

double Person::setAlmightyIncrease()
{
    this->almightyIncrease = this->Almighty * this->almightyRatio;
    return this->almightyIncrease;
}

double Person::changeAlmightyIncrease(const double almighty)
{
    this->almightyIncrease = almighty * this->almightyRatio;
    return this->almightyIncrease;
}

double Person::setDamageIncrease()
{
    this->damageIncrease = 0.0;
    return this->damageIncrease;
}

double Person::changeDamageIncrease(const double increase)
{
    this->damageIncrease += increase;
    return this->damageIncrease;
}

double Person::setEnhenceIncrease()
{
    this->enhenceIncrease = 0.0;
    return this->enhenceIncrease;
}

double Person::changeEnhenceIncrease(const double increase)
{
    this->enhenceIncrease += increase;
    return this->enhenceIncrease;
}

double Person::setCriticalDamage()
{
    this->criticicalDamage = 0.5;
    return this->criticicalDamage;
}

double Person::changeCriticalDamage(const double criticalDamage)
{
    this->criticicalDamage += criticalDamage;
    return this->criticicalDamage;
}

double Person::setLuckyMultiplying()
{
    // 41.25为基础倍率，0.75为幸运每增加1%增加的倍率
    this->luckyMultiplying = (41.25 + (this->Lucky - 0.05) * 100 * 0.25) / 100;
    return this->luckyMultiplying;
}

double Person::changeLuckyMultiplyingByAddMultiplying(const double addMultiplying)
{
    this->luckyMultiplying += addMultiplying;
    return this->luckyMultiplying;
}

double Person::setLuckyFinalIncrease()
{
    this->luckyFinalIncrease = 0;
    return this->luckyFinalIncrease;
}

double Person::changeLuckyFinalIncrease(const double luckyFinalIncrease)
{
    this->luckyFinalIncrease += luckyFinalIncrease;
    return this->luckyFinalIncrease;
}

double Person::setLuckyDreamIncrease()
{
    this->luckyDreamIncrease = 0;
    return this->luckyDreamIncrease;
}

double Person::changeLuckyDreamIncrease(const double luckyDreamIncrease)
{
    this->luckyDreamIncrease += luckyDreamIncrease;
    return this->luckyDreamIncrease;
}

double Person::setDreamIncrease()
{
    this->criticicalDamage = 0;
    return this->criticicalDamage;
}

double Person::changeDreamIncrease(const double dreamIncrease)
{
    this->dreamIncrease += dreamIncrease;
    return this->dreamIncrease;
}

double Person::chanageDamageReduce(const double n)
{
    this->damageReduce += n;
    return this->damageReduce;
}

double Person::changeCriticalCount(const int addCount)
{
    // 1. 获取当前的数值部分（去除额外百分比的影响）
    // 注意：基础暴击率是5%，所以要先减去基础5%和额外百分比
    double basePercent = 0.05;
    double currentCriticalWithoutExtraAndBase = this->Critical - this->CriticalExtraPercent - basePercent;

    // 如果当前暴击率小于基础5%+额外百分比，说明数值部分为0
    if (currentCriticalWithoutExtraAndBase < 0)
    {
        currentCriticalWithoutExtraAndBase = 0;
    }

    double currentCount = this->getCriticalCount(currentCriticalWithoutExtraAndBase);

    // 2. 加上新的数值
    double newCount = currentCount + addCount;

    // 3. 重新计算百分比
    // 数值转换的百分比 = newCount / (newCount + 转换系数)
    double convertedPercent = 0.0;
    if (newCount > 0)
    {
        convertedPercent = newCount / (newCount + this->getPropertyTransformationCoeffcient_General());
    }

    // 4. 更新数值和百分比
    this->CriticalCount = newCount;
    // 面板百分比 = 基础5% + 数值转换的百分比 + 额外百分比
    this->Critical = basePercent + convertedPercent + this->CriticalExtraPercent;
    return this->Critical;
}

double Person::changeQuicknessCount(const int addCount)
{
        // 获取当前数值部分对应的点数（剔除固定百分比）
    double numericPercent = this->Quickness - this->QuicknessExtraPersent;
    if (numericPercent < 0.0) numericPercent = 0.0;
    
    double currentCount = this->getQuicknessCount(numericPercent);
    double newCount = currentCount + addCount;
    if (newCount < 0.0) newCount = 0.0;
    
    // 数值部分转换的百分比
    double convertedPercent = 0.0;
    if (newCount > 0.0) {
        convertedPercent = newCount / (newCount + this->propertyTransformationCoeffcient_General);
    }
    
    // 更新数值点数与面板百分比
    this->QuicknessCount = newCount;
    this->Quickness = convertedPercent + this->QuicknessExtraPersent;
    
    // 重新计算施法速度和攻击速度
    recalcSpeedFromQuickness();
    
    return this->Quickness;
}

double Person::changeLuckyCount(const int addCount)
{
    // 幸运基础百分比为5%
    double basePercent = 0.05;
    double currentLuckyWithoutExtraAndBase = this->Lucky - this->LuckyExtraPersent - basePercent;

    if (currentLuckyWithoutExtraAndBase < 0)
    {
        currentLuckyWithoutExtraAndBase = 0;
    }

    double currentCount = this->getLuckyCount(currentLuckyWithoutExtraAndBase);
    double newCount = currentCount + addCount;

    double convertedPercent = 0.0;
    if (newCount > 0)
    {
        convertedPercent = newCount / (newCount + this->getPropertyTransformationCoeffcient_General());
    }

    this->LuckyCount = newCount;
    this->Lucky = basePercent + convertedPercent + this->LuckyExtraPersent;

    // 更新幸运相关属性
    this->luckyDamageIncrease = this->Lucky;
    this->setLuckyMultiplying();

    // 确保百分比在合理范围内
    if (this->Lucky < basePercent)
    {
        this->Lucky = basePercent;
    }
    return this->Lucky;
}

double Person::changeProficientCount(const int addCount)
{
    // 精通基础百分比为6%
    double basePercent = 0.06;
    double currentProficientWithoutExtraAndBase = this->Proficient - this->ProficientExtraPersent - basePercent;

    if (currentProficientWithoutExtraAndBase < 0)
    {
        currentProficientWithoutExtraAndBase = 0;
    }

    double currentCount = this->getProficientCount(currentProficientWithoutExtraAndBase);
    double newCount = currentCount + addCount;

    double convertedPercent = 0.0;
    if (newCount > 0)
    {
        convertedPercent = newCount / (newCount + this->getPropertyTransformationCoeffcient_General());
    }

    this->ProficientCount = newCount;
    // 更新元素增伤
    this->changeElementIncreaseByProficient(basePercent + convertedPercent + this->ProficientExtraPersent);

    this->Proficient = basePercent + convertedPercent + this->ProficientExtraPersent;

    // 确保百分比在合理范围内
    if (this->Proficient < basePercent)
    {
        this->Proficient = basePercent;
    }
    Logger::debug("Action","proficient changed, now: {0}",std::to_string(this->getProficient()));
    return this->Proficient;
}

double Person::changeAlmightyCount(const int addCount)
{
    // 全能没有基础百分比，且使用不同的转换系数
    double currentAlmightyWithoutExtra = this->Almighty - this->AlmightyExtraPersent;

    if (currentAlmightyWithoutExtra < 0)
    {
        currentAlmightyWithoutExtra = 0;
    }

    double currentCount = this->getAlmightyCount(currentAlmightyWithoutExtra);
    double newCount = currentCount + addCount;

    double convertedPercent = 0.0;
    if (newCount > 0)
    {
        convertedPercent = newCount / (newCount + this->getPropertyTransformationCoeffcient_Almighty());
    }

    this->AlmightyCount = newCount;
    // 更新全能增伤
    this->changeAlmightyIncrease(convertedPercent + this->AlmightyExtraPersent);

    this->Almighty = convertedPercent + this->AlmightyExtraPersent;

    // 确保百分比不为负数
    if (this->Almighty < 0)
    {
        this->Almighty = 0;
    }
    return this->Almighty;
}

double Person::changeCritialPersent(const double persent)
{
    // 1. 增加额外百分比
    this->CriticalExtraPercent += persent;

    // 2. 直接增加面板百分比
    this->Critical += persent;

    // 确保额外百分比不为负数
    if (this->CriticalExtraPercent < 0)
    {
        this->CriticalExtraPercent = 0;
    }

    // 确保面板百分比在合理范围内（0% - 100%）
    if (this->Critical < 0.05)
    {
        this->Critical = 0.05; // 基础5%不能低于
    }
    return this->Critical;
}

double Person::changeQuicknessPersent(const double persent)
{
        // 更新固定百分比
    this->QuicknessExtraPersent += persent;
    // 确保固定部分不为负（但可正可负）
    
    // 重新计算面板百分比（数值部分不变，仅固定部分变化）
    double numericPercent = this->Quickness - (this->QuicknessExtraPersent - persent); // 旧数值部分
    if (numericPercent < 0.0) numericPercent = 0.0;
    double numericCount = this->getQuicknessCount(numericPercent);
    double convertedPercent = 0.0;
    if (numericCount > 0.0) {
        convertedPercent = numericCount / (numericCount + this->propertyTransformationCoeffcient_General);
    }
    this->Quickness = convertedPercent + this->QuicknessExtraPersent;
    
    // 重新计算速度
    recalcSpeedFromQuickness();
    
    return this->Quickness;
}

double Person::changeLuckyPersent(const double persent)
{
    this->LuckyExtraPersent += persent;
    this->Lucky += persent;

    if (this->LuckyExtraPersent < 0)
    {
        this->LuckyExtraPersent = 0;
    }

    // 更新幸运相关属性
    this->luckyDamageIncrease = this->Lucky;
    this->setLuckyMultiplying();

    // 确保幸运不低于基础5%
    double basePercent = 0.05;
    if (this->Lucky < basePercent)
    {
        this->Lucky = basePercent;
    }
    return this->Lucky;
}

double Person::changeProficientPersent(const double persent)
{
    this->ProficientExtraPersent += persent;
    // 更新元素增伤
    this->changeElementIncreaseByProficient(this->Proficient + persent);

    this->Proficient += persent;

    if (this->ProficientExtraPersent < 0)
    {
        this->ProficientExtraPersent = 0;
    }

    // 确保精通不低于基础6%
    double basePercent = 0.06;
    if (this->Proficient < basePercent)
    {
        this->Proficient = basePercent;
    }
    Logger::debug("Action","proficient changed, now: {0}",std::to_string(this->getProficient()));
    return this->Proficient;
}

double Person::changeAlmightyPersent(const double persent)
{
    this->AlmightyExtraPersent += persent;
    // 更新全能增伤
    this->changeAlmightyIncrease(this->Almighty + persent);
    this->Almighty += persent;

    if (this->AlmightyExtraPersent < 0)
    {
        this->AlmightyExtraPersent = 0;
    }

    // 确保全能不为负数
    if (this->Almighty < 0)
    {
        this->Almighty = 0;
    }
    return this->Almighty;
}

void Person::addCastingSpeed(double persent)
{
    this->castingSpeedExtra += persent;
    // 重新计算最终施法速度（基础部分不变）
    this->castingSpeed = this->baseCastingSpeed + this->castingSpeedExtra;
}

void Person::addAttackSpeed(double persent)
{
    this->attackSpeedExtra += persent;
    // 重新计算最终攻击速度（基础部分不变）
    this->attackSpeed = this->baseAttackSpeed + this->attackSpeedExtra;
}

double Person::changePrimaryAttributesByCount(const double primaryAttributesCount)
{
    // 直接增加主属性数值
    this->ATK -= (1 + this->primaryAttributesIncrease) * this->primaryAttributes * this->primaryAttributeRatio;
    this->primaryAttributes += primaryAttributesCount;
    this->resetATK();
    return this->primaryAttributes;
}

double Person::changePrimaryAttributesByPersent(const double primaryAttributesPersent)
{
    this->ATK -= (1 + this->primaryAttributesIncrease)  * this->primaryAttributes * this->primaryAttributeRatio;
    this->primaryAttributesIncrease += primaryAttributesPersent;
    this->resetATK();
    return this->primaryAttributesIncrease;
}

double Person::changeCastingSpeedByPersent(const double castingSpeedPersent)
{
    this->castingSpeedExtra += castingSpeedPersent;
    this->castingSpeed = this->baseCastingSpeed + this->castingSpeedExtra;
    return this->castingSpeedExtra;
}

double Person::setATK(double atk)
{
    // 计算基础攻击力（去除智力转化）
    this->baseATK = atk - this->primaryAttributes * this->primaryAttributeRatio * (1 + this->primaryAttributesIncrease);
    return this->baseATK;
}

double Person::resetATK()
{
    // ATK = 基础攻击力 + 主属性转化的攻击力 × (1 + 主属性增加百分比)
    this->ATK = this->baseATK 
                + this->primaryAttributes * this->primaryAttributeRatio 
                * (1 + this->primaryAttributesIncrease);
    return this->ATK;
}

double Person::changeATKCount(double count)
{
    this->ATK += count;
    return this->ATK;
}

double Person::changeRefineATKCount(double n)
{
    this->refineATK += n;
    return this->refineATK;
}

double Person::changeAttackSpeedByPersent(const double attackSpeedPersent)
{
    this->attackSpeedExtra += attackSpeedPersent;
    this->attackSpeed = this->baseAttackSpeed + this->attackSpeedExtra;
    return this->attackSpeedExtra;
}

int Person::getCriticalCount(const double critical)
{
    const double y = critical;
    return static_cast<int>((this->propertyTransformationCoeffcient_General * y) / (1 - y));
}

int Person::getQuicknessCount(const double quickness)
{
    const double y = quickness;
    return static_cast<int>((this->propertyTransformationCoeffcient_General * y) / (1 - y));
}

int Person::getLuckyCount(const double lucky)
{
    const double y = lucky;
    return static_cast<int>((this->propertyTransformationCoeffcient_General * y) / (1 - y));
}

int Person::getProficientCount(const double proficient)
{
    const double y = proficient;    
    return static_cast<int>((this->propertyTransformationCoeffcient_General * y) / (1 - y));
}

int Person::getAlmightyCount(const double almighty)
{
    const double y = almighty;
    return static_cast<int>((this->propertyTransformationCoeffcient_Almighty * y) / (1 - y));
}

auto Person::calculateDamageStatistics() -> decltype(this->damageStatsMap)
{
    for (auto &info : damageInfoList)
    {
        auto it = damageStatsMap.find(info.skillName);
        if (it != damageStatsMap.end())
        {
            // 已存在该技能的统计，更新数据
            it->second.damageInfoAppend(info);
        }
        else
        {
            // 不存在该技能的统计，创建新的统计对象
            DamageStatistics newStats(this->totalTime);
            newStats.skillName = info.skillName;
            newStats.damageInfoAppend(info);
            damageStatsMap[info.skillName] = newStats;
        }
    }

    // 计算技能释放次数
    auto buff = this->findBuffInBuffList(SkillReleasedTimesStatistics::name);
    if (buff != -1)
    {
        auto *statisticsBuff = dynamic_cast<SkillReleasedTimesStatistics *>(this->buffList.at(buff).get());
        if (statisticsBuff)
        {
            for (const auto &i : statisticsBuff->skillReleasedTimesMap)
            {
                const std::string &skillName = i.first;
                int releaseTimes = statisticsBuff->skillReleasedTimesMap[skillName];
                damageStatsMap[skillName].releasedTimes = releaseTimes;
            }
        }
    }


    return damageStatsMap;
}

void Person::equipSkill(std::string skillName)
{
    auto it = SkillCreator::createSkill(skillName, this);
    if (it)
    {
        it->isEquipped = true;
        // 检查是否为 FightingFantasy 类型
        if (auto *fantasySkill = dynamic_cast<FightingFantasy *>(it.get()))
        {
            fantasySkill->setPassiveEffect(this);
        }

        this->skillCDList.push_back(std::move(it));
        Logger::debug("Person","skill: " + skillName + " equipped");
    }
}

void Person::equipInherentBuff(std::string buffName)
{
    auto it = BuffCreator::createBuff(buffName, this);
    if (it)
    {
        it->duration = 999999;
        it->maxDuration = it->duration;
        this->createBuff(std::move(it));
    }
}

void Person::pushDamgeInfo(DamageInfo& info) { this->damageInfoList.push_back(info); }

void Person::setRandomSeed(std::uint32_t seed)
{
    randomEngine.seed(seed);
}

double Person::getAttributeRatio() const { return this->primaryAttributeRatio; }
int Person::getPropertyTransformationCoeffcient_General() const { return this->propertyTransformationCoeffcient_General; }
int Person::getPropertyTransformationCoeffcient_Almighty() const { return this->propertyTransformationCoeffcient_Almighty; }

const std::vector<std::unique_ptr<Buff>> &Person::getBuffListRef() const { return this->buffList; }
const std::vector<std::unique_ptr<Skill>> &Person::getContinuousSkillListRef() const { return this->continuousSkillList; }
const std::vector<std::unique_ptr<Skill>> &Person::getNoReleasingSkillListRef() const { return this->noReleasingSkillList; }
const std::vector<std::unique_ptr<Skill>> &Person::getSkillCDListRef() const { return this->skillCDList; }
const std::queue<ActionInfo> &Person::getActionQueueRef() const { return this->actionQueue; }
const std::vector<DamageInfo> &Person::getDamageListInfoRef() const { return this->damageInfoList; }
const std::vector<std::string> &Person::getSkillListInfoRef() const { return this->skillInfoList; }
const std::vector<ErrorInfo> &Person::getErrorInfoListRef() const { return this->errorInfoList; }

int Person::getTotalTime() const { return totalTime; }
double Person::getMaxEnergy() const { return max_energy; }
double Person::getPresentEnergy() const { return present_energy; }
double Person::getCritical() const { return Critical; }
double Person::getLucky() const { return Lucky; }
double Person::getAlmighty() const { return Almighty; }
double Person::getProficient() const { return Proficient; }
double Person::getQuickness() const { return Quickness; }
double Person::getCriticalCount() const { return CriticalCount; }
double Person::getLuckyCount() const { return LuckyCount; }
double Person::getAlmightyCount() const { return AlmightyCount; }
double Person::getProficientCount() const { return ProficientCount; }
double Person::getQuicknessCount() const { return QuicknessCount; }

double Person::getATK() const { return ATK; }
double Person::getRefineATK() const { return refineATK; }
double Person::getElementATK() const { return elementATK; }
double Person::getPrimaryAttributes() const { return primaryAttributes * (1 + primaryAttributesIncrease); }
int Person::getResourceNum() const { return resourceNum; }
int Person::getMaxResourceNum() const { return maxResourceNum; }
double Person::getCastingSpeed() const { return castingSpeed; }
double Person::getAttackSpeed() const { return attackSpeed; }

double Person::getPrimaryAttributesIncrease() const { return primaryAttributesIncrease; }
double Person::getAttackIncrease() const { return attackIncrease; }
double Person::getDamageIncrease() const { return damageIncrease; }
double Person::getElementIncrease() const { return elementIncrease; }
double Person::getAlmightyIncrease() const { return almightyIncrease; }
double Person::getCriticicalDamage() const { return criticicalDamage; }
double Person::getEnhenceIncrease() const { return enhenceIncrease; }
double Person::getDamageReduce() const { return damageReduce; }
double Person::getFinalIncrease() const { return finalIncrease; }
double Person::getDreamIncrease() const { return dreamIncrease; }

double Person::getLuckyDamageIncrease() const { return luckyDamageIncrease; }
double Person::getLuckyMultiplying() const { return luckyMultiplying; }
double Person::getLuckyMultiplyingExtra() const { return luckyMultiplyingExtra; }
double Person::getLuckyFinalIncrease() const { return luckyFinalIncrease; }
double Person::getLuckyDreamIncrease() const { return luckyDreamIncrease; }

double Person::getProficientRatio() const { return proficientRatio; }
double Person::getAlmightyRatio() const { return almightyRatio; }
double Person::getCastingSpeedRatio() const { return castingSpeedRatio; }
double Person::getAttackSpeedRatio() const { return attackSpeedRatio; }
double Person::getProficientAmplification() const { return proficientAmplification; }

bool Person::getIsReleasingSkill() const { return isReleasingSkill; }

double Person::getEnergyAddIncrease() const { return energyAddIncrease; }
double Person::getEnergyReduceUP() const { return energyReduceUP; }
double Person::getEnergyReduceDOWN() const { return energyReduceDOWN; }

const AutoAttack *Person::getAutoAttack() const { return this->autoAttackPtr.get(); }
Skill* const Person::getNowReleasingSkill() const { return this->nowReleasingSkill.get(); }
void Person::clearNowReleasingSkill() { this->nowReleasingSkill.reset(); }

const Skill* Person::getCurtainPointerForAction(std::string skillName) const 
{
    for (const auto& skillPtr : this->pointerListForAction) 
    {
        if (skillPtr && skillPtr->getSkillName() == skillName) 
        {
            return skillPtr.get();
        }
    }
    return nullptr;
}

void Person::recalcSpeedFromQuickness() 
{
    // 基础施法速度 = 急速面板 × 转化系数
    this->baseCastingSpeed = this->castingSpeedRatio * this->Quickness;
    this->baseAttackSpeed  = this->attackSpeedRatio * this->Quickness;
    
    // 最终施法速度 = 基础 + 固定加成
    this->castingSpeed = this->baseCastingSpeed + this->castingSpeedExtra;
    this->attackSpeed  = this->baseAttackSpeed  + this->attackSpeedExtra;
}