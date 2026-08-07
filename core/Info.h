#pragma once
#include <memory>
#include <string>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <vector>

// 前向声明
class Action;

// ============================================================================
// Info 基类
// ============================================================================

/**
 * @class Info
 * @brief 信息基类
 * 
 * @details 所有信息类型（伤害、技能优先级、错误等）的抽象基类
 *          用于实现多态信息处理和存储
 */
class Info
{
public:
    virtual ~Info() = default;  // 虚析构函数确保正确销毁派生类
};

// ============================================================================
// DamageInfo 类 - 伤害信息
// ============================================================================

/**
 * @class DamageInfo
 * @brief 伤害信息类
 * 
 * 主要特性：
 * 1. 记录单个技能造成的伤害详情
 * 2. 包含暴击和幸运伤害信息
 * 3. 时间戳记录伤害发生时间
 * 4. 支持流输出操作符，便于日志记录
 * 
 * 使用场景：
 * 1. 技能造成伤害时创建并记录
 * 2. 伤害统计和数据分析
 * 3. 战斗日志记录
 */
class DamageInfo : public Info
{
public:
    std::string skillName = "None_DamageInfo";  // 造成伤害的技能名称
    double damageNum = 0;                       // 基础伤害值
    double luckyNum = 0;                        // 幸运伤害值（额外加成）
    bool isCritical = false;                    // 是否暴击
    bool isLucky = false;                       // 是否触发幸运效果
    double time = 0;                            // 伤害发生的时间点（使用AutoAttack计时器）

public:
    /**
     * @brief 获取伤害信息对象
     * 
     * @return const DamageInfo 当前对象的副本
     */
    const DamageInfo& get();
    
    /**
     * @brief 默认构造函数
     * @details 初始化时间为当前AutoAttack计时器值
     */
    DamageInfo();
    
    /**
     * @brief 参数化构造函数
     * 
     * @param skillName 技能名称
     * @param damageNum 基础伤害值
     * @param luckyNum 幸运伤害值
     * @param isCritical 是否暴击
     * @param isLucky 是否幸运
     * @details 使用提供的参数完全初始化伤害信息
     */
    DamageInfo(std::string skillName, double damageNum, double luckyNum, bool isCritical, bool isLucky);
    
    /**
     * @brief 拷贝构造函数（默认）
     */
    DamageInfo(const DamageInfo& other) = default;
    
    /**
     * @brief 赋值操作符重载
     * 
     * @param other 要复制的DamageInfo对象
     * @return DamageInfo& 当前对象的引用
     */
    DamageInfo &operator=(const DamageInfo &other);
    
    /**
     * @brief 流输出操作符重载（友元函数）
     * 
     * @param os 输出流
     * @param damageInfo 要输出的DamageInfo对象
     * @return std::ostream& 输出流引用
     */
    friend std::ostream &operator<<(std::ostream &os, const DamageInfo& damageInfo);
};

// ============================================================================
// PriorSkillInfo 类 - 优先级技能信息
// ============================================================================

/**
 * @class PriorSkillInfo
 * @brief 优先级技能信息类
 * 
 * 主要特性：
 * 1. 记录技能的优先级和添加方式
 * 2. 区分自动添加和手动添加的技能
 * 3. 用于AutoAttack系统中的技能调度
 * 
 * 使用场景：
 * 1. AutoAttack的优先级技能列表管理
 * 2. 技能释放顺序控制
 * 3. 爆发状态下的技能序列管理
 */
class PriorSkillInfo : public Info
{
public:
    std::string skillName = "None_PriorSkillInfo";  // 技能名称
    bool isAutoAdd = false;                         // 是否为自动添加到优先级列表
    bool isManiAdd = false;                         // 是否为手动添加到优先级列表
    int priority = 0;                               // 优先级数值（越大优先级越高）

    /**
     * @brief 构造函数（优先级版本）
     * 
     * @param skillName 技能名称
     * @param priority 优先级数值
     */
    PriorSkillInfo(std::string skillName, int priority);
    
    /**
     * @brief 构造函数（添加方式版本）
     * 
     * @param skillName 技能名称
     * @param isAutoAdd 是否为自动添加
     * @param isManiAdd 是否为手动添加
     */
    PriorSkillInfo(std::string skillName, bool isAutoAdd, bool isManiAdd);
    
    /**
     * @brief 完全参数构造函数
     * 
     * @param skillName 技能名称
     * @param isAutoAdd 是否为自动添加
     * @param isManiAdd 是否为手动添加
     * @param priority 优先级数值
     */
    PriorSkillInfo(std::string skillName, bool isAutoAdd, bool isManiAdd, int priority);
};

// ============================================================================
// ErrorInfo 类 - 错误信息
// ============================================================================

/**
 * @class ErrorInfo
 * @brief 错误信息类
 * 
 * 主要特性：
 * 1. 记录游戏中各种技能相关的错误
 * 2. 支持错误类型枚举与字符串的双向转换
 * 3. 时间戳记录错误发生时间
 * 4. 支持流输出操作符，便于错误日志记录
 * 
 * 错误类型包括：
 * - 层数不足、CD不足、充能CD不足
 * - 技能释放冲突、技能未找到
 * - 资源不足、未知Buff类型等
 */
class ErrorInfo : public Info
{
public:
    std::string skillName = "None_ErrorInfo";  // 发生错误的技能名称
    double time = 0;                            // 错误发生的时间点
    
    /**
     * @enum errorTypeEnum
     * @brief 错误类型枚举
     * 
     * 定义游戏中所有可能的技能相关错误类型
     */
    enum class errorTypeEnum
    {
        Stack_Not_Enough,       // 层数不足
        CD_Not_Enough,          // 普通CD不足
        ChargeCD_Not_Enough,    // 充能CD不足
        Now_Releasing_Skill,    // 正在释放技能（技能冲突）
        Skill_Not_Found,        // 技能未找到
        Resource_Not_Enough,    // 资源不足（如玄冰）
        Unknown_Buff_Type       // 未知Buff类型
    };
    
    ErrorInfo::errorTypeEnum errorType{};  // 具体的错误类型

    /**
     * @brief 构造函数
     * 
     * @param skillName 发生错误的技能名称
     * @param errorType 错误类型枚举值
     */
    ErrorInfo(std::string skillName, ErrorInfo::errorTypeEnum errorType);

    // ============================================================================
    // 枚举字符串转换相关静态方法
    // ============================================================================
    
    /**
     * @brief 错误类型枚举转字符串
     * 
     * @param type 错误类型枚举值
     * @return std::string 对应的字符串表示
     */
    static std::string errorTypeToString(errorTypeEnum type);
    
    /**
     * @brief 字符串转错误类型枚举
     * 
     * @param str 错误类型字符串
     * @return errorTypeEnum 对应的枚举值
     * @throws std::invalid_argument 当字符串无法识别时抛出异常
     */
    static errorTypeEnum stringToErrorType(const std::string& str);
    
    /**
     * @brief 获取错误类型映射表
     * 
     * @return const std::map<errorTypeEnum, std::string>& 错误类型映射表引用
     */
    static const std::map<errorTypeEnum, std::string>& getErrorTypeMap();
    
    /**
     * @brief 获取所有错误类型的字符串列表
     * 
     * @return std::vector<std::string> 错误类型字符串列表
     */
    static std::vector<std::string> getAllErrorTypeNames();

    /**
     * @brief 流输出操作符重载（友元函数）
     * 
     * @param os 输出流
     * @param errorInfo 要输出的ErrorInfo对象
     * @return std::ostream& 输出流引用
     */
    friend std::ostream &operator<<(std::ostream &os, const ErrorInfo& errorInfo);
};

// ============================================================================
// ActionInfo 类 - 动作信息
// ============================================================================

/**
 * @class ActionInfo
 * @brief 动作信息类
 * 
 * 主要特性：
 * 1. 封装游戏动作及其相关参数
 * 2. 使用unique_ptr管理Action对象生命周期
 * 3. 禁止拷贝但允许移动，确保资源唯一性
 * 
 * 使用场景：
 * 1. 动作队列管理
 * 2. 延迟动作执行
 * 3. 动作序列化存储
 */
class ActionInfo : public Info
{
public:
    std::unique_ptr<Action> actionPtr = nullptr;  // Action对象的智能指针
    double number = 0;                             // 动作相关数值参数

    /**
     * @brief 构造函数
     * 
     * @param number 动作相关数值参数
     * @param actionPtr Action对象的unique_ptr（使用std::move传入）
     */
    ActionInfo(double number, std::unique_ptr<Action> actionPtr);

    // 明确禁止拷贝（确保Action对象唯一性）
    ActionInfo(const ActionInfo&) = delete;
    ActionInfo& operator=(const ActionInfo&) = delete;
    
    // 允许移动语义（转移所有权）
    ActionInfo(ActionInfo&&) = default;
    ActionInfo& operator=(ActionInfo&&) = default;
    
    ~ActionInfo() = default;
};