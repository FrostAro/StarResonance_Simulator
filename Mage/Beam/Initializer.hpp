#pragma once
#include "../../core/Initializer.hpp"
#include "../../FightingFantasy/Buff.h"
#include "../../FightingFantasy/Skill.h"
#include "Buff.h"
#include "Skill.h"

// 射线流派：仅影响 Buff 选择，不影响职业/幻想配置
enum class BeamFlow
{
    QuicknessProficient = 0,  // 急速精通流
    Lucky = 1                 // 幸运流
};

class Initializer_Mage_Beam : public Initializer
{
    int m_flowConfig = 0;  // 0=急速精通流，1=幸运流
    void equipSkills() override
    {   
        //配置技能:射线
        equipCertainSkill(Beam::name);
        equipCertainSkill(Vortex::name);
        equipCertainSkill(WaterSpout::name);
        equipCertainSkill(FrostWind::name);
        equipCertainSkill(Flood_Beam::name);
        equipCertainSkill(Ultimate_Beam::name);
        // 根据幻想配置装备第二个幻想技能
        if (m_fantasyConfig == 0) {
            equipCertainSkill(MukuChief::name);
            equipCertainSkill(MukuScout::name);   // 姆头+尖兵
        } else if (m_fantasyConfig == 1) {
            equipCertainSkill(MukuChief::name);
            equipCertainSkill(YGLWS::name);       // 姆头+伊戈雷乌斯
        } else if (m_fantasyConfig == 2) {
            equipCertainSkill(MukuChief::name);
            equipCertainSkill(SXMQ::name);        // 姆头+嗜血毛球
        }
        else if (m_fantasyConfig == 3) {
            equipCertainSkill(HYXZ::name);        // 幻妖蟹蛛+嗜血毛球
            equipCertainSkill(SXMQ::name);
        }
        else if (m_fantasyConfig == 4) {
            equipCertainSkill(MukuScout::name);   // 姆克尖兵+嗜血毛球
            equipCertainSkill(SXMQ::name);
        }
        else if (m_fantasyConfig == 5) {
            equipCertainSkill(LSZZ::name);        // 掠食蜘蛛+嗜血毛球
            equipCertainSkill(SXMQ::name);
        }
        else if (m_fantasyConfig == 6) {
            equipCertainSkill(YZ::name);        // 游子+嗜血毛球
            equipCertainSkill(SXMQ::name);
        }
        else {
            // 默认配置(无幻想)
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
        // registerCertainSkill<FantasyImpact_Beam>();
        registerCertainSkill<MukuChief>();
        registerCertainSkill<MukuScout>();
        registerCertainSkill<YGLWS>();
        registerCertainSkill<SXMQ>();
        registerCertainSkill<HYXZ>();
        registerCertainSkill<LSZZ>();
        registerCertainSkill<YZ>();
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
        registerCertainBuff<HYXZBuff>();
        registerCertainBuff<SXMQBuff_Passive>();
        registerCertainBuff<LSZZBuff>();
        registerCertainBuff<YZBuff>();
        registerCertainBuff<FrostDecreePulseBuff>();
        registerCertainBuff<InstantCooldownBuff_Beam>();
        registerCertainBuff<ExtensiveArrow>();
        //registerCertainBuff<OtherExtraEnhanceBuff>();

        //心相仪与装备套装效果
        registerCertainBuff<FloatingExtraSecondaryAttributesBuff_Beam>();
        registerCertainBuff<ConquerorBuff>();
        registerCertainBuff<StackMomentumBuff>();
        registerCertainBuff<BreakThroughBuff>();
        //registerCertainBuff<CoefficientAdjustmentBuff_Beam>();
        registerCertainBuff<EquipmentSetEffectBuff_Beam>();

        // 流派专属 Buff：急速精通流 / 幸运流
        if (m_flowConfig == static_cast<int>(BeamFlow::QuicknessProficient))
        {
            registerCertainBuff<WaterSpoutRealBuff>();
            registerCertainBuff<NineIceBuff>();
            registerCertainBuff<IceRealBuff>();
        }
        else if (m_flowConfig == static_cast<int>(BeamFlow::Lucky))
        {
            registerCertainBuff<IceArrowLuckyRealBuff>();
            registerCertainBuff<FloodLuckyBuff>();
            registerCertainBuff<VortexLuckyDoubledBuff>();
        }
    }

public:
    Initializer_Mage_Beam(Person* p, double deltaTime, int fantasyConfig, int flowConfig = 0)
        : Initializer(p, deltaTime, fantasyConfig), m_flowConfig(flowConfig) {}
};