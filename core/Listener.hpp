#pragma once

#include "Info.h"              // 信息类定义
#include <functional>          // 函数对象
#include "Skill.h"             // 技能类定义
#include "Buff.h"              // 增益效果类定义

/**
 * @class DamageInfo
 * @brief 伤害信息类（前向声明）
 * 
 * 包含伤害计算相关的详细信息，用于伤害监听器的回调参数
 */
class DamageInfo;

// ============================================================================
// 监听器基类
// ============================================================================

/**
 * @class BaseListener
 * @brief 监听器抽象基类
 * 
 * 所有具体监听器类的基类，提供统一的接口和公共数据成员。
 * 采用纯虚析构函数确保派生类能够正确释放资源。
 * 
 * @note 使用buffID关联监听器与具体的增益效果
 */
class BaseListener {
public:
    int buffID;               ///< 关联的增益效果ID，用于标识监听器的来源
    
    /**
     * @brief 默认构造函数
     * 
     * 初始化buffID为0，表示未关联任何增益效果
     */
    BaseListener() : buffID(0) {}
    
    /**
     * @brief 虚析构函数（声明为纯虚）
     * 
     * 确保BaseListener成为抽象类，同时提供默认实现
     * 允许通过基类指针正确删除派生类对象
     */
    virtual ~BaseListener() = 0;
};

/**
 * @brief BaseListener纯虚析构函数的默认实现
 * 
 * 虽然声明为纯虚函数，但仍需提供实现以确保派生类析构时能正确释放资源
 */
inline BaseListener::~BaseListener() = default;

// ============================================================================
// 类型化监听器模板类
// ============================================================================

/**
 * @class TypedListener
 * @brief 类型化监听器模板基类
 * 
 * 为特定事件类型提供类型安全的回调机制，封装回调函数和关联ID。
 * 使用模板参数指定监听的事件数据类型，支持任意类型的回调函数。
 * 
 * @tparam T 监听的事件数据类型
 * 
 * @note 继承自BaseListener，是所有具体类型监听器的通用模板
 */
template <typename T>
class TypedListener : public BaseListener {
public:
    std::function<void(T&)> callback;  ///< 回调函数，接收T类型的事件数据
    
    /**
     * @brief 构造函数
     * 
     * @param id 关联的增益效果ID
     * @param func 回调函数，处理T类型的事件数据
     * 
     * @note 使用std::move避免回调函数不必要的复制，提高性能
     */
    TypedListener(int id, std::function<void(T&)> func) 
                : callback(std::move(func))
    {
        this->buffID = id;
    }
};

// ============================================================================
// 具体事件监听器类
// ============================================================================

/**
 * @class DamageListener
 * @brief 伤害事件监听器
 * 
 * 监听伤害事件，当伤害发生时触发回调函数。
 * 回调函数接收DamageInfo对象，包含详细的伤害计算信息。
 * 
 * @note 用于技能伤害、普攻伤害、环境伤害等各种伤害事件
 */
class DamageListener : public TypedListener<DamageInfo> {
public:
    /**
     * @brief 构造函数
     * 
     * @param id 关联的增益效果ID
     * @param func 伤害事件回调函数，接收DamageInfo引用
     */
    DamageListener(const int id, std::function<void(DamageInfo&)> func)
        : TypedListener<DamageInfo>(id, func) {}
    
    /**
     * @brief 触发伤害事件
     * 
     * 调用注册的回调函数处理伤害信息
     * 
     * @param info 伤害信息对象，包含伤害计算的所有相关数据
     */
    void trigger(DamageInfo &info) {
        if (callback) callback(info);
    }
};

/**
 * @class EnergyListener
 * @brief 能量变化监听器
 * 
 * 监听能量值变化事件，当能量增加或减少时触发回调。
 * 回调函数接收当前能量值或能量变化量。
 * 
 * @note 用于能量系统的监听
 */
class EnergyListener : public TypedListener<double> {
public:
    /**
     * @brief 构造函数
     * 
     * @param id 关联的增益效果ID
     * @param func 能量变化回调函数，接收能量值
     */
    EnergyListener(const int id, const std::function<void(const double)> func)
        : TypedListener<double>(id, func) {}
    
    /**
     * @brief 触发能量变化事件
     * 
     * 调用注册的回调函数处理能量值变化
     * 
     * @param amount 当前能量值或能量变化量
     */
    void trigger(double amount) const {
        if (callback) callback(amount);
    }
};

/**
 * @class ResourceListener
 * @brief 资源变化监听器
 * 
 * 监听资源值变化事件，当资源增加或减少时触发回调。
 * 回调函数接收当前资源值或资源变化量。
 * 
 * @note 用于资源系统的监听
 */
class ResourceListener : public TypedListener<double> {
public:
    /**
     * @brief 构造函数
     * 
     * @param id 关联的增益效果ID
     * @param func 资源变化回调函数，接收资源值
     */
    ResourceListener(const int id, const std::function<void(const double)> func)
        : TypedListener<double>(id, func) {}
    
    /**
     * @brief 触发资源变化事件
     * 
     * 调用注册的回调函数处理资源值变化
     * 
     * @param amount 当前资源值或资源变化量
     */
    void trigger(double amount) const {
        if (callback) callback(amount);
    }
};

/**
 * @class CDListener
 * @brief 冷却时间监听器
 * 
 * 监听冷却时间变化事件，当冷却时间更新时触发回调。
 * 回调函数接收当前冷却时间或冷却时间变化量。
 * 
 * @note 用于冷却系统的监听
 */
class CDListener : public TypedListener<double> {
public:
    /**
     * @brief 构造函数
     * 
     * @param id 关联的增益效果ID
     * @param func 冷却时间回调函数，接收冷却时间值
     */
    CDListener(const int id, const std::function<void(const double)> func)
        : TypedListener<double>(id, func) {}
    
    /**
     * @brief 触发冷却时间事件
     * 
     * 调用注册的回调函数处理冷却时间变化
     * 
     * @param amount 当前冷却时间或冷却时间变化量
     */
    void trigger(double amount) const {
        if (callback) callback(amount);
    }
};

/**
 * @class CreateSkillListener
 * @brief 技能创建监听器
 * 
 * 监听技能创建事件，当新技能被创建时触发回调。
 * 回调函数接收新创建的技能对象指针。
 * 
 * @note 用于技能创建的监听
 */
class CreateSkillListener : public TypedListener<Skill*> {
public:
    /**
     * @brief 构造函数
     * 
     * @param id 关联的增益效果ID
     * @param func 技能创建回调函数，接收Skill*指针
     */
    CreateSkillListener(const int id, std::function<void(Skill* const)> func)
        : TypedListener<Skill*>(id, func) {}
    
    /**
     * @brief 触发技能创建事件
     * 
     * 调用注册的回调函数处理新创建的技能
     * 
     * @param skill 新创建的技能对象指针
     * 
     * @note 回调函数应负责技能对象的生命周期管理
     */
    void trigger(Skill* skill) const {
        if (callback) callback(skill);
    }
};

/**
 * @class CreateBuffListener
 * @brief 增益效果创建监听器
 * 
 * 监听增益效果创建事件，当新增益效果被创建时触发回调。
 * 回调函数接收新创建的增益效果对象指针。
 * 
 * @note 用于buff创建的监听
 */
class CreateBuffListener : public TypedListener<Buff*> {
public:
    /**
     * @brief 构造函数
     * 
     * @param id 关联的增益效果ID
     * @param func 增益效果创建回调函数，接收Buff*指针
     */
    CreateBuffListener(const int id, std::function<void(Buff* const)> func)
        : TypedListener<Buff*>(id, func) {}
    
    /**
     * @brief 触发增益效果创建事件
     * 
     * 调用注册的回调函数处理新创建的增益效果
     * 
     * @param buff 新创建的增益效果对象指针
     * 
     * @note 回调函数应负责增益效果对象的生命周期管理
     */
    void trigger(Buff* buff) const {
        if (callback) callback(buff);
    }
};

// ============================================================================
// 属性监听器分类
// ============================================================================

/**
 * @class PrimaryAttributeListener
 * @brief 主属性监听器
 * 
 * 监听角色主属性变化事件，包括智力、敏捷、力量等。
 * 当主属性值发生变化时触发回调函数。
 * 
 * @note 用于角色基础属性系统的监听，通常影响角色的攻击力
 */
class PrimaryAttributeListener : public TypedListener<double> {
public:
    /**
     * @brief 构造函数
     * 
     * @param id 关联的增益效果ID
     * @param func 主属性变化回调函数，接收属性值
     */
    PrimaryAttributeListener(const int id, const std::function<void(const double)> func)
        : TypedListener<double>(id, func) {}
    
    /**
     * @brief 触发主属性变化事件
     * 
     * 调用注册的回调函数处理主属性值变化
     * 
     * @param value 当前主属性值或属性变化量
     */
    void trigger(double value) const {
        if (callback) callback(value);
    }
};

/**
 * @class SecondaryAttributeListener
 * @brief 副属性监听器
 * 
 * 监听角色副属性变化事件，包括暴击、急速、幸运、精通、全能等。
 * 当副属性值发生变化时触发回调函数。
 * 
 * @note 用于角色进阶属性系统的监听
 */
class SecondaryAttributeListener : public TypedListener<double> {
public:
    /**
     * @brief 构造函数
     * 
     * @param id 关联的增益效果ID
     * @param func 副属性变化回调函数，接收属性值
     */
    SecondaryAttributeListener(const int id, const std::function<void(const double)> func)
        : TypedListener<double>(id, func) {}
    
    /**
     * @brief 触发副属性变化事件
     * 
     * 调用注册的回调函数处理副属性值变化
     * 
     * @param amount 当前副属性值或属性变化量
     */
    void trigger(double amount) const {
        if (callback) callback(amount);
    }
};

/**
 * @class SpeedAttributeListener
 * @brief 速度属性监听器
 * 
 * 监听角色速度属性变化事件，包括施法速度、攻击速度等。
 * 当速度属性值发生变化时触发回调函数。
 * 
 * @note 用于角色动作速度系统的监听，影响角色的行动频率
 */
class SpeedAttributeListener : public TypedListener<double> {
public:
    /**
     * @brief 构造函数
     * 
     * @param id 关联的增益效果ID
     * @param func 速度属性变化回调函数，接收属性值
     */
    SpeedAttributeListener(const int id, const std::function<void(const double)> func)
        : TypedListener<double>(id, func) {}
    
    /**
     * @brief 触发速度属性变化事件
     * 
     * 调用注册的回调函数处理速度属性值变化
     * 
     * @param value 当前速度属性值或属性变化量
     */
    void trigger(double value) const {
        if (callback) callback(value);
    }
};

/**
 * @class MultiplierAttributeListener
 * @brief 乘区属性监听器
 * 
 * 监听角色乘区属性变化事件，包括攻击增伤、伤害增伤、元素增伤、全能增伤、爆伤、梦境增伤等。
 * 当乘区属性值发生变化时触发回调函数。
 * 
 * @note 用于角色伤害乘区系统的监听，以乘法方式影响最终伤害
 */
class MultiplierAttributeListener : public TypedListener<double> {
public:
    /**
     * @brief 构造函数
     * 
     * @param id 关联的增益效果ID
     * @param func 乘区属性变化回调函数，接收属性值
     */
    MultiplierAttributeListener(const int id, const std::function<void(const double)> func)
        : TypedListener<double>(id, func) {}
    
    /**
     * @brief 触发乘区属性变化事件
     * 
     * 调用注册的回调函数处理乘区属性值变化
     * 
     * @param value 当前乘区属性值或属性变化量
     */
    void trigger(double value) const {
        if (callback) callback(value);
    }
};

/**
 * @class FactorEnergyListener
 * @brief 因子能量监听器
 * 
 * 监听角色因子能量属性变化事件，包括因子能量值的增加或减少。
 * 当获取或消耗因子能量时触发回调函数。
 * 
 * @note 用于角色伤害乘区系统的监听，以乘法方式影响最终伤害
 */
class FactorEnergyListener : public TypedListener<double> {
public:
    /**
     * @brief 构造函数
     * 
     * @param id 关联的增益效果ID
     * @param func 乘区属性变化回调函数，接收属性值
     */
    FactorEnergyListener(const int id, const std::function<void(const double)> func)
        : TypedListener<double>(id, func) {}
    
    /**
     * @brief 触发乘区属性变化事件
     * 
     * 调用注册的回调函数处理乘区属性值变化
     * 
     * @param value 当前乘区属性值或属性变化量
     */
    void trigger(double value) const {
        if (callback) callback(value);
    }
};