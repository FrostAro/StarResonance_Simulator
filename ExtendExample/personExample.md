# Person类拓展

## 1.基类Person

Person类是所有角色职业的基类，定义了角色的基础属性、技能系统、Buff系统和事件系统。

```cpp
/* 简述 */
class Person {
    // 基础属性
    double Critical;                // 暴击
    double Lucky;                   // 幸运
    double Almighty;                // 全能
    double Proficient;              // 精通
    double Quickness;               // 急速
    // 下略...
    
    // 攻击属性
    double ATK;                     // 基础攻击力
    double refineATK;               // 精炼攻击力
    double elementATK;              // 元素攻击力
    // 下略...
    
    // 乘区系统
    double attackIncrease;          // 攻击力增加
    double damageIncrease;          // 伤害增加
    double elementIncrease;         // 元素伤害增加
    double criticalDamage;          // 暴击伤害
    // 下略...
    
    // 技能系统
    std::vector<std::unique_ptr<Skill>> continuousSkillList;  // 持续性技能列表
    std::vector<std::unique_ptr<Skill>> skillCDList;          // 技能CD列表
    std::unique_ptr<Skill> nowReleasingSkill;                 // 当前释放的技能
    
    // Buff系统
    std::vector<std::unique_ptr<Buff>> buffList;              // Buff列表
    
    // 事件系统
    std::queue<ActionInfo> actionQueue;                       // 事件队列
    
    // 必须实现的函数
    virtual ~Person() = 0;                                    // 虚析构函数
    
    // 关键接口
    DamageInfo Damage(const Skill *skill);                    // 计算伤害
    void createSkill(std::unique_ptr<Skill> newSkill);        // 创建技能
    void createBuff(std::unique_ptr<Buff> newBuff);           // 创建Buff
    template <typename T, typename... Args>
    void triggerAction(double count, Args &&...args);         // 触发事件
    // 下略...
};
```

## 2.拓展新的Person

### 2.1.Person.h中声明

```cpp
class NewPerson : public Person
{
public:
    friend class Initializer;

    NewPerson(const double attributes, const double critical, const double quickness, 
              const double lucky, const double Proficient, const double almighty,
              const int atk, const int refindatk, const int elementatk, 
              const double attackSpeed, const double castingSpeed,
              const double critialdamage_set, const double increasedamage_set, 
              const double elementdamage_set, const int totalTime,
              int fantasyConfig = 0, const WeaponConfig weaponConfig = {});
};
```

> `weaponConfig` 用于声明「武器的第一/第二属性」：这两项在游戏里会加进站街面板，
> 所以输入的面板值已经包含它们，代码内部走 `Person::reclassifyXxxPercentToExtra` 把
> 这段百分比从点数通道切到额外通道（总数不变、不参与点数换算）。

### 2.2.Person.cpp中实现

```cpp
// 构造函数实现
NewPerson::NewPerson(const double attributes, const double critical, const double quickness,
                   const double lucky, const double Proficient, const double almighty,
                   const int atk, const int refindatk, const int elementatk,
                   const double attackSpeed, const double castingSpeed,
                   const double critialdamage_set, const double increasedamage_set,
                   const double elementdamage_set, const int totalTime,
                   int fantasyConfig, const WeaponConfig weaponConfig)
    : Person(attributes, critical, quickness, lucky, Proficient, almighty,
             atk, refindatk, elementatk, attackSpeed, castingSpeed,
             critialdamage_set, increasedamage_set, elementdamage_set, totalTime, weaponConfig)
{
    // 1. 设置职业特有系数（必须设置）
    this->primaryAttributeRatio     // 主属性转化率：主属性→攻击力
    this->proficientRatio           // 精通转化率：精通→元素增伤
    this->almightyRatio             // 全能转化率：全能→全能增伤
    this->maxResourceNum            // 最大资源数（玄冰）
    this->castingSpeedRatio         // 施法速度转化率：急速→施法速度
    this->attackSpeedRatio          // 攻击速度转化率：急速→攻击速度
    this->coolDownReduce            // 冷却缩减

    this->damageReduce              // 伤害减免（魔法为10%，物理为30%）

    // 2. 设置基础攻击力（注意是两步：setATK 存基础值，resetATK 按主属性重算）
    setATK(atk);
    resetATK();
    
    // 3. 创建职业特有的AutoAttack控制器（必须）
    // 如果有多中配置使用fantasyConfig创建
    this->autoAttackPtr = std::make_unique<AutoAttack_NewPerson>(this);
    
    // 4. 初始化乘区
    this->initializeIncrease();

    // 5. 天赋/被动效果（可选）
    // 例如：冰矛天赋的幸运倍率加成
    changeLuckyMultiplyingByAddMultiplying(0.15 + (this->Lucky - 0.05) / 2);
    // 例2：冰矛天赋的乘区增加
    changeDamageIncrease(0.08);
    changeElementIncreaseByElementIncrease(0.1);
    
    // 6. 其他职业特有初始化
    // ...
}

// （可选）职业特有函数实现
void NewClass::xxx() {
    // 职业特有逻辑
    // 可以使用this->triggerAction<...>()触发事件
    // 或this->createBuff()创建Buff
    // 或直接修改角色属性
}
```

## 3.职业特有系数说明

每个职业需要设置以下特有系数

| 系数                | 说明                            | 示例值（射线） |
| :---:               | :-----:                         | :---:          |
| primaryAttributeRatio | 主属性转化率（主属性→攻击力） | 0.6            |
| proficientRatio     | 精通转化率（精通→元素增伤）     | 0.2            |
| almightyRatio       | 全能转化率（全能→全能增伤）     | 0.35           |
| maxResourceNum      | 最大资源数（如玄冰数）          | 3              |
| castingSpeedRatio   | 施法速度转化率（急速→施法速度） | 2.0            |
| attackSpeedRatio    | 攻击速度转化率（急速→攻击速度） | 0.2            |
| coolDownReduce      | 冷却缩减                        | 0.1            |
| damageReduce        | 伤害减免（魔法10%，物理30%）    | 0.1            |

## 4.对于属性修改部分职业需要特殊实现时

属性修改的特殊实现可以通过继承重写对于事件类实现，也可以在继承的角色类中直接重写对应的修改逻辑

以冰矛为例：

### 4.1.在Person.h中实现

```cpp
class Mage_Icicle : public Person
{
public:
    friend class Initializer;

    Mage_Icicle(const double primaryAttributes, const double critical, const double quickness, const double lucky, const double Proficient, const double almighty,
        const int atk, const int refindatk, const int elementatk, const  double attackSpeed, const double castingSpeed,
        const  double critialdamage_set, const double increasedamage_set, const double elementdamage_set, const int totalTime);

    // 这里是重写的修改逻辑
    double changeLuckyPercent(double percent) override;
    double changeLuckyCount(int addCount) override;
};
```

### 在Person.cpp中实现

```cpp
double Mage_Icicle::changeLuckyCount(int addCount)
{
    // 原有逻辑
    Person::changeLuckyCount(addCount);
    // 职业的特殊逻辑，此处为冰矛的额外幸运倍率天赋
    // 此次重写是因为幸运倍率要随着幸运面板的变化而变化，与职业构造函数的初始化不同
    changeLuckyMultiplyingByAddMultiplying(0.15 + (this->Lucky - 0.05) / 2);
    return this->Lucky;
}

double Mage_Icicle::changeLuckyPercent(double percent)
{
    // 原有逻辑
    Person::changeLuckyPercent(percent);
    // 职业的特殊逻辑，此处为冰矛的额外幸运倍率天赋
    changeLuckyMultiplyingByAddMultiplying(0.15 + (this->Lucky - 0.05) / 2);
    return this->Lucky;
}
```

### 5.注册新的幻想配置

`fantasyConfig` 是裸 `int`，**与 GUI 下拉框的 index 一一对应**（没有单独的枚举）。新增一档只需四处同步改：

1. `mainwindow.cpp` —— 两处 `m_fantasyCombo->addItem(...)`（初始化 + `onProfessionChanged`），
   顺序必须与 config 编号一致；
2. `<职业>/Person.cpp` —— 构造函数 `switch (fantasyConfig)` 加 `case N`，创建对应的 `AutoAttack_*`；
3. `<职业>/Initializer.hpp` —— `equipSkills()` 里加 `else if (m_fantasyConfig == N) { equipCertainSkill(...); }`，
   `registerSkills()` / `registerBuffs()` 里注册新技能与 Buff；
4. `README.md` / `main_Beam.cpp` 注释里的「0-N」范围说明。

如果需要新的自动战斗类，请实现它并在 Person 中创建（见 `autoAttackExample.md`）。

### 6.幻想的被动效果：`setPassiveEffect` 与「面板已含」加成

`Person::equipSkill` 会对 `FightingFantasy` 子类调用 `setPassiveEffect(this)`——
**这是幻想被动唯一的挂载点**，时机在 `Initializer::equipSkills()`，早于 `initializePerson()`。

普通被动直接改属性即可（例：`p->triggerAction<AttackIncreaseModifyAction>(0.15)`）。

但如果这个加成**在游戏里会直接进站街面板**（即玩家输入的面板值已经包含它），
就必须用「重新归类」而不是「再加一遍」：

```cpp
void JQR::setPassiveEffect(Person *p)
{
    FantasyPassiveBonus bonus;
    bonus.proficient = JQR::kPassiveProficient;   // 0.15 = 15%
    p->applyFantasyPassiveBonus(bonus, true);     // 总值不变，只把这段切到额外通道
}
```

原因：面板值反解点数时会被 `count/(count+coeff)` 放大，而且会被「双倍精通」一并翻倍；
而这段本质是独立百分比，不该参与点数换算。

`FantasyPassiveBonus` 支持 `proficient / quickness / critical / lucky / almighty` 五项。

> ⚠️ `Person::reclassifyXxxPercentToExtra` **仅限初始化期调用**。它直接改成员变量、不广播 Action 事件，
> 战斗中调用会让 `DoubleProficientBuff` / `FloatingExtraSecondaryAttributesBuff*` / `JQRBuff`
> 的缓存值脱钩，且不报错。
>
> `weaponConfig` 的武器属性走的是同一套机制。
