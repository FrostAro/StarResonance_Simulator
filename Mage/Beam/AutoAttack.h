#pragma once
#include "../../core/AutoAttack.h"
#include <vector>
#include <string>
#include <utility>

class Person;
class Beam;

// 阶段配置：检查的技能列表 + 要添加的技能列表
struct BurstStage {
    // 每个技能：名称，是否允许用堆叠替代CD（true表示允许stack>0也算就绪）
    std::vector<std::pair<std::string, bool>> checkSkills;
    std::vector<std::string> addSkills;
};

// 所有 Beam 法系自动爆发的基类
class AutoAttack_Mage_Beam_Base : public AutoAttack {
public:
    AutoAttack_Mage_Beam_Base(Person* p, const std::vector<BurstStage>& stages);
    ~AutoAttack_Mage_Beam_Base() override = default;

    void update(int deltaTime) override final;

protected:
    void windowPeriodLogic() override;
    void checkAndFinishOutBurst() override;

private:
    void tryTriggerStage(int stageIdx);
    bool checkSkills(const std::vector<std::pair<std::string, bool>>& skills) const;
    void addSkillsToList(const std::vector<std::string>& skills);

    std::vector<BurstStage> m_stages;
    int  nextOutBurstSignal = 1;
    bool nextIsWindow = false;
    bool windowSkillTriggered = false;   // 替代原static变量
    bool windowSecondBeamTriggered = false;
};

// -------------------- 具体派生类 --------------------
class AutoAttack_Mage_Beam_MukuScout : public AutoAttack_Mage_Beam_Base {
public:
    explicit AutoAttack_Mage_Beam_MukuScout(Person* p);
};

class AutoAttack_Mage_Beam_YGLWS : public AutoAttack_Mage_Beam_Base {
public:
    explicit AutoAttack_Mage_Beam_YGLWS(Person* p);
};

class AutoAttack_Mage_Beam_SXMQ : public AutoAttack_Mage_Beam_Base {
public:
    explicit AutoAttack_Mage_Beam_SXMQ(Person* p);
};

class AutoAttack_Mage_Beam_HYXZ : public AutoAttack_Mage_Beam_Base {
public:
    explicit AutoAttack_Mage_Beam_HYXZ(Person* p);
};

class AutoAttack_Mage_Beam_JBMQ : public AutoAttack_Mage_Beam_Base {
public:
    explicit AutoAttack_Mage_Beam_JBMQ(Person* p);
};

class AutoAttack_Mage_Beam_LSZZ : public AutoAttack_Mage_Beam_Base {
public:
    explicit AutoAttack_Mage_Beam_LSZZ(Person* p);
};

// 无幻想版本 - 仅使用基础技能，不依赖任何幻想技能
class AutoAttack_Mage_Beam_NoFantasy : public AutoAttack_Mage_Beam_Base {
public:
    explicit AutoAttack_Mage_Beam_NoFantasy(Person* p);
};