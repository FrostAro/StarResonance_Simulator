#pragma once
#include "../core/Skill.h"

class MukuChief : public FightingFantasy, public InstantSkill
{ // 姆克头目
public:
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;
	void setPassiveEffect(Person *p) override;
    void removePassiveEffect(Person *p) override;

	explicit MukuChief(Person *p);
};

class MukuScout : public FightingFantasy, public InstantSkill
{ // 姆克尖兵
public:
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;
	void setPassiveEffect(Person *p) override;
    void removePassiveEffect(Person *p) override;

	explicit MukuScout(Person *p);
};

class BYS : public FightingFantasy, public InstantSkill
{ // 博伊斯
public:
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;
	void setPassiveEffect(Person *p) override;
    void removePassiveEffect(Person *p) override;

	explicit BYS(Person *p);
};

class YGLWS : public FightingFantasy, public InstantSkill
{ // 伊戈雷乌斯
public:
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;
	void setPassiveEffect(Person *p) override;
    void removePassiveEffect(Person *p) override;

	explicit YGLWS(Person *p);
};

class SXMQ : public FightingFantasy, public InstantSkill
{ // 嗜血毛球
public:
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;
	void setPassiveEffect(Person *p) override;
    void removePassiveEffect(Person *p) override;

	explicit SXMQ(Person *p);
};

class HYXZ : public FightingFantasy, public InstantSkill
{ // 幻妖蟹蛛
public:
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;
	void setPassiveEffect(Person *p) override;
    void removePassiveEffect(Person *p) override;

	explicit HYXZ(Person *p);
};

class LSZZ : public FightingFantasy, public InstantSkill
{ // 掠食蜘蛛
public:
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;
	void setPassiveEffect(Person *p) override;
    void removePassiveEffect(Person *p) override;

	explicit LSZZ(Person *p);
};

class YZ : public FightingFantasy, public InstantSkill
{ // 游子
public:
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;
	void setPassiveEffect(Person *p) override;
    void removePassiveEffect(Person *p) override;

	explicit YZ(Person *p);
};

class JQR : public FightingFantasy, public InstantSkill
{ // 机器人
public:
	static std::string name;

	/// 机器人被动：精通 +15%。
	/// 游戏中该加成会直接计入站街面板，因此玩家输入的面板值已经包含它；
	/// 建模时需通过 Person::applyFantasyPassiveBonus 把它重新归类到
	/// 「额外百分比通道」，否则会被当作点数放大、并被双倍精通翻倍。
	static constexpr double kPassiveProficient = 0.15;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;
	void setPassiveEffect(Person *p) override;
    void removePassiveEffect(Person *p) override;

	explicit JQR(Person *p);

private:
	bool passiveApplied = false;   // setPassiveEffect 幂等保护
};