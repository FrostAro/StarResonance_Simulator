#pragma once
#include <memory>
#include <utility>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_map>
#include <queue>
#include "Buff.h"
#include "Skill.h"
#include "Info.h"

class AutoAttack;
class DamageStatistics;
class Skill;
class Buff;
class FightingFantasy;
class DamageStatistics;
class DamageInfo;
class ActionInfo;
class ErrorInfo;
class Initializer;

/**
 * @class Person
 * @brief 游戏角色基类，管理角色属性、技能、buff 和战斗逻辑
 * 
 * 核心职责：
 * - 管理角色属性（暴击、幸运、攻击力等）
 * - 处理技能释放和冷却
 * - 管理 buff/debuff 效果
 * - 计算伤害和触发概率
 * 
 * 设计模式：
 * - 组件模式：Skill、Buff 作为组件
 * - 状态模式：通过状态管理技能释放
 * - 观察者模式：事件触发机制
 */
class Person
{
private:
    // ==== 私有成员：只在内部使用 ====
    std::vector<std::unique_ptr<Skill>> pointerListForAction{};     // 用于Action的原始指针列表
    std::unique_ptr<Skill> nowReleasingSkill = nullptr;             // 当前正在释放的技能
    int propertyTransformationCoeffcient_General = 0;               // 一般属性转化系数（游戏数值平衡）
    int propertyTransformationCoeffcient_Almighty = 0;              // 全能属性转化系数（游戏数值平衡）
	int totalTime = 0;                                              // 模拟总时间（毫秒）

	// ==== 状态 ====
    bool isReleasingSkill = false;                                  // 是否正在释放技能
    
    // ==== 数据容器 ====
    std::queue<ActionInfo> actionQueue{};                           // 动作事件队列
    std::vector<std::string> skillInfoList{};                       // 技能信息列表（用于调试）
    std::vector<DamageInfo> damageInfoList{};                       // 伤害记录列表
    std::vector<ErrorInfo> errorInfoList{};                         // 错误信息列表

	// ==== 随机系统 ====
	mutable std::mt19937 randomEngine;                              // 随机数引擎（线程安全）
	mutable std::uniform_int_distribution<int> intDist{0, 9999};    

protected:
    // ==== 受保护成员：子类可以访问 ====
    std::vector<std::unique_ptr<Buff>> buffList{};                  // 生效中的buff列表
    std::vector<std::unique_ptr<Skill>> continuousSkillList{};      // 持续性技能列表（如涡流）
    std::vector<std::unique_ptr<Skill>> noReleasingSkillList{};     // 无前摇技能列表
    std::vector<std::unique_ptr<Skill>> skillCDList{};              // 技能冷却列表
    
    // ==== 职业特定系数（由子类设置） ====
    double proficientRatio = 0;			// 精通转化率：精通 → 元素增伤
    double almightyRatio = 0;			// 全能转化率：全能 → 全能增伤
    double castingSpeedRatio = 0;		// 施法速度转化率：急速 → 施法速度
    double attackSpeedRatio = 0;		// 攻击速度转化率：急速 → 攻击速度
	double primaryAttributeRatio = 0;	// 主属性转化率：主属性 → 攻击力
	double coolDownReduce = 0;			// 冷却缩减
	double proficientAmplification = 0;	// 精通增效
	
public:
	friend class Initializer;
	friend class AutoAttack;
	
	// ==== 基本属性（游戏数值） ====
    double max_energy = 125;                 // 最大能量值
    double present_energy = 0;               // 当前能量值
    double Critical = 0;                     // 暴击率 (0-1)
    double Lucky = 0;                        // 幸运率 (0-1)
    double Almighty = 0;                     // 全能值 (0-1)
    double Proficient = 0;                   // 精通值 (0-1)
    double Quickness = 0;                    // 急速值 (0-1)
    double CriticalCount = 0;                // 暴击属性点数
    double LuckyCount = 0;                   // 幸运属性点数
    double AlmightyCount = 0;                // 全能属性点数
    double ProficientCount = 0;              // 精通属性点数
    double QuicknessCount = 0;               // 急速属性点数
	double CriticalExtraPercent = 0;         // 额外暴击百分比（来自buff等）
	double LuckyExtraPersent = 0;            // 额外幸运百分比
	double QuicknessExtraPersent = 0;        // 额外急速百分比
	double ProficientExtraPersent = 0;       // 额外精通百分比
	double AlmightyExtraPersent = 0;         // 额外全能百分比
    
    // ==== 攻击属性 ====
	double baseATK = 0;                      // 基础攻击力（不包含属性转化）
    double ATK = 0;                          // 总攻击力
    double refineATK = 0;                    // 精炼攻击力
    double elementATK = 0;                   // 元素攻击力
    double primaryAttributes = 0;            // 主属性数值
    int resourceNum = 0;                     // 当前玄冰资源数量
    int maxResourceNum = 0;                  // 最大玄冰资源数量
	double baseCastingSpeed = 0;			 // 基础施法速度（由急速转化部分）
    double castingSpeed = 0;                 // 施法速度加成 (0-1)
	double castingSpeedExtra = 0;            // 额外施法速度（来自buff）
	double baseAttackSpeed = 0;			 	 // 基础攻击速度（由急速转化部分）
    double attackSpeed = 0;                  // 攻击速度加成 (0-1)
	double attackSpeedExtra = 0;             // 额外攻击速度（来自buff）
    
    // ==== 伤害乘区（独立乘法计算） ====
    double primaryAttributesIncrease = 0;    // 主属性增加百分比 (1 + 百分比)
    double attackIncrease = 0;               // 攻击力增加百分比 (1 + 百分比)
    double damageIncrease = 0;               // 通用增伤百分比 (1 + 百分比)
    double elementIncrease = 0;              // 元素增伤百分比 (1 + 百分比)
    double almightyIncrease = 0;             // 全能增伤百分比 (1 + 百分比)
    double criticicalDamage = 0.5;           // 暴击伤害加成（基础+50%）
    double vulnerable = 0;                   // 易伤效果（目标受到的伤害增加）
    double damageReduce = 0;                 // 减伤百分比（1 - 百分比）
    double finalIncrease = 0;                // 最终伤害增加百分比 (1 + 百分比)
    double dreamIncrease = 0;                // 梦境增伤百分比 (1 + 百分比)
	double criticaldamage_set = 0;           // 额外暴击伤害（调试用）
	double increase_set = 0;                 // 额外增伤（调试用）
	double elementincrease_set = 0;          // 额外元素增伤（调试用）

	// ==== 幸运相关 ====
    double luckyDamageIncrease = 0;          // 幸运伤害增加百分比 (1 + 百分比)
    double luckyMultiplying = 0;             // 幸运伤害倍率（基础41.25%，每1%幸运+0.75%）

	// ==== 能量系统 ====
    double energyAddIncrease = 0;            // 能量回复增加
    double energyReduceUP = 0;               // 能量消耗增加（上行）
    double energyReduceDOWN = 0;             // 能量消耗减少（下行）

	std::unordered_map<std::string, DamageStatistics> damageStatsMap{};  // 伤害统计映射表（按技能名）
	std::unique_ptr<AutoAttack> autoAttackPtr = nullptr;                 // 自动攻击系统

	// ==== 构造函数与析构函数 ====
	virtual ~Person() = 0;
	
	/**
	 * @brief 构造函数，初始化角色基础属性
	 * 
	 * @param PrimaryPrimaryAttributes 主属性数值
	 * @param critical 暴击率（百分比，如36表示36%）
	 * @param quickness 急速值（百分比）
	 * @param lucky 幸运值（百分比）
	 * @param Proficient 精通值（百分比）
	 * @param almighty 全能值（百分比）
	 * @param atk 攻击力
	 * @param refineatk 精炼攻击力
	 * @param elementatk 元素攻击力
	 * @param attackSpeed 攻击速度加成（0-1）
	 * @param castingSpeed 施法速度加成（0-1）
	 * @param critialdamage_set 额外爆伤（调试用）
	 * @param increasedamage_set 额外增伤（调试用）
	 * @param elementdamage_set 额外元素增伤（调试用）
	 * @param totalTime 模拟总时间（毫秒）
	 */
	Person(const double PrimaryPrimaryAttributes, const double critical, const double quickness, const double lucky, const double Proficient, const double almighty,
        const int atk, const  int refineatk, const  int elementatk, const double attackSpeed, const double castingSpeed,
        const double critialdamage_set, const double increasedamage_set, const double elementdamage_set, const  int totalTime);
	
	Person();
	// ==== 属性操作方法（可被子类重写） ====
	
	/**
	 * @brief 增加属性点数并重新计算属性值
	 * @param addCount 增加的属性点数
	 * @return 更新后的属性百分比
	 */
	virtual double changeCriticalCount(int addCount);	
	virtual double changeQuicknessCount(int addCount);		
	virtual double changeLuckyCount(int addCount);				
	virtual double changeProficientCount(int addCount);		
	virtual double changeAlmightyCount(int addCount);
	
	/**
	 * @brief 增加属性百分比（直接修改百分比）
	 * @param persent 增加的百分比（如0.05表示+5%）
	 * @return 更新后的属性百分比
	 */
	virtual double changeCritialPersent(double persent);
	virtual double changeQuicknessPersent(double persent);
	virtual double changeLuckyPersent(double persent);
	virtual double changeProficientPersent(double persent);
	virtual double changeAlmightyPersent(double persent);

	// ==== 核心战斗接口 ====

	/**
	 * @brief 计算技能伤害
	 * @param skill 要计算的技能指针
	 * @return DamageInfo 包含伤害结果的结构体
	 * 
	 * 伤害公式：
	 * [(ATK × 倍率 × 攻击增幅 × 减伤区) + (精炼攻击+元素攻击) × 倍率 + 固定值]
	 * × 增伤 × 元素伤害 × 全能增幅 × 梦境增幅 × 技能最终伤害
	 * × 暴击判定(1+暴伤)
	 */
	DamageInfo Damage(const Skill *skill);
	
	/**
	 * @brief 计算幸运伤害（独立伤害机制）
	 * @return 幸运伤害值
	 */
	double luckyDamage() const;

	// ==== 属性值设置 ====
	
	/**
	 * @brief 获取属性百分比对应的属性点数
	 * @param critical 暴击率（0-1）
	 * @return 对应的属性点数
	 */
	int getCriticalCount(double critical);				
	int getQuicknessCount(double quickness);				
	int getLuckyCount(double lucky);				
	int getProficientCount(double proficient);		
	int getAlmightyCount(double almighty);				

	// ==== 速度加成 ====
	void addCastingSpeed(double persent);  // 增加施法速度百分比
	void addAttackSpeed(double persent);   // 增加攻击速度百分比

	// ==== 乘区初始化与修改 ====
	void initializeIncrease();					  	  				// 初始化乘区
	virtual double changePrimaryAttributesByCount(double PrimaryAttributesCount);					// 通过属性点数修改主属性
	virtual double changePrimaryAttributesByPersent(double PrimaryAttributesPersent);				// 通过百分比修改主属性
	virtual double setATK(double atk);                        						// 设置基础攻击力
	virtual double resetATK();								  						// 重新计算攻击力（基于主属性）
	virtual double changeATKCount(double count);			  		// 修改攻击力数值
	virtual double changeRefineATKCount(double n);				  	// 修改精炼攻击

	virtual double setAattackIncrease();													// 初始化攻击增加乘区
	virtual double changeAattackIncrease(double attackIncrease); 					// 修改攻击增加乘区

	virtual double setElementIncrease();													// 初始化元素增伤乘区
	virtual double changeElementIncreaseByProficient(double proficient);			// 通过精通修改元素增伤
	virtual double changeElementIncreaseByElementIncrease(double elementIncrease); 	// 直接修改元素增伤

	virtual double setAlmightyIncrease();													// 初始化全能增伤乘区
	virtual double changeAlmightyIncrease(double almighty); 						// 修改全能增伤乘区

	virtual double setDamageIncrease();					   									// 初始化通用增伤乘区
	virtual double changeDamageIncrease(double increase); 							// 修改通用增伤乘区

	virtual double setCriticalDamage();						   								// 初始化暴击伤害乘区
	virtual double changeCriticalDamage(double criticalDamage); 					// 修改暴击伤害乘区

	virtual double setLuckyMultiplying();								   					// 初始化幸运倍率
	virtual double changeLuckyMultiplyingByAddMultiplying(double addMultiplying); 	// 修改幸运倍率

	virtual double setDreamIncrease();						   								// 初始化梦境增伤乘区
	virtual double changeDreamIncrease(double dreamIncrease); 						// 修改梦境增伤乘区

	virtual double changeCastingSpeeaByPersent(const double castingSpeedPersent);			// 修改施法速度
	virtual double changeAttackSpeeaByPersent(const double attackSpeedPersent);		// 修改攻击速度

	virtual double chanageDamageReduce(const double n);								// 修改减伤区

	// ==== 技能管理 ====
	void createSkill(std::unique_ptr<Skill> newSkill);				// 创建并开始释放一个技能
	void createNoReleasingSkill(std::unique_ptr<Skill> newSkill);	// 创建无前摇技能

	void updateSkills(int deltaTime);								// 更新所有技能状态
	void updateNowReleasingSkill(int deltaTime);					// 更新当前释放技能
	void updateContinuousList(int deltaTime);						// 更新持续性技能列表
	void updateSkillsCD(int deltaTime);								// 更新技能冷却
	void processCooling(Skill *skill, int deltaTime);				// 处理技能冷却逻辑
	void processChargedCooling(Skill *skill, int deltaTime);		// 处理充能冷却逻辑
	bool removeSkill(Skill *skill);									// 移除指定技能
	void clearSkills();												// 清空所有技能
	void cleanupFinishedSkills();									// 清理已结束的技能
	void updateAction(int deltaTime);								// 处理动作队列

	// ==== Buff管理 ====
	void createBuff(std::unique_ptr<Buff> newBuff);					// 创建一个buff
	void updateBuffs(int deltaTime);								// 更新所有buff状态
	void cleanupFinishedBuffs();									// 清理已结束的buff
	void clearBuffs();												// 清空所有buff

	// ==== 事件系统 ====
	template <typename T, typename... Args>
	void triggerAction(double count, Args &&...args);				// 触发一个动作事件

	// ==== 资源管理 ====
	bool consumeResource(int n); 		// 消耗玄冰资源
	bool revertResource(int n);			// 回复玄冰资源
	bool consumeEnergy(const double n); 		// 消耗能量
	bool revertEnergy(const double n);  		// 回复能量
	bool reduceSkillCD(std::string skillName,double n); 	// 减少技能冷却时间

	// ==== 查找功能 ====
	template <class T>
	int findBuffInBuffList(); 										// 通过模板类名查找buff索引
	int findBuffInBuffList(std::string buffName) const;				// 通过buff名查找buff索引
	template <class T>
	int findSkillInSkillCDList(); 									// 通过模板类名查找技能索引
	int findSkillInSkillCDList(std::string skillName) const;		// 通过技能名查找技能索引
	void addErrorInfoInList(const ErrorInfo& errorinfo);			// 添加错误信息到列表

	// ==== 辅助功能 ====
	bool isSuccess(double probability) const; 							// 概率判定
	auto calculateDamageStatistics() -> decltype(this->damageStatsMap); // 计算伤害统计数据							
	void equipSkill(std::string skillName);								// 装备技能到CD列表
	void equipInherentBuff(std::string buffName);						// 装备固有buff
	void pushDamgeInfo(DamageInfo& info);                               // 记录伤害信息
	void setRandomSeed(std::uint32_t seed);								// 设置随机数种子

	// ==== Getter接口（提供只读访问） ====
    double getAttributeRatio() const;
    int getPropertyTransformationCoeffcient_General() const;
    int getPropertyTransformationCoeffcient_Almighty() const;
    
    const std::vector<std::unique_ptr<Buff>>& getBuffListRef() const;
    const std::vector<std::unique_ptr<Skill>>& getContinuousSkillListRef() const;
    const std::vector<std::unique_ptr<Skill>>& getNoReleasingSkillListRef() const;
    const std::vector<std::unique_ptr<Skill>>& getSkillCDListRef() const;
    const std::queue<ActionInfo>& getActionQueueRef() const;
    const std::vector<std::string>& getSkillListInfoRef() const;
    const std::vector<DamageInfo>& getDamageListInfoRef() const;
    const std::vector<ErrorInfo>& getErrorInfoListRef() const;
    
    int getTotalTime() const;
    double getMaxEnergy() const;
    double getPresentEnergy() const;
    double getCritical() const;
    double getLucky() const;
    double getAlmighty() const;
    double getProficient() const;
    double getQuickness() const;
    double getCriticalCount() const;
    double getLuckyCount() const;
    double getAlmightyCount() const;
    double getProficientCount() const;
    double getQuicknessCount() const;
    
    double getATK() const;
    double getRefineATK() const;
    double getElementATK() const;
    double getPrimaryAttributes() const;
    int getResourceNum() const;
    int getMaxResourceNum() const;
    double getCastingSpeed() const;
    double getAttackSpeed() const;
    
    double getPrimaryAttributesIncrease() const;
    double getAttackIncrease() const;
    double getDamageIncrease() const;
    double getElementIncrease() const;
    double getAlmightyIncrease() const;
    double getCriticicalDamage() const;
    double getVulnerable() const;
    double getDamageReduce() const;
    double getFinalIncrease() const;
    double getDreamIncrease() const;
    
    double getLuckyDamageIncrease() const;
    double getLuckyMultiplying() const;
    
    double getProficientRatio() const;
    double getAlmightyRatio() const;
    double getCastingSpeedRatio() const;
    double getAttackSpeedRatio() const;
	double getProficientAmplification() const;
    
    bool getIsReleasingSkill() const;
    
    double getEnergyAddIncrease() const;
    double getEnergyReduceUP() const;
    double getEnergyReduceDOWN() const;
    
    const AutoAttack* getAutoAttack() const;
    Skill* const getNowReleasingSkill() const;

	const Skill* getCurtainPointerForAction(std::string skillName) const;
};

// ============================================================================
// 模板函数实现
// ============================================================================

/**
 * @brief 触发一个动作事件
 * @tparam T 动作类型
 * @tparam Args 动作构造函数参数类型
 * @param count 动作执行次数或参数
 * @param args 动作构造函数参数
 * 
 * 将动作添加到动作队列，由updateAction处理
 */
template <typename T, typename... Args>
void Person::triggerAction(double count, Args &&...args)
{
	auto it = std::make_unique<T>(std::forward<Args>(args)...);
	if(it)
	{
		it->execute(count,this);

		// 不建议再使用队列处理事件
		// 队列处理事件会导致模拟DPS偏低
		// 以及难以实现createSkill监听器回调函数对于技能属性的修改
		// 若要使用需要修改attackAction逻辑使其接受skillName而非Skill*
		// 并且取消autoAttack::updatePerson()中更新action的注释
		// this->actionQueue.push(ActionInfo(count,std::move(it)));
	}
}

/**
 * @brief 通过模板类名查找buff索引
 * @tparam T buff类类型
 * @return buff索引，-1表示未找到
 */
template <class T>
int Person::findBuffInBuffList()
{
	for (size_t i = 0; i < this->buffList.size(); i++)
	{
		if (buffList.at(i)->getBuffName() == T::name)
		{
			return static_cast<int>(i);
		}
	}
	return -1;
}

/**
 * @brief 通过模板类名查找技能索引
 * @tparam T 技能类类型
 * @return 技能索引，-1表示未找到
 */
template <class T>
int Person::findSkillInSkillCDList()
{
	for (int i = 0; i < this->skillCDList.size(); i++)
	{
		if (skillCDList.at(i)->getSkillName() == T::getSkillName())
		{
			return i;
		}
	}
	return -1;
}


// 临时的person类，用于初始化buff判断buff是否为固定
class temp_Person : public Person
{   
public:
    temp_Person() : Person(){};
    ~temp_Person(){};
};