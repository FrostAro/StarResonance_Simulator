#pragma once
#include "Logger.h"
#include "Person.h"
#include "Skill.h"
#include "Buff.h"
#include "AutoAttack.h"
#include "Creators.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

/* ============================================================================
 * 固有Buff标记的编译期探测
 *
 * 约定：固有Buff（常驻，如因子/职业被动）在类内声明
 *           static constexpr bool kInherent = true;
 * 本工具在编译期探测该成员：
 *     - 声明了 kInherent = true 的类 -> true
 *     - 未声明（含旧代码/外部扩展的Buff）-> false（SFINAE 回退，不会编译失败）
 * 因此新增固有Buff时只需在类内加一行标记，无需构造实例去问 isInherent，
 * 也不会再污染全局 Buff::ID / 静态监听器表。
 * ============================================================================
 */
template <typename T, typename = void>
struct is_inherent_buff : std::false_type {};

template <typename T>
struct is_inherent_buff<T, std::void_t<decltype(T::kInherent)>>
    : std::bool_constant<static_cast<bool>(T::kInherent)> {};

template <typename T>
inline constexpr bool is_inherent_buff_v = is_inherent_buff<T>::value;

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
    int m_fantasyConfig = 0;          ///< 额外的幻想配置参数，供子类使用
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
     * @tparam Buff_ 要注册的Buff类类型
     * 
     * 将Buff类型登记到BuffCreator；若为固有Buff（常驻，如因子/职业被动），
     * 则把名字加入 inherentBuffs，稍后由 initializePerson() 统一创建。
     * 
     * @note 是否固有由类内 `static constexpr bool kInherent` 在编译期探测（见文件顶部
     *       is_inherent_buff_v），不再构造临时 Buff/Person 实例去读 isInherent。
     *       原因：Buff 构造函数普遍带副作用（注册监听器、修改 p 的属性、递增全局 Buff::ID），
     *       为探测一个 bool 而构造临时对象会污染全局状态、在静态监听器表里留下悬挂回调，
     *       并让 Buff ID 分配变得顺序敏感。
     *       Debug 构建下 Initialize() 末尾会断言"构造函数设了 isInherent 的Buff必须声明
     *       kInherent = true"，忘记标记会在开发期直接暴露。
     */
    template<typename Buff_>
    void registerCertainBuff()
    {
        // 注册到BuffCreator的创建器映射表
        BuffCreator::creatorMap.insert({Buff_::name,
                        [](Person* p,double n = 0) { return std::make_unique<Buff_>(p,n); }});
        
        if constexpr (is_inherent_buff_v<Buff_>)
        {
            Logger::debug("Initializer",
                          "Inherent Buff Registered: " + Buff_::name);
            this->inherentBuffs.emplace_back(Buff_::name);
        }
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
        registerCertainBuff<SkillReleasedTimesStatistics>();
        this->registerSkills();
        this->registerBuffs();
        this->equipSkills();
        this->initializePerson();
        this->initializeAutoAttack();

#ifndef NDEBUG
        checkInherentFlagConsistency();
#endif
    }

#ifndef NDEBUG
    /**
     * @brief 校验 kInherent 标记与实际 isInherent 一致（仅 Debug）
     *
     * 覆盖两个方向的遗漏，避免"新写了固有Buff但忘记加 kInherent = true"这类静默回归：
     *   1. 注册为固有、但构造函数没有设置 isInherent；
     *   2. 构造函数设置了 isInherent，但该类没有声明 kInherent（于是没被注册为固有）。
     */
    void checkInherentFlagConsistency() const
    {
        for (const auto& name : inherentBuffs)
        {
            int idx = this->p->findBuffInBuffList(name);
            if (idx == -1)
                continue;
            const Buff* buff = this->p->getBuffListRef().at(idx).get();
            if (buff && !buff->getIsInherent())
            {
                Logger::error("Initializer",
                              "kInherent/isInherent 不一致（应为非固有，却标记为固有）: " + name);
            }
            assert(buff && buff->getIsInherent() &&
                   "该Buff声明了 kInherent=true，但构造函数没有设置 isInherent=true");
        }

        for (const auto& buff : this->p->getBuffListRef())
        {
            if (!buff || !buff->getIsInherent())
                continue;
            const std::string& name = buff->getBuffName();
            if (std::find(inherentBuffs.begin(), inherentBuffs.end(), name) == inherentBuffs.end())
            {
                Logger::error("Initializer",
                              "kInherent/isInherent 不一致（构造函数设为固有，却缺少 kInherent 标记）: " + name);
                assert(false &&
                       "该Buff构造函数设置了 isInherent=true，但类内缺少 static constexpr bool kInherent = true;");
            }
        }
    }
#endif

    /**
     * @brief 构造函数
     * @param p 要初始化的角色指针
     */
    Initializer(Person* p, double deltaTime, int fantasyConfig)
                : p(p),deltaTime(deltaTime), m_fantasyConfig(fantasyConfig){}

    virtual ~Initializer() = default;
};
