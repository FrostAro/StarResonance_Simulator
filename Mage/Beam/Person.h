#pragma once
#include "../../core/Person.h"

class Mage_Beam : public Person
{
private:
    double proficientToEnergyRatio = 0.4;             // 精通转额外回能率
    double proficientEnhanceRatio = 0.03;              // 精通增效
public:
    friend class Initializer;
    friend class EnergyRevertAction_Beam;

	Mage_Beam(const double primaryAttributes, const double critical, const double quickness, const double lucky, const double Proficient, const double almighty,
        const int atk, const int refindatk, const int elementatk, const  double attackSpeed, const double castingSpeed,
        const  double critialdamage_set, const double increasedamage_set, const double elementdamage_set, const int totalTime,
        const int fantasyConfig = 0);

    double changeElementIncreaseByProficient(double proficient) override;			// 通过精通修改元素增伤
    double setElementIncrease() override;											// 初始化元素增伤乘区
};  