#pragma once
#include "../../core/AutoAttack.h"
#include <vector>
#include <string>
#include <utility>
#include <map>

class Person;
class Beam;

// 所有 Beam 法系自动爆发的基类
class AutoAttack_Mage_Beam_Base : public AutoAttack
{
    enum class OutBurstTypeEnum;
    struct BurstStage;
    struct OutBurstLogic;
    
public:
    AutoAttack_Mage_Beam_Base(Person *p, const std::vector<BurstStage> &stages);
    ~AutoAttack_Mage_Beam_Base() override = default;

    void update(int deltaTime) override final;

protected:
    std::map<OutBurstTypeEnum, int> m_logicToStageIndex;
    std::vector<BurstStage> m_stages;
    std::map<OutBurstTypeEnum, OutBurstLogic> m_outBurstLogicMap;
    std::queue<OutBurstTypeEnum> m_outBurstQueue;

    void windowPeriodLogic() override;
    void checkAndFinishOutBurst() override;
    void addOutBurstLogicToQueue(OutBurstTypeEnum type);

    /**
     * @brief 配置硬编码爆发轴（仿照 LSZZ 轴）
     *
     * 设置爆发逻辑 -> 阶段下标的映射（[6]~[10]），并把固定的爆发顺序压入队列：
     * 双幻想+大招 -> 窗口 -> 爆发 -> 窗口 -> 爆发 -> 窗口 -> 双幻想+大招
     * -> 窗口 -> 爆发 -> 窗口 -> 爆发 -> 窗口 -> 双幻想+大招
     * （共13段：双幻想+大招 x3、爆发 x4、窗口 x6，合计176s，可跑满180s上限）
     *
     * @param simpleBurstInSecondBurst 第二次爆发是否改用 simpleFantasyOnly。
     *        幻想带2层充能时用 true：第二波另一个幻想还在CD，只再放那个2层充能的幻想。
     */
    void setupHardcodedOutBurstAxis(bool simpleBurstInSecondBurst = false);

private:
    void tryTriggerStage(int stageIdx);
    bool checkSkills(const std::vector<std::pair<std::string, bool>> &skills) const;
    void addSkillsToList(const std::vector<std::string> &skills);
    void checkSpecificLogicForOutBurst();
    void registerOutBurstLogic();
    int nextOutBurstSignal = 1;
    bool nextIsWindow = false;
    bool windowSkillTriggered = false; // 替代原static变量
    bool windowSecondBeamTriggered = false;
    int timerForVariousStages = 0; // 用于不同阶段的计时器
    OutBurstTypeEnum currentOutBurstType = OutBurstTypeEnum::None;

protected:
    enum class OutBurstTypeEnum
    {
        twoFantasyAndUlti = 0,
        simpleFantasyOnly = 1,
        ultiOnly = 2,
        pureBurst = 3,
        window = 4,
        None = 5
    };

    // 阶段配置：检查的技能列表 + 要添加的技能列表
    struct BurstStage
    {
        // 每个技能：名称，是否允许用堆叠替代CD（true表示允许stack>0也算就绪）
        std::vector<std::pair<std::string, bool>> checkSkills;
        std::vector<std::string> addSkills;
    };

    struct OutBurstLogic
    {
        int lastingTime; // 持续时间（毫秒）
        std::function<void()> logic; // 逻辑函数
    };
};

// -------------------- 具体派生类 --------------------
class AutoAttack_Mage_Beam_MukuScout : public AutoAttack_Mage_Beam_Base
{
public:
    explicit AutoAttack_Mage_Beam_MukuScout(Person *p);
};

class AutoAttack_Mage_Beam_YGLWS : public AutoAttack_Mage_Beam_Base
{
public:
    explicit AutoAttack_Mage_Beam_YGLWS(Person *p);
};

class AutoAttack_Mage_Beam_SXMQ : public AutoAttack_Mage_Beam_Base
{
public:
    explicit AutoAttack_Mage_Beam_SXMQ(Person *p);
};

class AutoAttack_Mage_Beam_HYXZ : public AutoAttack_Mage_Beam_Base
{
public:
    explicit AutoAttack_Mage_Beam_HYXZ(Person *p);
};

class AutoAttack_Mage_Beam_JBMQ : public AutoAttack_Mage_Beam_Base
{
public:
    explicit AutoAttack_Mage_Beam_JBMQ(Person *p);
};

class AutoAttack_Mage_Beam_LSZZ : public AutoAttack_Mage_Beam_Base
{
public:
    explicit AutoAttack_Mage_Beam_LSZZ(Person *p);
};

class AutoAttack_Mage_Beam_YZ : public AutoAttack_Mage_Beam_Base
{
public:
    explicit AutoAttack_Mage_Beam_YZ(Person *p);
};