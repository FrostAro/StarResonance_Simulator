#pragma once
#include "Buff.h"
#include <vector>
#include <string>
#include <map>

// 前向声明
class Person;
class Initializer;

// ============================================================================
// Skill 基类声明
// ============================================================================

/**
 * @class Skill
 * @brief 技能系统基类
 * 
 * 主要特性：
 * 1. 支持多种技能类型：立即触发、持续触发、引导性技能
 * 2. 完整的冷却系统（CD、充能CD）
 * 3. 层数管理和技能堆叠
 * 4. 技能类型枚举系统和转换工具
 * 5. 技能效果加成系统（暴击、元素、伤害等）
 * 
 * 技能分类：
 * - InstantSkill（立即触发性技能）：一次性触发，如单体攻击
 * - ContinuousSkill（持续触发性技能）：多次触发，如持续伤害技能
 * - FacilitationSkill（引导性技能）：需要引导时间，可中途中断
 * 
 * @note 这是一个抽象基类，需要子类实现具体的技能逻辑
 */
class Skill
{
protected:
    // ============================================================================
    // 静态成员变量
    // ============================================================================
    
    static std::string name;        // 技能类名称（类级别）
    static int ID;                  // 技能ID计数器（类级别）

    // ============================================================================
    // 技能核心属性
    // ============================================================================
    
    int skillID = 0;                // 技能唯一ID（实例级别）
    
    // 技能类型标志
    bool canTriggerLucky = false;   // 可触发幸运伤害
    bool canTriggerCrit = true;     // 可触发暴击伤害
    bool canCharge = false;         // 可充能（拥有充能CD）
    bool isInstant = false;         // 立即触发性技能
    bool isContinuous = false;      // 持续性技能
    bool isNoReleasing = false;     // 无前摇技能
    bool isFacilitation = false;    // 引导性技能

    double energyReduceUP = 0;      // 技能独立能量消耗增加
    double energyReduceDOWN = 0;    // 技能独立能量消耗减少
    double energyAddIncrease = 0;   // 技能独立能量回复增加

public:
    friend class Initializer;

    // ============================================================================
    // 公共成员变量
    // ============================================================================
    
    bool triggered = false;          // 是否已触发技能（是否已造成伤害/产生效果）
    bool isEquipped = false;         // 是否已装备该技能
    double damageTriggerTimer = 0;   // 伤害触发计时器（用于持续性技能）

    // 能量系统
    double energyAdd = 0;           // 能量回复量
    double energyReduce = 0;        // 能量消耗量
    
    // 冷却系统
    double CD = 0;                  // 当前冷却时间
    double MaxCD = 0;               // 最大冷却时间
    double chargeCD = 0;            // 当前充能CD
    double MaxchargeCD = 0;         // 最大充能CD
    
    // 层数系统
    int stack = 1;                  // 当前层数（可使用次数）
    int maxStack = 1;               // 最大层数
    
    // 伤害计算
    double multiplying = 0;         // 基础倍率（百分比加成）
    double fixedValue = 0;          // 基础固定值（固定伤害）
    double luckyFiexedValue = 0;    // 幸运伤害固定值（固定幸运伤害）
    
    // 时间系统
    double duration = 0;                // 持续时间（毫秒）
    double damageTriggerInterval = 0;   // 伤害触发间隔（毫秒）
    int singing = 0;                    // 吟唱时间（毫秒）
    double releasingTime = 0;           // 前摇时间（毫秒）
    
    // ============================================================================
    // 技能效果加成系统
    // ============================================================================
    
    /**
     * @brief 技能提供的属性加成效果
     * @details 这些加成会影响Person对象的属性
     */
    double criticalIncreaseAdd = 0;      // 爆伤加成
    double elementIncreaseAdd = 0;       // 元素伤害加成
    double damageIncreaseAdd = 0;        // 伤害加成
    double almightyIncreaseAdd = 0;      // 全能伤害加成
    double dreamIncreaseAdd = 0;         // 幻梦伤害加成
    double finalIncreaseAdd = 0;         // 最终伤害加成
    double multiplyingIncrease = 0;      // 技能倍率加成
    double luckyIncreaseAdd = 0;         // 幸运伤害加成
    double luckyFinalIncreaseAdd = 0;    // 幸运最终伤害加成
    double luckyDreamIncreaseAdd = 0;    // 幸运幻梦伤害加成

    // 技能属性针对加成
    double criticalAdd = 0;             //暴击属性加成
    
    // ============================================================================
    // 技能类型枚举和列表
    // ============================================================================
    
    /**
     * @enum skillTypeEnum
     * @brief 技能类型枚举
     * 
     * 用于分类技能的属性和作用方式：
     * - NONE: 无类型（默认）
     * - NORMAL: 普通技能
     * - SPECIALIZED: 专精技能
     * - PARTICULAR: 特殊技能
     * - FAR: 远程技能
     * - CLOSE: 近战技能
     * - WITCHCRAFT: 巫术技能
     * - PHYSICS: 物理技能
     */
    enum class skillTypeEnum
    {
        NONE,
        NORMAL,
        SPECIALIZED,
        PARTICULAR,
        FAR,
        CLOSE,
        WITCHCRAFT,
        PHYSICS
    };
    
    std::vector<skillTypeEnum> skillTypeList{};  // 技能类型列表（一个技能可拥有多个类型）

public:
    // ============================================================================
    // 构造与析构函数
    // ============================================================================
    
    /**
     * @brief 默认构造函数
     * @details 分配唯一的技能ID并递增计数器
     */
    Skill();
    
    /**
     * @brief 虚析构函数
     * @details 确保派生类对象被正确销毁
     */
    virtual ~Skill();
    
    // ============================================================================
    // 纯虚函数声明（需要子类实现）
    // ============================================================================
    
    /**
     * @brief 释放技能或每tick触发逻辑
     * 
     * @param p 释放技能的人物对象指针
     * 
     * @note 对于持续性技能，此函数在每tick调用
     *       对于立即触发性技能，此函数只调用一次
     */
    virtual void use(Person *p) = 0;
    
    /**
     * @brief 设置技能类型
     * @details 子类应在此函数中设置skillTypeList
     */
    virtual void setSkillType() = 0;
    
    /**
     * @brief 获取技能名称
     * 
     * @return std::string 技能名称
     */
    virtual std::string getSkillName() const = 0;
    
    // ============================================================================
    // 公共成员函数
    // ============================================================================
    
    /**
     * @brief 判断技能是否应该被移除（暂时弃用）
     * 
     * @return bool true=应该移除，false=继续保留
     * @deprecated 此函数暂时弃用，未来可能会重新实现
     */
    virtual bool shouldBeRemoved();
    
    // ============================================================================
    // 时间管理函数
    // ============================================================================
    
    bool reduceDuration();                       // 减少持续时间（步长1）
    bool reduceDuration(double n);               // 减少持续时间（指定步长）
    bool addDamageTriggerTimer();                // 增加伤害触发计时器（步长1）
    bool addDamageTriggerTimer(double n);        // 增加伤害触发计时器（指定步长）
    bool reduceReleasingTime();                  // 减少前摇时间（步长1）
    bool reduceReleasingTime(double n);          // 减少前摇时间（指定步长）

    // ============================================================================
    // 能量变化管理函数
    // ============================================================================

    void changeEnergyReduceUP(double n);
    void changeEnergyReduceDOWN(double n);
    void changeEnergyAddIncrease(double n);
    
    // ============================================================================
    // 技能类型判断函数
    // ============================================================================
    
    /**
     * @brief 判断技能是否拥有指定类型
     * 
     * @param type 要检查的技能类型
     * @return bool true=拥有该类型，false=不拥有
     */
    bool hasSkillType(skillTypeEnum type) const;
    
    // ============================================================================
    // 枚举字符串转换相关静态方法
    // ============================================================================
    
    /**
     * @brief 技能类型枚举转字符串
     * 
     * @param type 技能类型枚举值
     * @return std::string 对应的字符串表示
     */
    static std::string skillTypeToString(skillTypeEnum type);
    
    /**
     * @brief 字符串转技能类型枚举
     * 
     * @param str 技能类型字符串
     * @return skillTypeEnum 对应的枚举值
     * @throws std::invalid_argument 当字符串无法识别时抛出异常
     */
    static skillTypeEnum stringToSkillType(const std::string& str);
    
    /**
     * @brief 获取技能类型映射表
     * 
     * @return const std::map<skillTypeEnum, std::string>& 技能类型映射表引用
     */
    static const std::map<skillTypeEnum, std::string>& getSkillTypeMap();
    
    /**
     * @brief 获取所有技能类型的字符串列表
     * 
     * @return std::vector<std::string> 技能类型字符串列表
     */
    static std::vector<std::string> getAllSkillTypeNames();
    
    /**
     * @brief 将技能类型列表转换为字符串表示
     * 
     * @param typeList 技能类型枚举列表
     * @return std::string 格式化的字符串，如"[NORMAL, CLOSE, PHYSICS]"
     */
    std::string skillTypeListToString(const std::vector<Skill::skillTypeEnum>& typeList);
    
    // ============================================================================
    // 访问器函数
    // ============================================================================
    
    int getSkillID() const;
    double getAdd() const;
    double getEnergyReduce() const;
    bool getCanTriggerLucky() const;
    bool getCanTriggerCrit() const;
    void setCanTriggerLucky(bool value);
    void setCanTriggerCrit(bool value);
    bool getCanCharge() const;
    bool getIsEquipped() const;
    bool getTriggered() const;
    bool getIsInstant() const;
    bool getIsContinuous() const;
    bool getIsNoReleasing() const;
    bool getIsFacilitation() const;

    double getEnergyReduceUP() const;
    double getEnergyReduceDOWN() const;
    double getEnergyAddIncrease() const;
    
    double getCriticalIncreaseAdd() const;
    double getElementIncreaseAdd() const;
    double getDamageIncreaseAdd() const;
    double getAlmightyIncreaseAdd() const;
    double getDreamIncreaseAdd() const;
    double getFinalIncreaseAdd() const;
    double getMultiplyingIncrease() const;
    double getLuckyIncreaseAdd() const;
    double getLuckyFinalIncreaseAdd() const;
    double getLuckyDreamIncreaseAdd() const;
    
    double getMaxCD() const;
    double &getCurrentCD_Ref();
    double getCurrentCD_Value() const;
    void setCD(double cd);
    double getMaxChargedCD() const;
    double &getCurrentChargedCD_Ref();
    double getCurrentChargedCD_Value() const;
    int &getStackRef();
    int getStackValue() const;
    int getMaxStack() const;
    double getMutiplying() const;
    double getFixedValue() const;
    double getLuckyFiexedValue() const;
    double getReleasingTime() const;
    double getDuration() const;
    double getDamageTriggerInterval() const;
    double getDamageTriggerTimer() const;
    int getSinging() const;
    std::vector<Skill::skillTypeEnum> getSkillType();
};

// ============================================================================
// InstantSkill 类 - 立即触发性技能
// ============================================================================

/**
 * @class InstantSkill
 * @brief 立即触发性技能抽象类
 * 
 * 主要特性：
 * 1. 一次性触发，释放后立即生效
 * 2. 通常用于单体攻击或瞬发效果
 * 3. triggered标志确保只触发一次
 * 
 * @note 需要子类实现trigger()函数定义具体效果
 */
class InstantSkill : public Skill {
protected:
    /**
     * @brief 技能使用函数（重写）
     * @details 调用trigger()函数触发技能效果，并设置triggered标志
     */
    void use(Person* p) override;
    
    /**
     * @brief 技能触发函数（纯虚）
     * @details 子类必须实现此函数定义技能的具体效果
     */
    virtual void trigger(Person* p) = 0;
    
    /**
     * @brief 构造函数
     * @details 设置isInstant标志为true
     */
    InstantSkill();
    
    virtual ~InstantSkill() = default;
};

// ============================================================================
// ContinuousSkill 类 - 持续触发性技能
// ============================================================================

/**
 * @class ContinuousSkill
 * @brief 持续触发性技能抽象类
 * 
 * 主要特性：
 * 1. 多次触发，在持续时间内定期生效
 * 2. 使用damageTriggerTimer和damageTriggerInterval控制触发频率
 * 3. 通常用于持续伤害或持续治疗效果
 * 
 * @note 需要子类实现trigger()函数定义每次触发的具体效果
 */
class ContinuousSkill : public Skill {
protected:
    /**
     * @brief 技能使用函数（重写）
     * @details 当damageTriggerTimer达到damageTriggerInterval时调用trigger()
     */
    void use(Person* p) override;
    
    /**
     * @brief 技能触发函数（纯虚）
     * @details 子类必须实现此函数定义每次触发的具体效果
     */
    virtual void trigger(Person* p) = 0;
    
    /**
     * @brief 构造函数
     * @details 设置isContinuous标志为true
     */
    ContinuousSkill();
    
    virtual ~ContinuousSkill() = default;
};

// ============================================================================
// FacilitationSkill 类 - 引导性技能
// ============================================================================

/**
 * @class FacilitationSkill
 * @brief 引导性技能抽象类
 * 
 * 主要特性：
 * 1. 需要引导时间，引导期间持续生效
 * 2. 可被中断，通过canEndFacilitation()判断引导是否结束
 * 3. 通常用于需要持续施法的技能
 * 
 * @note 需要子类实现trigger()和canEndFacilitation()函数
 */
class FacilitationSkill : public Skill {
protected:
    /**
     * @brief 技能使用函数（重写）
     * @details 当damageTriggerTimer达到damageTriggerInterval时调用trigger()
     */
    void use(Person* p) override;
    
    /**
     * @brief 技能触发函数（纯虚）
     * @details 子类必须实现此函数定义引导期间每次触发的具体效果
     */
    virtual void trigger(Person* p) = 0;
    
    /**
     * @brief 构造函数
     * @details 设置isFacilitation标志为true
     */
    FacilitationSkill();
    
    virtual ~FacilitationSkill() = default;

public:
    /**
     * @brief 判断引导是否结束（纯虚）
     * 
     * @param p 释放技能的人物对象指针
     * @return bool true=引导结束，false=继续引导
     * 
     * @details 子类必须实现此函数定义引导结束的条件
     */
    virtual bool canEndFacilitation(Person *p) = 0;

    /**
     * @brief 强行停止引导技能
     * 
     * @details 通过将duration变为0实现停止
     */
    void stop();
};


// 幻想
class FightingFantasy
{
public:
    virtual void setPassiveEffect(Person *p) = 0;
    virtual void removePassiveEffect(Person *p) = 0;

	FightingFantasy() = default;
    virtual ~FightingFantasy() = default;
};

// 临时直接释放型技能（可自定义倍率，便于实现某些临时攻击）
class temp_InstantSkill : public InstantSkill
{
public:
	static std::string name;

	void setSkillType() override {};
	void trigger(Person *p) override {};
	std::string getSkillName() const override;

	explicit temp_InstantSkill(std::string skillName,double multiplying,double fixedValue);
};