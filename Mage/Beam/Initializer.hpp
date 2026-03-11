#pragma once
#include "../../core/Initializer.hpp"
#include "../../FightingFantasy/Buff.h"
#include "../../FightingFantasy/Skill.h"
#include "Buff.h"
#include "Skill.h"

class Initializer_Mage_Beam : public Initializer
{
    void equipSkills() override
    {   
        //配置技能:射线
        equipCertainSkill(Beam::name);
        equipCertainSkill(Vortex::name);
        equipCertainSkill(WaterSpout::name);
        equipCertainSkill(FrostWind::name);
        equipCertainSkill(Flood_Beam::name);
        equipCertainSkill(Ultimate_Beam::name);
        equipCertainSkill(MukuChief::name);
        // 根据幻想配置装备第二个幻想技能
        if (m_fantasyConfig == 0) {
            equipCertainSkill(MukuScout::name);   // 姆头+尖兵
        } else if (m_fantasyConfig == 1) {
            equipCertainSkill(YGLWS::name);       // 姆头+伊戈雷乌斯
        } else {
            equipCertainSkill(SXMQ::name);        // 姆头+嗜血毛球
        }
    }

    void registerSkills() override
    {
        registerCertainSkill<IceArrow_Beam>();
        registerCertainSkill<Flood_Beam>();
        registerCertainSkill<Ultimate_Beam>();
        registerCertainSkill<Beam>();
        registerCertainSkill<Vortex>();
        registerCertainSkill<FrostWind>();
        registerCertainSkill<WaterSpout>();
        registerCertainSkill<CrystalsHail>();
        registerCertainSkill<FrostDecreePulse>();
        registerCertainSkill<FrostBurst>();
        registerCertainSkill<MukuChief>();
        registerCertainSkill<MukuScout>();
        registerCertainSkill<YGLWS>();
        registerCertainSkill<SXMQ>();
    }

    void registerBuffs() override
    {
        registerCertainBuff<BeamBuildBuff>();
        registerCertainBuff<NaturalEnergyRegenBuff>();
        registerCertainBuff<IcePromiseBuff>();
        registerCertainBuff<FrostCrystalResonanceBuff>();
        registerCertainBuff<ChillPersistenceBuff>();
        registerCertainBuff<FloodBuff_Beam>();
        registerCertainBuff<IntellectCrystalBuff>();
        registerCertainBuff<IceTideBuff>();
        registerCertainBuff<ElementIncreaseBuff_IceTide>();
        registerCertainBuff<IceInfiniteBuff>();
        registerCertainBuff<BeamMagnumOpusBuff>();
        registerCertainBuff<DoubleProficientBuff>();
        registerCertainBuff<FrostwindFocusBuff>();
        registerCertainBuff<SwiftCastBuff>();
        registerCertainBuff<FrostCrystalPowerBuff>();
        registerCertainBuff<EnergySurgeLawBuff>();
        registerCertainBuff<EnergyRevertBuff_EnergySurgeLaw>();
        registerCertainBuff<UltiIncreaseBuff_Beam>();
        registerCertainBuff<MukuChiefBuff>();
        registerCertainBuff<MukuScoutBuff>();
        registerCertainBuff<YGLWSBuff>();
        registerCertainBuff<SXMQBuff>();
        registerCertainBuff<SXMQBuff_Passive>();
        registerCertainBuff<FrostDecreePulseBuff>();
        registerCertainBuff<InstantCooldownBuff_Beam>();

        // //心相仪与装备套装效果
        registerCertainBuff<EquipmentSetEffectBuff_Beam>();
        registerCertainBuff<IllusoryDreamBuff>();
        registerCertainBuff<ATKIncreaseBuff_IllusoryDream>();
        registerCertainBuff<FloatingExtraSecondaryAttributesBuff_Beam>();
    }

public:
    Initializer_Mage_Beam(Person* p, double deltaTime, int fantasyConfig) : Initializer(p,deltaTime,fantasyConfig){}
};