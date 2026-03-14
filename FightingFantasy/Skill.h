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