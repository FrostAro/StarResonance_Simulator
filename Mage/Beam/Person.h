#pragma once
#include "../../core/Person.h"

class Mage_Beam : public Person
{
private:
    double proficientToEnergyRatio = 0.4;             // 精通转额外回能率
public:
    friend class Initializer;
    friend class EnergyRevertAction_Beam;

	Mage_Beam(const double primaryAttributes, const double critical, const double quickness, const double lucky, const double Proficient, const double almighty,
        const int atk, const int refindatk, const int elementatk, const  double attackSpeed, const double castingSpeed,
        const  double critialdamage_set, const double increasedamage_set, const double elementdamage_set, const int totalTime,
        const int fantasyConfig = 0);
};