#include "Person.h"
#include "AutoAttack.h"
#include "../../core/Action.h"
#include "Action.h"

Mage_Icicle::Mage_Icicle(const double PrimaryAttributes, const double critical, const double quickness, const double lucky, const double Proficient, const double almighty,
                         const int atk, const int refindatk, const int elementatk, const double attackSpeed, const double castingSpeed,
                         const double critialdamage_set, const double increasedamage_set, const double elementdamage_set, const int totalTime,
                         const int fantasyConfig)
    : Person(PrimaryAttributes, critical, quickness, lucky, Proficient, almighty,
             atk, refindatk, elementatk, attackSpeed, castingSpeed,
             critialdamage_set, increasedamage_set, elementdamage_set, totalTime)
{
    // 覆盖基类中的默认值
    this->primaryAttributeRatio = 0.6;
    this->proficientRatio = 0.65;
    this->almightyRatio = 0.35;
    this->maxResourceNum = 4;
    this->resourceNum = this->maxResourceNum;
    this->castingSpeedRatio = 2;
    this->attackSpeedRatio = 0.2;
    this->damageReduce = 0.1;
    this->coolDownReduce = 0.1;
    this->max_energy = 125;

        // 根据幻想配置选择对应的自动战斗逻辑
    switch (fantasyConfig) {
        case 0:
            this->autoAttackPtr = std::make_unique<AutoAttack_Mage_Icicle_MukuScout>(this);
            break;
        case 1:
            this->autoAttackPtr = std::make_unique<AutoAttack_Mage_Icicle_BYS>(this);
            break;
        case 2:
            this->autoAttackPtr = std::make_unique<AutoAttack_Mage_Icicle_YGLWS>(this);
            break;
        default:
            this->autoAttackPtr = std::make_unique<AutoAttack_Mage_Icicle_MukuScout>(this);
            break;
    }

    setATK(atk);
    resetATK();

    // 冰矛天赋的幸运倍率加成
    changeLuckyMultiplyingByAddMultiplying(0.15 + (this->Lucky - 0.05) / 2);

    // 因子效果:智力
    // triggerAction<PrimaryAttributesCountModifyAction>(70);
    // triggerAction<PrimaryAttributesPercentModifyAction>(0.0184);
    // 因子效果：暴击，幸运
    // triggerAction<CriticalCountModifyAction>(static_cast<int>(this->CriticalCount * 0.1));
    // triggerAction<LuckyCountModifyAction_Icicle>(static_cast<int>(this->LuckyCount * 0.0927));
    
    initializeIncrease();
    changeDamageIncrease(0.08);
    changeElementIncreaseByElementIncrease(0.1);
}

double Mage_Icicle::changeLuckyCount(int addCount)
{

    Person::changeLuckyCount(addCount);
    // changeLuckyCout中重置了幸运倍率，所以这里需要将冰矛特有倍率加回来
    changeLuckyMultiplyingByAddMultiplying(0.15 + this->Lucky / 2);
    return this->Lucky;
}

double Mage_Icicle::changeLuckyPersent(double persent)
{
    Person::changeLuckyPersent(persent);
    changeLuckyMultiplyingByAddMultiplying(0.15 + this->Lucky / 2);
    return this->Lucky;
}
