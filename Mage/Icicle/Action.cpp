#include "Action.h"
#include "../../core/GameConstants.h"
#include "../../core/Person.h"
#include "../../core/Logger.h"
#include "Person.h"

// 幸运数值
std::string LuckyCountModifyAction_Icicle::name = "LuckyCountModifyAction_Icicle";

LuckyCountModifyAction_Icicle::LuckyCountModifyAction_Icicle()
    : LuckyCountModifyAction() {}

void LuckyCountModifyAction_Icicle::execute(double n, Person *p)
{
    // 幸运基础百分比为5%
    LuckyCountModifyAction::execute(n,p);
    p->changeLuckyMultiplyingByAddMultiplying(0.15 + p->Lucky / 2);
    Logger::debugAction(AutoAttack::getTimer(), this->getActionName(),
                        "triggered, lucky multiplier adjusted, value: " + std::to_string(n));
}

std::string LuckyCountModifyAction_Icicle::getActionName() { return LuckyCountModifyAction_Icicle::name; }


// 幸运百分比
std::string LuckyPercentModifyAction_Icicle::name = "LuckyPercentModifyAction_Icicle";

LuckyPercentModifyAction_Icicle::LuckyPercentModifyAction_Icicle()
    : LuckyPercentModifyAction() {}

void LuckyPercentModifyAction_Icicle::execute(double n, Person *p)
{
    LuckyPercentModifyAction::execute(n,p);
    p->changeLuckyMultiplyingByAddMultiplying(0.15 + (p->Lucky - kBaseLuckyPercent) / 2);
    Logger::debugAction(AutoAttack::getTimer(), this->getActionName(),
                        "triggered, lucky multiplier adjusted, value: " + std::to_string(n));
}

std::string LuckyPercentModifyAction_Icicle::getActionName() { return LuckyPercentModifyAction_Icicle::name; }
