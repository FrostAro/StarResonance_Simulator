#pragma once
#include "Logger.h"
#include "Person.h"
#include "Skill.h"
#include "AutoAttack.h"
#include "Creators.hpp"
#include <memory>
#include <vector>

// 临时的person类，用于初始化buff判断buff是否为固定
class temp_Person : public Person
{   
public:
    temp_Person() : Person(){};
    ~temp_Person(){};
};

/* ============================================================================
 * @class Initializer
 * @brief 游戏初始化器基类，负责角色和战斗系统的初始化
 * 
 * 主要职责：
 * 1. 注册所有技能和Buff类型到创建器
 * 2. 装备角色的初始技能和Buff
 * 3. 配置自动攻击系统的技能优先级
 * 4. 设置角色的初始属性状态
 * 
 * 设计模式：模板方法模式
 * 子类通过重写虚函数来配置具体的技能和Buff。
 * 
 * 初始化流程：
 * 1. registerSkills() - 注册技能类型
 * 2. registerBuffs() - 注册Buff类型
 * 3. equipSkills() - 装备初始技能
 * 4. initializePerson() - 初始化角色属性
 * 5. initializeAutoAttack() - 初始化自动攻击系统
 * ============================================================================
 */
class Initializer
{
protected:
    double deltaTime = 0;
    Person* p;                        ///< 要初始化的角色指针
    std::vector<std::string> equippedSkills{};      ///< 已装备技能名称列表
    std::vector<std::string> inherentBuffs{};       ///< 固有Buff名称列表

    /**
     * @brief 装备技能虚函数，子类必须重写
     * 
     * 在此方法中调用equipCertainSkill()来装备角色的初始技能。
     */
    virtual void equipSkills() = 0;
    
    /**
     * @brief 注册技能虚函数，子类必须重写
     * 
     * 在此方法中调用registerCertainSkill()来注册所有需要的技能类型。
     */
    virtual void registerSkills() = 0;
    
    /**
     * @brief 注册Buff虚函数，子类必须重写
     * 
     * 在此方法中调用registerCertainBuff()来注册所有需要的Buff类型。
     */
    virtual void registerBuffs() = 0;

    /**
     * @brief 装备特定技能
     * @param skillName 要装备的技能名称
     * 
     * 将技能名称添加到equippedSkills列表，用于后续创建和装备。
     */
    void equipCertainSkill(std::string skillName)
    {
        this->equippedSkills.emplace_back(skillName);
        Logger::debug("Initializer",
                      "Equipped skill: " + skillName);
    }

    /**
     * @brief 注册特定技能类型
     * @tparam Skill_ 技能类类型（带下划线避免与Skill类冲突）
     * 
     * 将技能类型注册到SkillCreator，同时创建临时实例添加到角色指针列表。
     */
    template<typename Skill_>
    void registerCertainSkill()
    {
        // 注册到SkillCreator的创建器映射表
        SkillCreator::creatorMap.insert({Skill_::name,
                                [](Person* p) { return std::make_unique<Skill_>(p); }});
        Logger::debug("Initializer",
                      "Registered skill: " + Skill_::name);
        
        // 添加到角色的指针列表，用于Action系统获取技能指针(弃用)
        // p->pointerListForAction.push_back(
        //         std::make_unique<Skill_>(p));
    }

    /**
     * @brief 注册特定Buff类型
     * @tparam Buff_ Buff类类型
     * 
     * 将Buff类型注册到BuffCreator，同时检查是否为固有Buff。
     */
    template<typename Buff_>
    void registerCertainBuff()
    {
        // 注册到BuffCreator的创建器映射表
        BuffCreator::creatorMap.insert({Buff_::name,
                        [](Person* p,double n = 0) { return std::make_unique<Buff_>(p,n); }});
        
        // 检查是否为固有Buff
        // 创建一个临时的person
        std::unique_ptr<temp_Person> temp_p = std::make_unique<temp_Person>();
        bool isInherent = false;
        auto temp = std::make_unique<Buff_>(temp_p.get(),0);
        isInherent = temp->getIsInherent();
        
        if(isInherent)
        {
            Logger::debug("Initializer",
                          "Inherent Buff Registered: " + Buff_::name);
            this->inherentBuffs.emplace_back(temp->getBuffName());
        }
        
        // 重置Buff ID计数器，避免测试实例影响正式ID分配
        Buff::resetID();
        temp.reset();
    }

    /**
     * @brief 初始化自动攻击系统
     * 
     * 配置技能优先级列表和初始资源状态。
     */
    void initializeAutoAttack()
    {
        AutoAttack::deltaTime = this->deltaTime;
        // 初始化自动攻击系统技能优先级列表与无前摇技能列表
        // 创建一个临时的person
        std::unique_ptr<temp_Person> temp_p = std::make_unique<temp_Person>();
        for(auto & skillName : equippedSkills)
        {
            this->p->autoAttackPtr->skillPriority.push_back(
                            PriorSkillInfo(skillName, 0));
            
            auto temp_skill = SkillCreator::createSkill(skillName,temp_p.get());
            if(temp_skill->getIsNoReleasing())
            {
                this->p->autoAttackPtr->equippedAndNoReleasingTimeSkill.push_back(skillName);
            }
        }
        temp_p.reset();

        // 重置角色资源数
        this->p->resourceNum = this->p->maxResourceNum;

        // 对优先级进行排序（降序）
        std::stable_sort(this->p->autoAttackPtr->skillPriority.begin(), 
              this->p->autoAttackPtr->skillPriority.end(),
              [](const PriorSkillInfo &a, const PriorSkillInfo &b) -> bool
              {
                  return a.priority > b.priority; // 降序排序
              });
    }

    /**
     * @brief 初始化角色
     * 
     * 设置角色属性、装备技能、重置技能层数、创建固有Buff。
     */
    void initializePerson()
    {
        // 装备技能
        for(auto & skillName : equippedSkills)
        {
            this->p->equipSkill(skillName);
        }

        // 设置冷却缩减
        for(auto& skill : this->p->getSkillCDListRef())
        {
            skill->MaxCD *= (1 - this->p->coolDownReduce);
        }


        // 重置技能层数
        for (const auto &skill : this->p->getSkillCDListRef())
        {
            skill->getStackRef() = skill->getMaxStack();
        }

        // 创建固有Buff
        for(auto & buffName : inherentBuffs)
        {
                this->p->equipInherentBuff(buffName);
        }
        this->p->resetATK();
    }

public:
    /**
     * @brief 执行完整初始化流程
     * 
     * 调用顺序：
     * 1. registerSkills()
     * 2. registerBuffs()
     * 3. equipSkills()
     * 4. initializePerson()
     * 5. initializeAutoAttack()
     */
    void Initialize()
    {
        this->registerSkills();
        this->registerBuffs();
        this->equipSkills();
        this->initializePerson();
        this->initializeAutoAttack();
    }

    /**
     * @brief 构造函数
     * @param p 要初始化的角色指针
     */
    Initializer(Person* p, double deltaTime)
                : p(p),deltaTime(deltaTime){}

    virtual ~Initializer() = default;
};
