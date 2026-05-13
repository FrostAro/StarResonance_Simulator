#pragma once
#include "../../core/Skill.h"

// 射线部分
// 射线
class Beam : public FacilitationSkill
{
public:
	static std::string name;
	int addMultiplyingTimes = 0;
	double baseMultiplying = 0;
	double baseFixedValue = 0;
	int noEnergyConsumeTime = 0;
	int count = 0;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;
	bool canEndFacilitation(Person *p) override;

	explicit Beam(Person *p);
};

// 涡流
class Vortex : public ContinuousSkill
{
public:
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;

	explicit Vortex(Person *p);
};

// 水龙卷
// 龙卷默认在释放射线后释放，不实现单独释放逻辑
class WaterSpout : public ContinuousSkill
{
public:
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;

	explicit WaterSpout(Person *p);
};

// 冻结寒风
class FrostWind : public InstantSkill
{
public:
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;

	explicit FrostWind(Person *p);
};

class Flood_Beam : public InstantSkill
{
public:
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;

	explicit Flood_Beam(Person *p);
};

// 冰晶坠落
class CrystalsHail : public InstantSkill
{
public:
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;

	explicit CrystalsHail(Person *p);
};

// 冰令脉冲
class FrostDecreePulse : public InstantSkill
{
public:
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;

	explicit FrostDecreePulse(Person *p);
};

class IceArrow_Beam : public InstantSkill
{ // 冰箭
public:
	
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;

	explicit IceArrow_Beam(Person *p);
};

// 霜爆（冰箭爆炸）
class FrostBurst : public InstantSkill
{
public:
	
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;

	explicit FrostBurst(Person *p);
};

class Ultimate_Beam : public InstantSkill
{ // 大招
public:
	
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;

	explicit Ultimate_Beam(Person *p);
};

class FantasyImpact_Beam : public InstantSkill
{ // 幻想冲击
public:
	
	static std::string name;

	void setSkillType() override;
	void trigger(Person *p) override;
	std::string getSkillName() const override;

	explicit FantasyImpact_Beam(Person *p);
};
