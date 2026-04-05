#pragma once
#include "../../core/Buff.h"

class Skill;

// 射线部分
// 射线流派天赋
class BeamBuildBuff : public Buff  
{
public:
    static std::string name;
    double lastExtraIncrease = 0;           // 上次修改
    double proficientRatio = 0;             // 精通转增伤率
    double energyRatio = 0;                 // 能量增耗率
    int count = 0;                          // 计数器
    int triggerNum = 0;                     // 触发所需次数

public:
    void listenerCallback(DamageInfo& info);
    void listenerCallback2(double n);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    BeamBuildBuff(Person *p, double n);
    ~BeamBuildBuff();
};

// 自然回能
class NaturalEnergyRegenBuff : public Buff
{
public:
    static std::string name;
    double timer = 0;
    double triggerInterval = 0;

public:
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    NaturalEnergyRegenBuff(Person *p, double n);
};

// 冰凌之约
class IcePromiseBuff : public Buff  
{
public:
    static std::string name;
    double timer;

public:
    void listenerCallback(Skill* const skill);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    IcePromiseBuff(Person *p, double n);
    ~IcePromiseBuff();
};

// 冰晶共鸣&冰光共鸣&冰箭速射（射线触发冰箭）
class FrostCrystalResonanceBuff : public Buff  
{
public:
    static std::string name;
    double triggerTimer = 0;            //计时器
    double triggerInterval = 0;         //触发间隔

public:
    void listenerCallback(DamageInfo& info);
    void listenerCallback2(Skill *const skill);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    FrostCrystalResonanceBuff(Person *p, double n);
    ~FrostCrystalResonanceBuff();
};

// 寒意留存
class ChillPersistenceBuff : public Buff  
{
public:
    static std::string name;
    bool triggered = false;

public:
    void listenerCallback(Skill* const skill);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    ChillPersistenceBuff(Person *p, double n);
    ~ChillPersistenceBuff();
};

// 灌注
class FloodBuff_Beam : public Buff  
{
public:
    static std::string name;
    double energyRevertInterval = 0;
    double timer = 0;

public:
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    FloodBuff_Beam(Person *p, double n);
    ~FloodBuff_Beam();
};

// 智力冰晶
class IntellectCrystalBuff : public Buff  
{
public:
    static std::string name;
    double maxEnergyAdd = 0;

public:
    void listenerCallback(double n);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    IntellectCrystalBuff(Person *p, double n);
    ~IntellectCrystalBuff();
};

// 玄冰潮汐
class IceTideBuff : public Buff  
{
public:
    static std::string name;

public:
    void listenerCallback(DamageInfo& info);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    IceTideBuff(Person *p, double n);
    ~IceTideBuff();
};

// 元素增伤buff（用于玄冰潮汐）
class ElementIncreaseBuff_IceTide : public Buff  
{
public:
    static std::string name;

public:
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    ElementIncreaseBuff_IceTide(Person *p, double n);
    ~ElementIncreaseBuff_IceTide();
};

// 玄冰无界
class IceInfiniteBuff : public Buff  
{
public:
    static std::string name;
    double energyCount = 0;
    double lastChange = 0;      // 记录上次修改

public:
    void listenerCallback(double n);
    void listenerCallback2(double n);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    IceInfiniteBuff(Person *p, double n);
    ~IceInfiniteBuff();
};

// 射线绝唱
class BeamMagnumOpusBuff : public Buff  
{
public:
    static std::string name;

public:
    void listenerCallback(DamageInfo& info);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    BeamMagnumOpusBuff(Person *p, double n);
    ~BeamMagnumOpusBuff();
};

// 双倍精通（用于射线绝唱）
class DoubleProficientBuff : public Buff  
{
public:
    static std::string name;
    double timer = 0;       // 用于防止无限递归的计时器

public:
    void listenerCallback(double n);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    DoubleProficientBuff(Person *p, double n);
    ~DoubleProficientBuff();
};

// 寒风凝聚
class FrostwindFocusBuff : public Buff  
{
public:
    static std::string name;

public:
    void listenerCallback(Skill* const skill);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    FrostwindFocusBuff(Person *p, double n);
    ~FrostwindFocusBuff();
};

// 快速吟唱
class SwiftCastBuff : public Buff  
{
public:
    static std::string name;
    bool triggered = false;

public:
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    SwiftCastBuff(Person *p, double n);
    ~SwiftCastBuff();
};

// 冰晶之力
class FrostCrystalPowerBuff : public Buff  
{
public:
    static std::string name;
    bool triggered = false;

public:
    void listenerCallback(double n);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    FrostCrystalPowerBuff(Person *p, double n);
    ~FrostCrystalPowerBuff();
};

// 涌能法则
class EnergySurgeLawBuff : public Buff  
{
public:
    static std::string name;
    int count = 0;          // 计数器

public:
    void listenerCallback(DamageInfo& info);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    EnergySurgeLawBuff(Person *p, double n);
    ~EnergySurgeLawBuff();
};

// 能量回复（用于涌能法则）
class EnergyRevertBuff_EnergySurgeLaw : public Buff  
{
public:
    static std::string name;
    double triggerInterval = 0;     // 触发间隔
    double timer = 0;               // 计时器

public:
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    EnergyRevertBuff_EnergySurgeLaw(Person *p, double n);
};

class UltiIncreaseBuff_Beam : public Buff
{
public:
    // 大招冰伤
    static std::string name;
    double triggerInterval = 0;
    double timer = 0;

public:
    void listenerCallback(double n);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    UltiIncreaseBuff_Beam(Person *p, double n);
    ~UltiIncreaseBuff_Beam() override;
};

// 装备套装效果 
class EquipmentSetEffectBuff_Beam : public Buff
{
public:   
    static std::string name;

public:
    void listenerCallback(Skill* const skill);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    EquipmentSetEffectBuff_Beam(Person *p, double n);
    ~EquipmentSetEffectBuff_Beam() override;
};

// 射线心相仪：迷幻梦境（征服者）
class IllusoryDreamBuff : public Buff
{
public:   
    static std::string name;
    int triggerCount = 0;// 触发所需层数
    int count = 0;       // 计数器
    double simulateAttackTriggerInterval = 0; // 模拟平a叠层
    double timer = 0;                         // 计时器

public:
    void listenerCallback(DamageInfo& info);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    IllusoryDreamBuff(Person *p, double n);
    ~IllusoryDreamBuff() override;
};

// 攻击力增加（用于迷幻梦境）
class ATKIncreaseBuff_IllusoryDream : public Buff
{
public:   
    static std::string name;

public:
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    ATKIncreaseBuff_IllusoryDream(Person *p, double n);
    ~ATKIncreaseBuff_IllusoryDream() override;
};

// 浮动额外副属性百分比
class FloatingExtraSecondaryAttributesBuff_Beam : public Buff
{
private:
    enum class secondaryAttributesEnum
    {
        CRITICAL,
        QUICKNESS,
        LUCKY,
        PROFICIENT,
        ALMIGHTY
    };
public: 
    static std::string name;
    secondaryAttributesEnum lastAttribute = secondaryAttributesEnum::CRITICAL;

public:
    void listenerCallback(double n);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    FloatingExtraSecondaryAttributesBuff_Beam(Person *p, double n);
    ~FloatingExtraSecondaryAttributesBuff_Beam() override;
};

// 职业因子(G4极性)
class OccupationalFactorBuff_Beam : public Factor
{
public:   
    static std::string name;
    int count = 0;              // 计数器
    int arrowTriggerNum = 5;    // 冰箭触发所需层数
    double timer = 0;           // 计时器
    double beamTriggerInterval = 100; // 射线触发间隔


public:
    void listenerCallback(Skill* const skill);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    OccupationalFactorBuff_Beam(Person *p, double n);
    ~OccupationalFactorBuff_Beam() override;
};

// 冰令脉冲
class FrostDecreePulseBuff : public Buff
{
public:   
    static std::string name;
    int triggerCount = 0;
    int count = 0;

public:
    void listenerCallback(DamageInfo& info);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    FrostDecreePulseBuff(Person *p, double n);
    ~FrostDecreePulseBuff() override;
};

// 冷却瞬息
class InstantCooldownBuff_Beam : public Buff
{
public:   
    static std::string name;
    double triggerNum = 0;
    double count = 0;

public:
    void listenerCallback(double n);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    InstantCooldownBuff_Beam(Person *p, double n);
    ~InstantCooldownBuff_Beam() override;
};

// 龙卷真实因子
class WaterSpoutRealBuff : public RealFactor
{
public:   
    static std::string name;
    double triggerNum = 720;
    bool canTrigger = false;

public:
    void listenerCallback(double n);
    void listenerCallback2(Skill* const skill);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    WaterSpoutRealBuff(Person *p, double n);
    ~WaterSpoutRealBuff() override;
};

// 9冰真实因子
class IceRealBuff : public RealFactor
{
public:   
    static std::string name;
    double triggerNum = 450;

public:
    void listenerCallback(double n);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    IceRealBuff(Person *p, double n);
    ~IceRealBuff() override;
};

// 9冰
class NineIceBuff : public Buff
{
public:   
    static std::string name;

public:
    void listenerCallback(Skill* const skill);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    NineIceBuff(Person *p, double n);
    ~NineIceBuff() override;
};

// 无尽思维
class InfiniteMindBuff : public Buff
{
public:   
    static std::string name;

public:
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    InfiniteMindBuff(Person *p, double n);
    ~InfiniteMindBuff() override;
};