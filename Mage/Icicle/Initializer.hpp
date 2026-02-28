#pragma once
#include "../../core/Initializer.hpp"
#include "../../FightingFantasy/Buff.h"
#include "../../FightingFantasy/Skill.h"
#include "Buff.h"
#include "Skill.h"

class Initializer_Mage_Icicle : public Initializer
{
    void equipSkills() override
    {   
        //例：配置技能:冰矛
        equipCertainSkill(Spear::name);
        equipCertainSkill(Meteorite::name);
        equipCertainSkill(WaterDrop::name);
        equipCertainSkill(Flood_Icicle::name);
        equipCertainSkill(Ultimate_Icicle::name);
        equipCertainSkill(MukuChief::name);
        //equipCertainSkill(MukuScout::name);
        //equipCertainSkill(BYS::name);
        equipCertainSkill(YGLWS::name);
    }

    void registerSkills() override
    {
        registerCertainSkill<IceArrow_Icicle>();
        registerCertainSkill<Flood_Icicle>();
        registerCertainSkill<Ultimate_Icicle>();
        registerCertainSkill<Spear>();
        registerCertainSkill<PierceSpear>();
        registerCertainSkill<Meteorite>();
        registerCertainSkill<SynergyMeteorite>();
        registerCertainSkill<WaterDrop>();
        registerCertainSkill<FrostComet>();
        registerCertainSkill<FantasyImpact>();
        registerCertainSkill<MukuChief>();
        registerCertainSkill<MukuScout>();
        registerCertainSkill<BYS>();
        registerCertainSkill<YGLWS>();
    }

    void registerBuffs() override
    {
        registerCertainBuff<SpearCritialBuff>();
        registerCertainBuff<SpearCritialToRevertIceBuff>();
        registerCertainBuff<IceCountBuff>();
        registerCertainBuff<DoubleSpearBuff>();
        registerCertainBuff<FloodBuff_Icicle>();
        registerCertainBuff<IceRevertBuff>();
        registerCertainBuff<ConsumedEnergyCountBuff>();
        registerCertainBuff<EndlessColdBuff>();
        registerCertainBuff<UltiIncreaseBuff_Icicle>();
        registerCertainBuff<MeteoriteRefreshBuff>();
        registerCertainBuff<FrostCometBuff>();
        registerCertainBuff<MeteoriteSynergyBuff>();
        registerCertainBuff<MukuChiefBuff>();
        registerCertainBuff<MukuScoutBuff>();
        registerCertainBuff<BYSBuff>();
        registerCertainBuff<YGLWSBuff>();
        registerCertainBuff<SimulateNormalAttackToRevertIceBuff>();
        registerCertainBuff<PierceSpearBuff>();
        registerCertainBuff<EquipmentSetEffectBuff_Icicle>();
        registerCertainBuff<FantasyImpactBuff>();
        registerCertainBuff<ExtremeLuckDivisor>();
        registerCertainBuff<OccupationalDivisor_Icicle>();
        registerCertainBuff<InstantCooldownBuff_Icicle>();
        registerCertainBuff<FloatingExtraSecondaryAttributesBuff_Icicle>();
    }

public:
    Initializer_Mage_Icicle(Person* p, double deltaTime) : Initializer(p,deltaTime){}
};
