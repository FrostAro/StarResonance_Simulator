#include "Person.h"
#include "AutoAttack.h"
#include "../../core/Action.h"
#include "Action.h"

// 射线部分
Mage_Beam::Mage_Beam(const double PrimaryAttributes, const double critical, const double quickness, const double lucky, const double Proficient, const double almighty,
                         const int atk, const int refindatk, const int elementatk, const double attackSpeed, const double castingSpeed,
                         const double critialdamage_set, const double increasedamage_set, const double elementdamage_set, const int totalTime,
                         const int fantasyConfig)
    : Person(PrimaryAttributes, critical, quickness, lucky, Proficient, almighty,
             atk, refindatk, elementatk, attackSpeed, castingSpeed,
             critialdamage_set, increasedamage_set, elementdamage_set, totalTime)
{
    // 覆盖基类中的默认值
    this->primaryAttributeRatio = 0.6;
    this->proficientRatio = 0.4;
    this->almightyRatio = 0.35;
    this->maxResourceNum = 3;
    this->castingSpeedRatio = 2;
    this->attackSpeedRatio = 0.2;
    this->damageReduce = 0.1;
    this->coolDownReduce = 0.1;
    this->max_energy = 165;
    this->present_energy = max_energy;
    this->proficientToEnergyRatio = 1;

    // 根据幻想配置选择对应的自动战斗逻辑
    switch (fantasyConfig) {
        case 0:
            this->autoAttackPtr = std::make_unique<AutoAttack_Mage_Beam_MukuScout>(this);
            break;
        case 1:
            this->autoAttackPtr = std::make_unique<AutoAttack_Mage_Beam_YGLWS>(this);
            break;
        case 2:
            this->autoAttackPtr = std::make_unique<AutoAttack_Mage_Beam_SXMQ>(this);
            break;
        default:
            this->autoAttackPtr = std::make_unique<AutoAttack_Mage_Beam_MukuScout>(this);
            break;
    }

    setATK(atk);
    resetATK();

    // 奇妙bug：因子效果不可删
    // 因子效果:智力
    //triggerAction<PrimaryAttributesCountModifyAction>(1);
    //triggerAction<PrimaryAttributesPercentModifyAction>(0.0000000184);
    // 因子效果：暴击，全能
    // triggerAction<CriticalCountModifyAction>(static_cast<int>(this->CriticalCount * 0.0000001));
    // triggerAction<AlmightyCountModifyAction>(static_cast<int>(this->AlmightyCount * 0.0000001));
    this->triggerAction<AttackIncreaseModifyAction>(0.0000001);
    initializeIncrease();
    changeDamageIncrease(0.08);
    changeElementIncreaseByElementIncrease(0.1);
}