#include "Info.h"
#include "AutoAttack.h"
#include "Action.h"
#include <iostream>
#include <algorithm>
#include <cctype>

// 前向声明
class Skill;

// ============================================================================
// DamageInfo 实现
// ============================================================================

/**
 * @brief DamageInfo默认构造函数
 * @details 初始化时间为当前AutoAttack计时器值
 */
DamageInfo::DamageInfo()
            : time(AutoAttack::getTimer())
            {}

/**
 * @brief DamageInfo参数化构造函数
 * 
 * @param skillName 技能名称
 * @param damageNum 基础伤害值
 * @param luckyNum 幸运伤害值
 * @param isCritical 是否暴击
 * @param isLucky 是否幸运
 */
DamageInfo::DamageInfo(std::string skillName, double damageNum, double luckyNum, bool isCritical, bool isLucky)
            : skillName(skillName),
              damageNum(damageNum),
              luckyNum(luckyNum),
              isCritical(isCritical),
              isLucky(isLucky),
              time(AutoAttack::getTimer())  // 使用当前时间戳
            {}

/**
 * @brief 获取当前DamageInfo对象
 * 
 * @return const DamageInfo 当前对象的常量引用
 */
const DamageInfo& DamageInfo::get() { return *this; }

/**
 * @brief 赋值操作符重载实现
 * 
 * @param other 要复制的DamageInfo对象
 * @return DamageInfo& 当前对象的引用
 */
DamageInfo &DamageInfo::operator=(const DamageInfo &other)
{
    this->skillName = other.skillName;
    this->damageNum = other.damageNum;
    this->luckyNum = other.luckyNum;
    this->isCritical = other.isCritical;
    this->isLucky = other.isLucky;
    this->time = other.time;
    return *this;
}

/**
 * @brief 流输出操作符重载实现
 * 
 * @param os 输出流
 * @param damageInfo 要输出的DamageInfo对象
 * @return std::ostream& 输出流引用
 */
std::ostream& operator<<(std::ostream& os, const DamageInfo& damageInfo) {
    os << "[DamageInfo]: time: " << damageInfo.time 
    << ", skillName:" << damageInfo.skillName
    << ", damageNum:" << damageInfo.damageNum
    << ", luckyNum:" << damageInfo.luckyNum
    << ", isCritical:" << damageInfo.isCritical;
    return os;
}

// ============================================================================
// PriorSkillInfo 实现
// ============================================================================

/**
 * @brief PriorSkillInfo构造函数（完全参数）
 * 
 * @param skillName 技能名称
 * @param isAutoAdd 是否为自动添加
 * @param isManiAdd 是否为手动添加
 * @param priority 优先级数值
 */
PriorSkillInfo::PriorSkillInfo(std::string skillName, bool isAutoAdd, bool isManiAdd, int priority)
                : skillName(skillName),
                  isAutoAdd(isAutoAdd),
                  isManiAdd(isManiAdd),
                  priority(priority)
                {}

/**
 * @brief PriorSkillInfo构造函数（添加方式版本）
 * 
 * @param skillName 技能名称
 * @param isAutoAdd 是否为自动添加
 * @param isManiAdd 是否为手动添加
 */
PriorSkillInfo::PriorSkillInfo(std::string skillName, bool isAutoAdd, bool isManiAdd)
                : skillName(skillName),
                  isAutoAdd(isAutoAdd),
                  isManiAdd(isManiAdd)
                {}

/**
 * @brief PriorSkillInfo构造函数（优先级版本）
 * 
 * @param skillName 技能名称
 * @param priority 优先级数值
 */
PriorSkillInfo::PriorSkillInfo(std::string skillName, int priority)
                : skillName(skillName),
                  priority(priority)
                {}

// ============================================================================
// ErrorInfo 实现
// ============================================================================

/**
 * @brief ErrorInfo构造函数
 * 
 * @param skillName 发生错误的技能名称
 * @param errorType 错误类型枚举值
 */
ErrorInfo::ErrorInfo(std::string skillName, ErrorInfo::errorTypeEnum errorType)
            : skillName(skillName),
              time(AutoAttack::getTimer()),  // 记录当前时间
              errorType(errorType)
            {}

/**
 * @brief errorTypeEnum到字符串的静态映射表
 * 
 * @details 在静态初始化时创建，确保线程安全（C++11保证）
 */
static const std::map<ErrorInfo::errorTypeEnum, std::string> errorTypeMap = {
    {ErrorInfo::errorTypeEnum::Stack_Not_Enough, "STACK_NOT_ENOUGH"},
    {ErrorInfo::errorTypeEnum::CD_Not_Enough, "CD_NOT_ENOUGH"},
    {ErrorInfo::errorTypeEnum::ChargeCD_Not_Enough, "CHARGECD_NOT_ENOUGH"},
    {ErrorInfo::errorTypeEnum::Now_Releasing_Skill, "NOW_RELEASING_SKILL"},
    {ErrorInfo::errorTypeEnum::Skill_Not_Found, "SKILL_NOT_FOUND"},
    {ErrorInfo::errorTypeEnum::Resource_Not_Enough, "ICE_NOT_ENOUGH"},
    {ErrorInfo::errorTypeEnum::Unknown_Buff_Type, "UNKNOWN_BUFF_TYPE"}
};

/**
 * @brief 字符串到errorTypeEnum的反向映射表
 * 
 * @details 使用lambda函数初始化，确保在errorTypeMap之后初始化
 */
static const std::map<std::string, ErrorInfo::errorTypeEnum> reverseErrorTypeMap = []() {
    std::map<std::string, ErrorInfo::errorTypeEnum> reverseMap;
    for (const auto& pair : errorTypeMap) {
        reverseMap[pair.second] = pair.first;
    }
    return reverseMap;
}();

/**
 * @brief 错误类型枚举转字符串实现
 * 
 * @param type 错误类型枚举值
 * @return std::string 对应的字符串表示
 */
std::string ErrorInfo::errorTypeToString(errorTypeEnum type) {
    auto it = errorTypeMap.find(type);
    if (it != errorTypeMap.end()) {
        return it->second;
    }
    // 未知类型处理
    return "[Error,timer=" + std::to_string(AutoAttack::getTimer()) + "]: Error - Unknown error type turn to string";
}

/**
 * @brief 字符串转错误类型枚举实现
 * 
 * @param str 错误类型字符串
 * @return errorTypeEnum 对应的枚举值
 * @throws std::invalid_argument 当字符串无法识别时抛出异常
 */
ErrorInfo::errorTypeEnum ErrorInfo::stringToErrorType(const std::string& str) {
    // 1. 尝试精确匹配
    auto it = reverseErrorTypeMap.find(str);
    if (it != reverseErrorTypeMap.end()) {
        return it->second;
    }
    
    // 2. 尝试不区分大小写匹配
    std::string upperStr = str;
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);
    
    for (const auto& pair : errorTypeMap) {
        std::string upperName = pair.second;
        std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
        
        if (upperStr == upperName) {
            return pair.first;
        }
    }
    
    // 3. 尝试处理常见的缩写或别名
    static const std::map<std::string, ErrorInfo::errorTypeEnum> aliasMap = {
        {"STACK", ErrorInfo::errorTypeEnum::Stack_Not_Enough},
        {"CD", ErrorInfo::errorTypeEnum::CD_Not_Enough},
        {"CHARGECD", ErrorInfo::errorTypeEnum::ChargeCD_Not_Enough},
        {"RELEASING", ErrorInfo::errorTypeEnum::Now_Releasing_Skill},
        {"SKILL", ErrorInfo::errorTypeEnum::Skill_Not_Found},
        {"ICE", ErrorInfo::errorTypeEnum::Resource_Not_Enough},
        {"BUFF", ErrorInfo::errorTypeEnum::Unknown_Buff_Type}
    };
    
    std::string upperAlias = upperStr;
    auto aliasIt = aliasMap.find(upperAlias);
    if (aliasIt != aliasMap.end()) {
        return aliasIt->second;
    }
    
    // 4. 所有匹配失败，抛出异常
    throw std::invalid_argument("[Error,timer=" + std::to_string(AutoAttack::getTimer()) + "]: Error - Unknown string turn to error type : " + str);
}

/**
 * @brief 获取错误类型映射表实现
 * 
 * @return const std::map<errorTypeEnum, std::string>& 错误类型映射表引用
 */
const std::map<ErrorInfo::errorTypeEnum, std::string>& ErrorInfo::getErrorTypeMap() {
    return errorTypeMap;
}

/**
 * @brief 获取所有错误类型字符串列表实现
 * 
 * @return std::vector<std::string> 错误类型字符串列表
 */
std::vector<std::string> ErrorInfo::getAllErrorTypeNames() {
    std::vector<std::string> names;
    for (const auto& pair : errorTypeMap) {
        names.push_back(pair.second);
    }
    return names;
}

/**
 * @brief 流输出操作符重载实现
 * 
 * @param os 输出流
 * @param errorInfo 要输出的ErrorInfo对象
 * @return std::ostream& 输出流引用
 */
std::ostream& operator<<(std::ostream& os, const ErrorInfo& errorInfo) {
    os << "[ErrorInfo]: time: " << errorInfo.time 
    << " skillType:" << errorInfo.skillName
    << " errorType:" << ErrorInfo::errorTypeToString(errorInfo.errorType);
    return os;
}

// ============================================================================
// ActionInfo 实现
// ============================================================================

/**
 * @brief ActionInfo构造函数
 * 
 * @param number 动作相关数值参数
 * @param actionPtr Action对象的unique_ptr
 */
ActionInfo::ActionInfo(double number, std::unique_ptr<Action> actionPtr)
            : actionPtr(std::move(actionPtr)),  // 转移所有权
              number(number)
            {}

// 以下三个特殊成员函数必须在 Action 完整类型可用处定义
ActionInfo::ActionInfo(ActionInfo&&) noexcept = default;
ActionInfo& ActionInfo::operator=(ActionInfo&&) noexcept = default;
ActionInfo::~ActionInfo() = default;