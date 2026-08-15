#pragma once
#include "Info.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <queue>

class Person;
class PriorSkillInfo;

// ============================================================================
// AutoAttack 类声明
// ============================================================================

/**
 * @class AutoAttack
 * @brief 自动战斗系统核心类
 * 
 * 主要特性：
 * 1. 模拟游戏内自动施法AI
 * 2. 支持手动技能序列与优先级自动判断
 * 3. 分爆发阶段管理（大爆发/小爆发/空窗期）
 * 4. 可扩展的技能释放条件判断
 * 5. 线程安全的技能优先级管理
 * 
 * 工作原理：
 * 1. 通过定时器驱动更新逻辑
 * 2. 根据技能优先级和冷却状态选择释放技能
 * 3. 支持爆发状态下的特殊技能序列
 * 4. 可自定义技能释放的额外条件
 * 
 * @note 这是一个抽象基类，需要子类实现具体的战斗逻辑
 */
class AutoAttack
{
protected:
    // ============================================================================
    // 内部结构定义
    // ============================================================================
    
    /**
     * @struct judgeConditionStruct
     * @brief 技能释放条件判断结构
     * 
     * 用于定义额外的技能释放条件，包含：
     * 1. 条件判断函数
     * 2. 条件不满足时的错误类型
     */
    struct judgeConditionStruct 
    {
        std::function<bool(const PriorSkillInfo&)> condition;  // 条件判断函数
        ErrorInfo::errorTypeEnum errorType;                    // 条件不满足时的错误类型
    };

    /**
     * @struct certainTimeSkillStruct
     * @brief 记录时间与该时间应释放的技能
     */
    struct certainTimeSkillStruct 
    {
        std::string skillName;
        double time;
    };

    // ============================================================================
    // 静态成员变量
    // ============================================================================
    
    static double timer;        // 全局计时器，用于同步所有AutoAttack实例的时间
    static double deltaTime;    // 全局时间变量，用于同步所有更新事件倍速

    // ============================================================================
    // 实例成员变量
    // ============================================================================
    
    std::vector<PriorSkillInfo> priorSkillList{};                          // 优先级技能列表（待释放）
    std::vector<PriorSkillInfo> skillPriority{};                           // 有序的优先级列表（优先级高→低）
    std::vector<judgeConditionStruct> judgingConditionsForCreateSkill{};   // 技能释放的判定条件列表
    std::vector<std::string> equippedAndNoReleasingTimeSkill{};            // 已装备技能中无前摇，即可同时释放的技能（如射线龙卷）
    std::queue<certainTimeSkillStruct> certainTimeSkillList{};            // 特定时间触发技能列表
    
    bool isReleasingSkill = false;          // 当前是否正在释放技能（防止技能打断）
    bool isOutBurst = false;                // 是否处于爆发状态
    int nextOutBurstType = 1;               // 下一次爆发的类型：0=小爆发，1=大爆发
    bool signalForRepeatedlyOutBurst = false; // 防止连续爆发触发的标志位
    unsigned short maxProcessPerTick = 50;  // 每tick最大处理的技能数量
    
    Person* p;  // 所属的人物对象指针

    std::unordered_map<std::string, int> skillIndexCache{};  // 技能名->索引缓存

    // ============================================================================
    // 受保护的成员函数声明（内部使用）
    // ============================================================================
    
    /**
     * @brief 获取技能在skillCDList的索引
     * @param skillName 技能名称
     * @details 
     * 1.通过skillIndexCache快速寻找技能返回索引
     * 2.未在skillIndexCache找到则执行findSkillInSkillCDList()
     * 3.如果findSkillInSkillCDList()找到（即 != -1）则将其添加进skillIndexCache
     * 4.返回index
     */
     int getSkillIndex(const std::string& skillName);

    /**
     * @brief 手动添加优先级技能（允许重复）
     * @param skillName 技能名称
     * @details 将技能添加到待释放列表，即使已存在也会添加
     */
    void maniAddPriorSkillList(std::string skillName);
    
    /**
     * @brief 自动添加优先级技能（去重）
     * 
     * @param skillName 技能名称
     * @details 只有在列表中不存在时才会添加
     */
    void autoAddPriorSkillList(std::string skillName);
    
    /**
     * @brief 自动释放技能的主逻辑
     * 
     * @details 按照以下顺序判断：
     * 1. 如果正在释放技能，直接返回
     * 2. 如果有待释放的优先级技能，释放第一个
     * 3. 否则按技能优先级列表寻找可释放技能
     * 4. 检查技能冷却、层数、释放条件等
     */
    void createSkillByAuto();

    /**
     * @brief 统一更新person类相关update函数
     * 
     * @details 更新相关update函数如下:
     * 1.updateBuffs
     * 2.updateSkills
     * 3.updateSkillsCD
     */
    void updatePerson(int deltaTime);

    /**
     * @brief 根据模拟时间释放技能
     */
    void createSkillInCertainTime(double time, std::string skillName);
    
    // ============================================================================
    // 需要子类实现的具体战斗逻辑，以下函数为大爆发，小爆发，空窗期的三段式轴
    // 仅用作参考，实际实现可自行发挥
    // ============================================================================
    
    /**
     * @brief 大爆发触发时的具体逻辑
     */
    virtual void largeOutBurst(){};
    
    /**
     * @brief 小爆发触发时的具体逻辑
     */
    virtual void smallOutBurst(){};
    
    /**
     * @brief 检查并触发大爆发
     */
    virtual void checkAndTriggerLargeOutBurst(){};
    
    /**
     * @brief 检查并触发小爆发
     */
    virtual void checkAndTriggerSmallOutBurst(){};
    
    /**
     * @brief 检查并结束爆发状态
     */
    virtual void checkAndFinishOutBurst(){};
    
    /**
     * @brief 检查并进入爆发状态
     */
    virtual void checkAndSetOutBurstState(){};
    
    /**
     * @brief 空窗期逻辑
     * @details 不在爆发状态时执行的常规逻辑
     */
    virtual void windowPeriodLogic(){};
    
    /**
     * @brief 添加技能释放的禁止条件
     * 
     * @param condition 条件判断函数
     * @param errorType 条件不满足时的错误类型
     * 
     * @details 当condition返回true时，表示技能不能释放
     */
    void addJudgingConditionsForCantCreateSkill(std::function<bool(const PriorSkillInfo&)> condition, 
                                                ErrorInfo::errorTypeEnum errorType);
    
    /**
     * @brief 立即结束爆发状态
     * 
     * @details 重置所有技能优先级并更新状态标志
     */
    void finishOutBurst();

public:
    // ============================================================================
    // 友元声明
    // ============================================================================
    
    friend class Initializer;  // 允许Initializer类访问所有成员

    // ============================================================================
    // 公共成员函数声明
    // ============================================================================
    
    /**
     * @brief 每tick更新自动攻击逻辑
     * 
     * @param deltaTime 距离上次更新的时间间隔
     * 
     * @note 这是AutoAttack的核心函数，在游戏主循环中调用
     */
    virtual void update(int deltaTime) = 0;
    
    /**
     * @brief 更新技能状态为"正在释放"
     * 
     * @details 设置isReleasingSkill标志位，防止技能打断
     */
    void updateSkillRelease();
    
    /**
     * @brief 更新技能状态为"释放完成"
     * 
     * @details 清除isReleasingSkill标志位，允许下一个技能释放
     */
    void updateSkillFinish();
    
    /**
     * @brief 更改指定技能的优先级
     * 
     * @param skillName 技能名称
     * @param priority 新的优先级（数值越大优先级越高）
     * 
     * @details 修改后会重新排序技能优先级列表
     */
    void changePriority(std::string skillName, int priority);
    
    /**
     * @brief 重置所有技能优先级为0
     */
    void resetPriority();
    
    /**
     * @brief 重置指定技能的优先级为0
     * 
     * @param skillName 技能名称
     */
    void resetCertainPriority(std::string skillName);
    
    // ============================================================================
    // 访问器和修改器
    // ============================================================================

    static double getTimer();
    static double& setTimer();
    static double getDeltaTime();
    bool getIsReleasingSkill() const;
    bool getIsOutBurst() const;
    int getNextOutBurstType() const;
    unsigned short getMaxProcessPerTick() const;
    
    // ============================================================================
    // 构造与析构函数
    // ============================================================================
    
    /**
     * @brief 构造函数
     * 
     * @param p 所属的人物对象指针
     * 
     * @details 初始化时重置计时器并设置默认爆发类型为大爆发
     */
    AutoAttack(Person* p);
    virtual ~AutoAttack() = default;
};