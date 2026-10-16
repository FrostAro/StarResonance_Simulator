# Skill 类扩展

## 1.技能类型体系

```cpp
// 立即触发技能，指单次出伤技能（注：此单次出伤允许一次触发多个数字，例：陨星）
class InstantSkill : public virtual Skill {
    virtual void trigger(Person *p) = 0;  // 触发逻辑（必须实现）
    // 默认use()会调用trigger()
};

// 持续性技能，指释放后脱手的持续出伤技能（例：涡流）
class ContinuousSkill : public virtual Skill {
    virtual void trigger(Person *p) = 0;// 每次达到技能触发间隔时所执行的逻辑
    // 默认use()会检查时间并调用trigger()
};

// 引导技能，指释放后不脱手的持续出伤技能（例：射线）
class FacilitationSkill : public virtual Skill {
    virtual void trigger(Person *p) = 0;// 每次达到技能触发间隔时所执行的逻辑
    virtual bool canEndFacilitation(Person *p) = 0;  // 判断引导结束
    void stop();// 通过将duration变为0实现强行中断
};
```

## 2.扩展新技能步骤

### 2.1.选择继承类型（InstantSkill/ContinuousSkill/FacilitationSkill）

### 2.2.在skill.h中声明

```cpp
class NewSkill : public InstantSkill {  // 以InstantSkill为例
public:
    // 技能名
    static std::string name;

    //技能触发逻辑
    void trigger(Person *p) override;
    // 获取技能名
    std::string getSkillName() const override;
    explicit NewSkill(Person *p);

    // （可选，未应用）设置技能类型
    void setSkillType() override; 

    // 引导技能的结束条件（若为引导性技能则必须实现）
    bool canEndFacilitation(Person *p) override;
};
```

### 2.3.在skill.cpp中实现

```cpp
// 静态名称
std::string NewSkill::name = "NewSkill";

// 构造函数：设置所有必要属性
NewSkill::NewSkill(Person *p) : InstantSkill() {
    // 你应该重写以下成员变量
    // 基本属性
    this->add;                      // 回能值
    this->energyReduce;             // 能量消耗
    this->canTriggerLucky;          // 是否能触发幸运
    this->canCharge;                // 是否为充能技能（技能是否可以存储多层，例：陨星）
    this->isNoReleasing;            // 是否为无前摇技能（例：冰箭）
    
    // 伤害相关
    this->multiplying;              // 倍率
    this->fixedValue;               // 固定值
    this->damageIncreaseAdd;        // 增伤加成
    this->elementIncreaseAdd;       // 元素增伤加成
    this->criticalIncreaseAdd;      // 爆伤加成
    
    // CD相关
    this->MaxCD;                    // 最大CD
    this->MaxchargeCD;              // 充能CD
    this->maxStack;                 // 最大层数
    this->stack = this->maxStack;   // 当前层数
    
    // 时间相关 
    
    this->duration;                 // 持续时间（持续/引导技能用）
    this->baseDamageTriggerInterval;    // 基础伤害触发间隔（持续/引导技能用）
    this->damageTriggerInterval;    // 伤害触发间隔（持续/引导技能用），根据实际情况变化
    //补充：对于无前摇的立即触发型技能技能需要将duration设置为等同于baseDamageTriggerInterval

    // 吟唱时间
    this->singingTime;              
    // 如果吟唱时间受到施法速度影响
    if(p) this->releasingTime /= (1 + p->castingSpeed);

    // 前摇时间（此前摇时间合并了吟唱时间与技能释放时间）
    this->releasingTime = /*技能释放所需时间*/ + this->singingTime;
    // 如果释放时间受到攻击速度影响
    if(p) this->releasingTime /= (1 + p->attackSpeed);   
    
    // 调用类型设置
    this->setSkillType();
}

void NewSkill::setSkillType() {
    // 添加技能类型枚举，可多个
    this->skillTypeList.push_back(Skill::skillTypeEnum::NORMAL);
    this->skillTypeList.push_back(Skill::skillTypeEnum::WITCHCRAFT);
}

void NewSkill::trigger(Person *p) {
    // 技能触发逻辑
    // 使用p->triggerAction<...>()来触发事件
    p->triggerAction<AttackAction>(0, this->getSkillName());
    // 或创建buff
    // p->triggerAction<CreateBuffAction>(参数, Buff名称);
    // 也可以触发其他事件
}

std::string NewSkill::getSkillName() const { return NewSkill::name; }
```
