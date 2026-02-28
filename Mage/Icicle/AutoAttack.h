#pragma once
#include "../../core/AutoAttack.h"
#include "Skill.h"

class AutoAttack_Mage_Icicle_MukuScout : public AutoAttack
{
    void largeOutBurst() override;                   
    void smallOutBurst() override;
    void checkAndTriggerLargeOutBurst() override;                
    void checkAndTriggerSmallOutBurst() override;
    void checkAndFinishOutBurst() override;                      
    void checkAndSetOutBurstState() override;
    void windowPeriodLogic() override;
    void update(int deltaTime) override;                             

public:
    AutoAttack_Mage_Icicle_MukuScout(Person* p);
};

class AutoAttack_Mage_Icicle_BYS : public AutoAttack
{
    void largeOutBurst() override;                   
    void smallOutBurst() override;
    void checkAndTriggerLargeOutBurst() override;                
    void checkAndTriggerSmallOutBurst() override;
    void checkAndFinishOutBurst() override;                      
    void checkAndSetOutBurstState() override;
    void windowPeriodLogic() override;
    void update(int deltaTime) override;                             

public:
    AutoAttack_Mage_Icicle_BYS(Person* p);
};

class AutoAttack_Mage_Icicle_YGLWS : public AutoAttack
{
    void largeOutBurst() override;                   
    void smallOutBurst() override;
    void checkAndTriggerLargeOutBurst() override;                
    void checkAndTriggerSmallOutBurst() override;
    void checkAndFinishOutBurst() override;                      
    void checkAndSetOutBurstState() override;
    void windowPeriodLogic() override;
    void update(int deltaTime) override;                             

public:
    AutoAttack_Mage_Icicle_YGLWS(Person* p);
};