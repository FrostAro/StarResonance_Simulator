#include "Skill.h"
#include "AutoAttack.h"
#include <iostream>

// ============================================================================
// 静态成员初始化
// ============================================================================

/**
 * @brief 静态ID计数器初始化
 */
int Skill::ID = 0;

/**
 * @brief 静态技能名称初始化
 */
std::string Skill::name = "skill";

// ============================================================================
// Skill 类实现
// ============================================================================

/**
 * @brief Skill构造函数
 * @details 分配技能ID并递增计数器
 */
Skill::Skill() 
    : skillID(ID)  // 使用当前ID值
{
    ID++;  // 递增静态ID计数器
}

/**
 * @brief Skill析构函数（默认实现）
 */
Skill::~Skill() = default;

/**
 * @brief 判断技能是否应该被移除（暂时弃用）
 * 
 * @return bool 总是返回false
 * @deprecated 此函数暂时弃用
 */
bool Skill::shouldBeRemoved()
{
    return false;
}

/**
 * @brief 减少持续时间（步长1）
 * 
 * @return bool true=成功减少，false=持续时间已无效
 */
bool Skill::reduceDuration()
{
    if (this->duration >= 0)
    {
        this->duration -= 1;
        return true;
    }
    return false;
}

/**
 * @brief 增加伤害触发计时器（步长1）
 * 
 * @return bool true=成功增加，false=已达到触发间隔
 */
bool Skill::addDamageTriggerTimer()
{
    if (this->damageTriggerTimer < damageTriggerInterval)
    {
        this->damageTriggerTimer += 1;
        return true;
    }
    return false;
}

/**
 * @brief 减少前摇时间（步长1）
 * 
 * @return bool true=成功减少，false=前摇时间已无效
 */
bool Skill::reduceReleasingTime()
{
    if (this->releasingTime >= 0)
    {
        this->releasingTime -= 1;
        return true;
    }
    return false;
}

/**
 * @brief 减少持续时间（指定步长）
 * 
 * @param n 要减少的时间量
 * @return bool true=成功减少，false=参数无效
 */
bool Skill::reduceDuration(const double n)
{
    this->duration -= n;
    return true;
}

/**
 * @brief 增加伤害触发计时器（指定步长）
 * 
 * @param n 要增加的时间量
 * @return bool true=成功增加，false=已达到触发间隔
 */
bool Skill::addDamageTriggerTimer(const double n)
{
    this->damageTriggerTimer += n;
    return true;
}

/**
 * @brief 减少前摇时间（指定步长）
 * 
 * @param n 要减少的时间量
 * @return bool true=成功减少，false=参数无效
 */
bool Skill::reduceReleasingTime(const double n)
{
    this->releasingTime -= n;
    return true;
}

void Skill::changeEnergyReduceUP(double n)
{
    this->energyReduceUP += n;
    if(this->energyReduceUP < 0)
    {
        this->energyReduceUP = 0;
    }
}

void Skill::changeEnergyReduceDOWN(double n)
{
    this->energyReduceDOWN += n;
    if(this->energyReduceDOWN > 1)
    {
        this->energyReduceDOWN = 1;
    }
}

void Skill::changeEnergyAddIncrease(double n)
{
    this->energyAddIncrease += n;
}

// ============================================================================
// 技能类型枚举映射表
// ============================================================================

/**
 * @brief skillTypeEnum到字符串的静态映射表
 */
static const std::map<Skill::skillTypeEnum, std::string> skillTypeMap = {
    {Skill::skillTypeEnum::NONE, "NONE"},
    {Skill::skillTypeEnum::NORMAL, "NORMAL"},
    {Skill::skillTypeEnum::SPECIALIZED, "SPECIALIZED"},
    {Skill::skillTypeEnum::PARTICULAR, "PARTICULAR"},
    {Skill::skillTypeEnum::FAR, "FAR"},
    {Skill::skillTypeEnum::CLOSE, "CLOSE"},
    {Skill::skillTypeEnum::WITCHCRAFT, "WITCHCRAFT"},
    {Skill::skillTypeEnum::PHYSICS, "PHYSICS"}
};

/**
 * @brief 字符串到skillTypeEnum的反向映射表
 * 
 * @details 使用lambda函数初始化，确保在skillTypeMap之后初始化
 */
static const std::map<std::string, Skill::skillTypeEnum> reverseSkillTypeMap = []()
{
    std::map<std::string, Skill::skillTypeEnum> reverseMap;
    for (const auto &pair : skillTypeMap)
    {
        reverseMap[pair.second] = pair.first;
    }
    return reverseMap;
}();

/**
 * @brief 技能类型枚举转字符串实现
 * 
 * @param type 技能类型枚举值
 * @return std::string 对应的字符串表示
 */
std::string Skill::skillTypeToString(skillTypeEnum type)
{
    auto it = skillTypeMap.find(type);
    if (it != skillTypeMap.end())
    {
        return it->second;
    }
    // 未知类型处理
    return "[Error,timer=" + std::to_string(AutoAttack::getTimer()) + "]: Error       - Unknown skilltype turn to string";
}

/**
 * @brief 字符串转技能类型枚举实现
 * 
 * @param str 技能类型字符串
 * @return skillTypeEnum 对应的枚举值
 * @throws std::invalid_argument 当字符串无法识别时抛出异常
 */
Skill::skillTypeEnum Skill::stringToSkillType(const std::string &str)
{
    // 1. 尝试精确匹配
    auto it = reverseSkillTypeMap.find(str);
    if (it != reverseSkillTypeMap.end())
    {
        return it->second;
    }

    // 2. 尝试不区分大小写匹配
    std::string upperStr = str;
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);

    for (const auto &pair : skillTypeMap)
    {
        std::string upperName = pair.second;
        std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);

        if (upperStr == upperName)
        {
            return pair.first;
        }
    }

    // 3. 所有匹配失败，抛出异常
    throw std::invalid_argument("[Eorror,timer=" + std::to_string(AutoAttack::getTimer()) + "]: Error       - Unknown string turn to skilltype : " + str);
}

/**
 * @brief 获取技能类型映射表实现
 * 
 * @return const std::map<skillTypeEnum, std::string>& 技能类型映射表引用
 */
const std::map<Skill::skillTypeEnum, std::string> &Skill::getSkillTypeMap()
{
    return skillTypeMap;
}

/**
 * @brief 获取所有技能类型字符串列表实现
 * 
 * @return std::vector<std::string> 技能类型字符串列表
 */
std::vector<std::string> Skill::getAllSkillTypeNames()
{
    std::vector<std::string> names;
    for (const auto &pair : skillTypeMap)
    {
        names.push_back(pair.second);
    }
    return names;
}

/**
 * @brief 流输出操作符重载实现（用于skillTypeEnum）
 * 
 * @param os 输出流
 * @param type 技能类型枚举值
 * @return std::ostream& 输出流引用
 */
std::ostream &operator<<(std::ostream &os, Skill::skillTypeEnum type)
{
    os << Skill::skillTypeToString(type);
    return os;
}

/**
 * @brief 将技能类型列表转换为字符串表示
 * 
 * @param typeList 技能类型枚举列表
 * @return std::string 格式化的字符串，如"[NORMAL, CLOSE, PHYSICS]"
 */
std::string Skill::skillTypeListToString(const std::vector<Skill::skillTypeEnum> &typeList)
{
    std::string result = "[";
    for (size_t i = 0; i < typeList.size(); ++i)
    {
        if (i > 0)
            result += ", ";
        result += Skill::skillTypeToString(typeList.at(i));
    }
    result += "]";
    return result;
}

/**
 * @brief 判断技能是否拥有指定类型实现
 * 
 * @param type 要检查的技能类型
 * @return bool true=拥有该类型，false=不拥有
 */
bool Skill::hasSkillType(skillTypeEnum type) const
{
    return std::find(skillTypeList.begin(), skillTypeList.end(), type) != skillTypeList.end();
}

// ============================================================================
// 访问器函数实现
// ============================================================================

int Skill::getSkillID() const { return this->skillID; }
double Skill::getAdd() const { return this->energyAdd; }
double Skill::getEnergyReduce() const { return this->energyReduce; }
bool Skill::getCanTriggerLucky() const { return this->canTriggerLucky; }
void Skill::setCanTriggerLucky(bool value) { this->canTriggerLucky = value; }
bool Skill::getCanCharge() const { return this->canCharge; }
bool Skill::getIsEquipped() const { return this->isEquipped; }
bool Skill::getTriggered() const { return this->triggered; }
bool Skill::getIsInstant() const { return this->isInstant; }
bool Skill::getIsContinuous() const { return this->isContinuous; }
bool Skill::getIsNoReleasing() const { return this->isNoReleasing; }
bool Skill::getIsFacilitation() const { return this->isFacilitation; }

double Skill::getEnergyReduceUP() const { return this->energyReduceUP; }
double Skill::getEnergyReduceDOWN() const { return this->energyReduceDOWN; }
double Skill::getEnergyAddIncrease() const { return this->energyAddIncrease; }

double Skill::getCriticalIncreaseAdd() const { return this->criticalIncreaseAdd; }
double Skill::getElementIncreaseAdd() const { return this->elementIncreaseAdd; }
double Skill::getDamageIncreaseAdd() const { return this->damageIncreaseAdd; }
double Skill::getAlmightyIncreaseAdd() const { return this->almightyIncreaseAdd; }
double Skill::getDreamIncreaseAdd() const { return this->dreamIncreaseAdd; }
double Skill::getFinalIncreaseAdd() const { return this->finalIncreaseAdd; }
double Skill::getMultiplyingIncrease() const { return this->multiplyingIncrease; }

double Skill::getMaxCD() const { return this->MaxCD; }
double &Skill::getCurrentCD_Ref() { return this->CD; }
double Skill::getCurrentCD_Value() const { return this->CD; }
double Skill::getMaxChargedCD() const { return this->MaxchargeCD; }
double &Skill::getCurrentChargedCD_Ref() { return this->chargeCD; }
double Skill::getCurrentChargedCD_Value() const { return this->chargeCD; }
void Skill::setCD(const double cd) { this->CD = cd; }
int &Skill::getStackRef() { return this->stack; }
int Skill::getStackValue() const { return this->stack; }
int Skill::getMaxStack() const { return this->maxStack; }
double Skill::getMutiplying() const { return this->multiplying; }
double Skill::getFixedValue() const { return this->fixedValue; }
double Skill::getLuckyFiexedValue() const { return this->luckyFiexedValue; }
double Skill::getReleasingTime() const { return this->releasingTime; }
double Skill::getDuration() const { return this->duration; }
double Skill::getDamageTriggerInterval() const { return this->damageTriggerInterval; }
double Skill::getDamageTriggerTimer() const { return this->damageTriggerTimer; }
int Skill::getSinging() const { return this->singing; }
std::vector<Skill::skillTypeEnum> Skill::getSkillType() { return this->skillTypeList; }

// ============================================================================
// InstantSkill 类实现
// ============================================================================

/**
 * @brief InstantSkill构造函数
 * @details 设置isInstant标志为true
 */
InstantSkill::InstantSkill() : Skill()
{
    this->isInstant = true;
}

/**
 * @brief InstantSkill::use实现
 * 
 * @details 如果技能尚未触发，则调用trigger()并设置triggered标志
 */
void InstantSkill::use(Person *p)
{   
    if(!this->triggered){
        this->trigger(p);      // 触发具体效果
        this->triggered = true; // 标记为已触发
    }
}

// ============================================================================
// ContinuousSkill 类实现
// ============================================================================

/**
 * @brief ContinuousSkill构造函数
 * @details 设置isContinuous标志为true
 */
ContinuousSkill::ContinuousSkill() : Skill()
{
    this->isContinuous = true;
}

/**
 * @brief ContinuousSkill::use实现
 * 
 * @details 当伤害触发计时器达到触发间隔时，调用trigger()函数
 */
void ContinuousSkill::use(Person *p)
{
    addDamageTriggerTimer(AutoAttack::getDeltaTime());
    if(this->damageTriggerTimer >= this->damageTriggerInterval){
        this->trigger(p);  // 触发具体效果
        this->damageTriggerTimer -= damageTriggerInterval;
    }
}

// ============================================================================
// FacilitationSkill 类实现
// ============================================================================

/**
 * @brief FacilitationSkill构造函数
 * @details 设置isFacilitation标志为true
 */
FacilitationSkill::FacilitationSkill() : Skill()
{
    this->isFacilitation = true;
}

/**
 * @brief FacilitationSkill::use实现
 * 
 * @details 当伤害触发计时器达到触发间隔时，调用trigger()函数
 */
void FacilitationSkill::use(Person* p)
{
    addDamageTriggerTimer(AutoAttack::getDeltaTime());
    if(this->damageTriggerTimer >= this->damageTriggerInterval){
        this->trigger(p);  // 触发具体效果
        this->damageTriggerTimer -= damageTriggerInterval;
    }
}

void FacilitationSkill::stop()
{
    this->duration = 0;
}

std::string temp_InstantSkill::name = "temp_InstantSkill";
temp_InstantSkill::temp_InstantSkill(std::string skillName,double multiplying,double fixedValue) : InstantSkill()
{
    this->name = skillName;
    this->multiplying = multiplying;
    this->fixedValue = fixedValue;
}

std::string temp_InstantSkill::getSkillName() const { return temp_InstantSkill::name; }