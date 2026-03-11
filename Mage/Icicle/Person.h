#pragma once
#include "../../core/Person.h"

class Mage_Icicle : public Person
{
public:
    friend class Initializer;

	Mage_Icicle(const double primaryAttributes, const double critical, const double quickness, const double lucky, const double Proficient, const double almighty,
        const int atk, const int refindatk, const int elementatk, const  double attackSpeed, const double castingSpeed,
        const  double critialdamage_set, const double increasedamage_set, const double elementdamage_set, const int totalTime,
        const int fantasyConfig = 0);

	double changeLuckyPersent(double persent) override;
	double changeLuckyCount(int addCount) override;
};