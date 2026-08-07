#pragma once
#include "../../core/AutoAttack.h"
#include <string>

// ============================================================================
// 数据驱动的冰矛自动爆发基类
//
// 三个幻想变体（MukuScout / BYS / YGLWS）的自动战斗逻辑完全相同，
// 唯一差异是大爆发中使用的幻想技能名（出现在大爆发判定与大爆发技能序列）。
// 因此把该技能名参数化，消除三份逐行复制的实现。
// ============================================================================
class AutoAttack_Mage_Icicle_Base : public AutoAttack
{
protected:
    explicit AutoAttack_Mage_Icicle_Base(Person* p, std::string fantasySkillName);

    void largeOutBurst() override;
    void smallOutBurst() override;
    void checkAndTriggerLargeOutBurst() override;
    void checkAndTriggerSmallOutBurst() override;
    void checkAndFinishOutBurst() override;
    void checkAndSetOutBurstState() override;
    void windowPeriodLogic() override;
    void update(int deltaTime) override;

    std::string m_fantasySkillName;   // 大爆发使用的幻想技能名

public:
    ~AutoAttack_Mage_Icicle_Base() override = default;
};

// 具体幻想变体：只需把幻想技能名传给基类
class AutoAttack_Mage_Icicle_MukuScout : public AutoAttack_Mage_Icicle_Base
{
public:
    explicit AutoAttack_Mage_Icicle_MukuScout(Person* p);
};

class AutoAttack_Mage_Icicle_BYS : public AutoAttack_Mage_Icicle_Base
{
public:
    explicit AutoAttack_Mage_Icicle_BYS(Person* p);
};

class AutoAttack_Mage_Icicle_YGLWS : public AutoAttack_Mage_Icicle_Base
{
public:
    explicit AutoAttack_Mage_Icicle_YGLWS(Person* p);
};
