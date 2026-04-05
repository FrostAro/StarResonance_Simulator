#pragma once
#include "Listener.hpp"
#include <string>
#include <vector>
#include <memory>

// 前向声明
class DamageInfo;
class Person;
class DamageListener;
class ResourceListener;
class EnergyListener;
class CDListener;
class Skill;
class CreateSkillListener;
class CreateBuffListener;
class SecondaryAttributeListener;
class SimulationWorker;

/* ============================================================================
 * @class Action
 * @brief 动作系统基类，实现游戏事件触发机制
 * 
 * 核心设计模式：观察者模式
 * 主要职责：
 * 1. 定义游戏事件的标准接口
 * 2. 管理事件监听器的注册和删除
 * 3. 触发事件时通知所有注册的监听器
 * 
 * 工作流程：
 * 1. Person调用triggerAction()触发事件
 * 2. triggerAction()创建Action对象并推入队列
 * 3. Person::updateAction()调用execute()
 * 4. execute()遍历监听器并触发回调
 * 
 * 特殊事件处理流程（创建技能/Buff）：
 * 1. 调用SkillCreator::createSkill()或BuffCreator::createBuff()
 * 2. 调用Person::createSkill()或Person::createBuff()
 * 3. 调用triggerAction<CreateSkillAction>()或triggerAction<CreateBuffAction>()
 * ============================================================================
 */

/**
 * @brief 动作系统基类，所有具体动作的抽象接口
 * 
 * 采用多态设计，通过execute()方法执行具体动作逻辑。
 * 支持移动语义，禁止拷贝（因为通常通过智能指针管理）。
 */
class Action
{
private:
    static std::string name;  ///< 动作名称（静态成员，所有实例共享）
public:
    Action() = default;
    
    // 移动构造函数
    Action(Action &&) = default;
    Action &operator=(Action &&) = default;
    
    // 禁止拷贝（因为Action是多态基类，通常通过指针使用）
    Action(const Action &) = delete;
    Action &operator=(const Action &) = delete;

    virtual ~Action() = default;
    
    /**
     * @brief 执行动作的纯虚函数
     * @param n 动作参数（如伤害值、冷却减少量等）
     * @param p 执行动作的角色指针
     */
    virtual void execute(double n, Person *p) = 0;
    
    /**
     * @brief 获取动作名称
     * @return 动作名称字符串
     */
    virtual std::string getActionName();
};

/* ============================================================================
 * 具体动作类定义
 * 每个动作类包含：
 * 1. 静态名称和监听器列表
 * 2. 监听器管理方法（addListener/deleteListener）
 * 3. execute()方法实现具体逻辑
 * ============================================================================
 */

/**
 * @class AttackAction
 * @brief 攻击动作，触发技能攻击并计算伤害
 * 
 * 执行流程：
 * 1. 获取技能指针
 * 2. 计算伤害信息
 * 3. 通知所有伤害监听器
 * 4. 记录伤害信息
 */
class AttackAction : public Action
{
public:
    static std::string name;        ///< 动作名称："AttackAction"
    static std::vector<std::unique_ptr<DamageListener>> listeners;  ///< 伤害监听器列表
    const Skill* const skill;       ///< 技能指针

public:
    /**
     * @brief 构造函数
     * @param skill 要攻击的技能
     */
    explicit AttackAction(const Skill* const skill);

    virtual void execute(double n, Person *p) override; // double n未使用
    static void addListener(std::unique_ptr<DamageListener> listener);
    static void deleteListener(int buffID); // 通过buffID删除监听器
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    std::string getActionName() override;
};

/**
 * @class ResourceConsumeAction
 * @brief 资源消耗动作，消耗玄冰资源
 */
class ResourceConsumeAction : public Action
{
private:
    static std::string name;  ///< 动作名称："ResourceConsumeAction"
    static std::vector<std::unique_ptr<ResourceListener>> listeners;  ///< 资源监听器列表

public:
    ResourceConsumeAction() : Action() {};
    
    /**
     * @brief 执行资源消耗
     * @param n 消耗的资源数量
     * @param p 执行动作的角色
     */
    virtual void execute(double n, Person *p) override;
    static void addListener(std::unique_ptr<ResourceListener> info);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    std::string getActionName() override;
};

/**
 * @class ResourceRevertAction
 * @brief 资源回复动作，回复玄冰资源
 */
class ResourceRevertAction : public Action
{
private:
    static std::string name;  ///< 动作名称："ResourceRevertAction"
    static std::vector<std::unique_ptr<ResourceListener>> listeners;  ///< 资源监听器列表

public:
    ResourceRevertAction() : Action() {};
    
    /**
     * @brief 执行资源回复
     * @param n 回复的资源数量
     * @param p 执行动作的角色
     */
    virtual void execute(double n, Person *p) override;
    static void addListener(std::unique_ptr<ResourceListener> info);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    std::string getActionName() override;
};

/**
 * @class EnergyConsumeAction
 * @brief 能量消耗动作，消耗角色能量
 */
class EnergyConsumeAction : public Action
{
private:
    static std::string name;        ///< 动作名称："EnergyConsumeAction"
    static std::vector<std::unique_ptr<EnergyListener>> listeners;  ///< 能量监听器列表
    const Skill* const skill;       ///< 技能指针

public:
    EnergyConsumeAction() : Action(), skill(nullptr) {};
    EnergyConsumeAction(const Skill* const skill) : Action(), skill(skill) {};
    
    /**
     * @brief 执行能量消耗
     * @param n 消耗的能量值
     * @param p 执行动作的角色
     */
    virtual void execute(double n, Person *p) override;
    static void addListener(std::unique_ptr<EnergyListener> info);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    std::string getActionName() override;
};

/**
 * @class EnergyRevertAction
 * @brief 能量回复动作，回复角色能量
 */
class EnergyRevertAction : public Action
{
private:
    static std::string name;  ///< 动作名称："EnergyRevertAction"
    static std::vector<std::unique_ptr<EnergyListener>> listeners;  ///< 能量监听器列表
    const Skill* const skill;       ///< 技能指针

public:
    EnergyRevertAction() : Action(), skill(nullptr) {};
    EnergyRevertAction(const Skill* const skill) : Action(), skill(skill) {};
    
    /**
     * @brief 执行能量回复
     * @param n 回复的能量值
     * @param p 执行动作的角色
     */
    virtual void execute(double n, Person *p) override;
    static void addListener(std::unique_ptr<EnergyListener> info);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    std::string getActionName() override;
};

/**
 * @class CDReduceAction
 * @brief 冷却减少动作，减少技能冷却时间
 */
class CDReduceAction : public Action
{
private:
    static std::string name;  ///< 动作名称："CDReduceAction"
    static std::vector<std::unique_ptr<CDListener>> listeners;  ///< 冷却监听器列表
    std::string skillName;    ///< 技能名称

public:
    /**
     * @brief 构造函数
     * @param skillName 要减少冷却的技能名称
     */
    explicit CDReduceAction(std::string skillName);
    
    /**
     * @brief 执行冷却减少
     * @param n 减少的冷却时间（毫秒）
     * @param p 执行动作的角色
     */
    virtual void execute(double n, Person *p) override;
    static void addListener(std::unique_ptr<CDListener> info);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    std::string getActionName() override;
};

/**
 * @class CDRefreshAction
 * @brief 冷却刷新动作，完全重置技能冷却
 */
class CDRefreshAction : public Action
{
private:
    static std::string name;  ///< 动作名称："CDRefreshAction"
    static std::vector<std::unique_ptr<CDListener>> listeners;  ///< 冷却监听器列表
    std::string skillName;    ///< 技能名称

public:
    /**
     * @brief 构造函数
     * @param skillName 要刷新冷却的技能名称
     */
    explicit CDRefreshAction(std::string skillName);
    
    /**
     * @brief 执行冷却刷新
     * @param n 未使用参数
     * @param p 执行动作的角色
     */
    void execute(double n, Person *p) override;
    static void addListener(std::unique_ptr<CDListener> info);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    std::string getActionName() override;
};

/**
 * @class CreateSkillAction
 * @brief 创建技能动作，实例化并释放新技能
 */
class CreateSkillAction : public Action
{
private:
    static std::string name;  ///< 动作名称："CreateSkillAction"
    static std::vector<std::unique_ptr<CreateSkillListener>> listeners;  ///< 技能创建监听器列表
    std::string skillName;    ///< 技能名称

public:
    /**
     * @brief 构造函数
     * @param skillname 要创建的技能名称
     */
    CreateSkillAction(std::string skillname);
    
    /**
     * @brief 执行技能创建
     * @param p 执行动作的角色
     */
    virtual void execute(double, Person *p) override; // double n未使用
    static void addListener(std::unique_ptr<CreateSkillListener> info);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    std::string getActionName() override;
};

/**
 * @class CreateBuffAction
 * @brief 创建Buff动作，实例化并应用新Buff
 */
class CreateBuffAction : public Action
{
private:
    static std::string name;  ///< 动作名称："CreateBuffAction"
    static std::vector<std::unique_ptr<CreateBuffListener>> listeners;  ///< Buff创建监听器列表
    std::string buffName;     ///< Buff名称

public:
    /**
     * @brief 构造函数
     * @param buffName 要创建的Buff名称
     */
    explicit CreateBuffAction(std::string buffName);
    
    /**
     * @brief 执行Buff创建
     * @param n 对于可叠层Buff作为叠层数，其他情况未使用
     * @param p 执行动作的角色
     */
    virtual void execute(double n, Person *p) override;
    static void addListener(std::unique_ptr<CreateBuffListener> info);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    std::string getActionName() override;
};

/**
 * @class AddFactorEnergyAction
 * @brief 增加角色的因子能量值
 */
class AddFactorEnergyAction : public Action
{
private:
    static std::string name;  ///< 动作名称："AddFactorEnergyAction"
    static std::vector<std::unique_ptr<FactorEnergyListener>> listeners;  ///< Buff创建监听器列表

public:
    /**
     * @brief 构造函数
     */
    explicit AddFactorEnergyAction() : Action() {};
    
    /**
     * @brief 执行Buff创建
     * @param n 对于可叠层Buff作为叠层数，其他情况未使用
     * @param p 执行动作的角色
     */
    virtual void execute(double n, Person *p) override;
    static void addListener(std::unique_ptr<FactorEnergyListener> info);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    std::string getActionName() override;
};

/* ============================================================================
 * 属性修改动作类
 * 每个属性都有数值修改和百分比修改两种方式
 * ============================================================================
 */

/**
 * @class CriticalCountModifyAction
 * @brief 暴击数值修改动作，通过属性点数修改暴击率
 */
class CriticalCountModifyAction : public Action
{
private:
    static std::string name;  ///< 动作名称："CriticalCountModifyAction"
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;  ///< 副属性监听器列表

public:
    CriticalCountModifyAction();
    
    /**
     * @brief 执行暴击数值修改
     * @param n 增加的暴击属性点数
     * @param p 执行动作的角色
     * 下同
     */
    virtual void execute(double n, Person *p) override;
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

/**
 * @class CriticalPercentModifyAction
 * @brief 暴击百分比修改动作，直接修改暴击率百分比
 */
class CriticalPercentModifyAction : public Action
{
private:
    static std::string name;  ///< 动作名称："CriticalPercentModifyAction"
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;  ///< 副属性监听器列表

public:
    CriticalPercentModifyAction();
    
    /**
     * @brief 执行暴击百分比修改
     * @param n 增加的暴击百分比（如0.05表示+5%）
     * @param p 执行动作的角色
     * 下同
     */
    virtual void execute(double n, Person *p) override;
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 急速数值
class QuicknessCountModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;

public:
    QuicknessCountModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的急速数值
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 急速百分比
class QuicknessPercentModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;

public:
    QuicknessPercentModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的急速百分比
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 幸运数值
class LuckyCountModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;

public:
    LuckyCountModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的幸运数值
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 幸运百分比
class LuckyPercentModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;

public:
    LuckyPercentModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的幸运百分比
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 精通数值
class ProficientCountModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;

public:
    ProficientCountModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的精通数值
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 精通百分比
class ProficientPercentModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;

public:
    ProficientPercentModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的精通百分比
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 全能数值
class AlmightyCountModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;

public:
    AlmightyCountModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的全能数值
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 全能百分比
class AlmightyPercentModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;

public:
    AlmightyPercentModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的全能百分比
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 主属性数值
class PrimaryAttributesCountModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<PrimaryAttributeListener>> listeners;

public:
    PrimaryAttributesCountModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的主属性数值
    static void addListener(std::unique_ptr<PrimaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 主属性百分比
class PrimaryAttributesPercentModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<PrimaryAttributeListener>> listeners;

public:
    PrimaryAttributesPercentModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的主属性百分比
    static void addListener(std::unique_ptr<PrimaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 施法速度百分比
class CastingSpeedPercentModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;

public:
    CastingSpeedPercentModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的施法速度百分比
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 攻击速度百分比
class AttackSpeedPercentModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;

public:
    AttackSpeedPercentModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的攻击速度百分比
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 攻击增加数值
class AttackCountModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<PrimaryAttributeListener>> listeners;

public:
    AttackCountModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的攻击增加百分比
    static void addListener(std::unique_ptr<PrimaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 攻击增加百分比
class AttackIncreaseModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<PrimaryAttributeListener>> listeners;

public:
    AttackIncreaseModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的攻击增加百分比
    static void addListener(std::unique_ptr<PrimaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 精炼攻击增加数值
class RefineATKCountModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<PrimaryAttributeListener>> listeners;

public:
    RefineATKCountModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的攻击增加百分比
    static void addListener(std::unique_ptr<PrimaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 伤害增伤百分比
class DamageIncreaseModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;

public:
    DamageIncreaseModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的伤害增伤百分比
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 元素增伤百分比
class ElementIncreaseModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;

public:
    ElementIncreaseModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的元素增伤百分比
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 全能增伤百分比
class AlmightyIncreaseModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;

public:
    AlmightyIncreaseModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的全能增伤百分比
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 爆伤百分比
class CriticalDamageModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;

public:
    CriticalDamageModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的爆伤百分比
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};

// 梦境增伤百分比
class DreamIncreaseModifyAction : public Action
{
private:
    static std::string name;
    static std::vector<std::unique_ptr<SecondaryAttributeListener>> listeners;

public:
    DreamIncreaseModifyAction();
    virtual void execute(double n, Person *p) override; // n为增加的梦境增伤百分比
    static void addListener(std::unique_ptr<SecondaryAttributeListener> listener);
    static void deleteListener(int buffID);
    static auto& getListeners() { return listeners; } // 获取监听器列表（调试用）
    virtual std::string getActionName() override;
};
