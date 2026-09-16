#include "AutoAttack.h"
#include "Person.h"
#include "Action.h"
#include "Logger.h"
#include <string_view>

// 静态成员初始化
double AutoAttack::timer = 0;
double AutoAttack::deltaTime = 10;  // 1 tick = 1ms

// 构造函数
AutoAttack::AutoAttack(Person *p)
    : p(p)
{
    timer = 0; // 初始化时重置全局计时器
}

// 快速查找技能索引
int AutoAttack::getSkillIndex(const std::string &skillName)
{
    // 1. 先尝试从缓存中获取
    auto it = skillIndexCache.find(skillName);
    if (it != skillIndexCache.end())
    {
        // 验证索引是否有效（防止越界）
        if (it->second >= 0 && static_cast<size_t>(it->second) < p->getSkillCDListRef().size())
        {
            // 验证技能名称是否匹配（防止技能列表变化）
            if (p->getSkillCDListRef().at(it->second)->getSkillName() == skillName)
            {
                return it->second;
            }
        }
        // 如果无效，从缓存中移除
        skillIndexCache.erase(it);
    }
    
    // 2. 缓存未命中，执行原始查找
    int index = p->findSkillInSkillCDList(skillName);
    if (index != -1)
    {
        // 添加到缓存
        skillIndexCache[skillName] = index;
    }
    
    return index;
}

// 手动添加优先级技能（允许重复）
void AutoAttack::maniAddPriorSkillList(std::string skillName)
{
    auto priorSkill = PriorSkillInfo(skillName, false, true);
    this->priorSkillList.push_back(priorSkill);
}

// 自动添加优先级技能（去重）
void AutoAttack::autoAddPriorSkillList(std::string skillName)
{
    for (auto &i : this->priorSkillList)
    {
        if (i.skillName == skillName)
        {
            return; // 已存在，直接返回
        }
    }
    auto priorSkill = PriorSkillInfo(skillName, true, false);
    this->priorSkillList.push_back(priorSkill);
}

// 结束爆发状态
void AutoAttack::finishOutBurst()
{
    this->isOutBurst = false;
    this->resetPriority();
    Logger::debugAutoAttack(this->timer, "Finish Out Burst");
}

// 更改技能优先级
void AutoAttack::changePriority(std::string skillName, int priority)
{
    for (auto &i : this->skillPriority)
    {
        if (i.skillName == skillName)
        {
            i.priority = priority;
            // 优先级更新后需要重新排序
            std::stable_sort(this->skillPriority.begin(), this->skillPriority.end(),
                             [](const PriorSkillInfo &a, const PriorSkillInfo &b)
                             {
                                 return a.priority > b.priority; // 降序排序，优先级高的在前
                             });
            return;
        }
    }
}

// 自动释放技能的核心逻辑
void AutoAttack::createSkillByAuto()
{
    // 优先检查certainTimeSkillList
    if(!certainTimeSkillList.empty())
    {
        auto& a = certainTimeSkillList.front();
        if(this->timer >= a.time)
        {
            std::string skillName = a.skillName;
            this->p->triggerAction<CreateSkillAction>(0,skillName);
            certainTimeSkillList.pop();
            Logger::debugAutoAttack(this->timer, "certain time skill created, Skill: " + skillName);
            return;
        }
    }


    // 1. 检查是否正在释放技能（防止打断）
    if (this->isReleasingSkill)
    {
        bool end = true;
        if(!this->priorSkillList.empty())
        {
            // 如果优先队列的第一个技能是已装备的无前摇技能，释放此技能
            for(std::string_view a : this->equippedAndNoReleasingTimeSkill)
            {
                if(a == this->priorSkillList[0].skillName)
                    end = false;
            }
        }
        if(end)
        {
            this->p->addErrorInfoInList(ErrorInfo(
            "NONE", ErrorInfo::errorTypeEnum::Now_Releasing_Skill));
            return;
        }
    }

    // 2. 优先处理待释放的优先级技能列表
    if (!this->priorSkillList.empty())
    {
        auto willSkillName = this->priorSkillList.at(0).skillName;
        this->p->triggerAction<CreateSkillAction>(0, willSkillName);
        this->priorSkillList.erase(this->priorSkillList.begin()); // 删除已处理的技能
        return;
    }

    // 3. 按优先级列表寻找可释放的技能
    for (auto &i : this->skillPriority)
    {
        // 只处理优先级大于0的技能
        if (i.priority <= 0)
            // 优先级列表默认有序，<=0直接返回
            return;

        // 查找技能在CD列表中的索引
        int a = getSkillIndex(i.skillName);  // 使用缓存替代原始查找
        if (a == -1)
        {
            this->p->addErrorInfoInList(
                ErrorInfo(i.skillName, ErrorInfo::errorTypeEnum::Skill_Not_Found));
            return;
        }

        // 检查技能层数是否足够
        if (this->p->getSkillCDListRef().at(a)->getStackRef() < 1)
        {
            this->p->addErrorInfoInList(
                ErrorInfo(i.skillName, ErrorInfo::errorTypeEnum::Stack_Not_Enough));
            continue;
        }

        // 区分非充能技能和充能技能的CD检查
        if (!this->p->getSkillCDListRef().at(a)->getCanCharge())
        {
            // 非充能技能：检查普通CD
            if (this->p->getSkillCDListRef().at(a)->getCurrentCD_Value() > 0)
            {
                this->p->addErrorInfoInList(
                    ErrorInfo(i.skillName, ErrorInfo::errorTypeEnum::CD_Not_Enough));
                continue;
            }
        }
        else
        {
            // 充能技能：检查充能CD
            if (this->p->skillCDList[a]->getCurrentChargedCD_Value() > 0)
            {
                this->p->addErrorInfoInList(
                    ErrorInfo(i.skillName, ErrorInfo::errorTypeEnum::ChargeCD_Not_Enough));
                continue;
            }
        }

        // 4. 检查额外的释放条件
        bool canCreateSkill = true;
        for (auto &judgeConditionStruct : this->judgingConditionsForCreateSkill)
        {
            if (judgeConditionStruct.condition(i))
            {
                // 条件不满足，记录错误并跳过该技能
                this->p->addErrorInfoInList(
                    ErrorInfo(i.skillName, judgeConditionStruct.errorType));
                canCreateSkill = false;
                break; // 跳出条件检查循环
            }
        }

        if (!canCreateSkill)
        {
            continue; // 条件不满足，检查下一个技能
        }

        // 5. 所有条件满足，添加到待释放列表（自动去重）
        this->autoAddPriorSkillList(i.skillName);
        break; // 只处理一个技能，找到后即退出循环
    }
}

void AutoAttack::updatePerson(int deltaTime)
{
    this->p->updateBuffs(deltaTime);
    this->p->updateSkills(deltaTime);
    this->p->updateSkillsCD(deltaTime);
    //this->p->updateAction(deltaTime);
}

void AutoAttack::createSkillInCertainTime(double time, std::string skillName)
{
    certainTimeSkillList.push({skillName,time});
}

// 添加技能释放的禁止条件
void AutoAttack::addJudgingConditionsForCantCreateSkill(std::function<bool(const PriorSkillInfo &)> condition,
                                                        ErrorInfo::errorTypeEnum errorType)
{
    this->judgingConditionsForCreateSkill.push_back(judgeConditionStruct{condition, errorType});
}

// 重置所有技能优先级为0
void AutoAttack::resetPriority()
{
    for (auto &i : this->skillPriority)
    {
        i.priority = 0;
    }
}

// 重置指定技能优先级为0
void AutoAttack::resetCertainPriority(std::string skillName)
{
    for (auto &i : this->skillPriority)
    {
        if (i.skillName == skillName)
        {
            i.priority = 0;
        }
    }
}

// 更新技能状态为"正在释放"
void AutoAttack::updateSkillRelease()
{
    this->isReleasingSkill = true;
    this->p->isReleasingSkill = true;
}

// 更新技能状态为"释放完成"
void AutoAttack::updateSkillFinish()
{
    this->isReleasingSkill = false;
    this->p->isReleasingSkill = false;
}

// 访问器实现
double AutoAttack::getTimer() { return AutoAttack::timer; }
double &AutoAttack::setTimer() { return AutoAttack::timer; }
double AutoAttack::getDeltaTime() { return AutoAttack::deltaTime; }
bool AutoAttack::getIsReleasingSkill() const { return this->isReleasingSkill; }
bool AutoAttack::getIsOutBurst() const { return this->isOutBurst; }
int AutoAttack::getNextOutBurstType() const { return this->nextOutBurstType; }
unsigned short AutoAttack::getMaxProcessPerTick() const { return this->maxProcessPerTick; }