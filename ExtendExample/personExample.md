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
    double refindATK;               // 精炼攻击力
    double elementATK;              // 元素攻击力
    // 下略...
    
    // 乘区系统
    double attackIncrease;          // 攻击力增加
    double damageIncrease;          // 伤害增加
    double elementIncrease;         // 元素伤害增加
    double criticicalDamage;        // 暴击伤害
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
              const double elementdamage_set, const int totalTime, int fantasyConfig);
};
```

### 2.2.Person.cpp中实现

```cpp
// 构造函数实现
NewPerson::NewPerson(const double attributes, const double critical, const double quickness,
                   const double lucky, const double Proficient, const double almighty,
                   const int atk, const int refindatk, const int elementatk,
                   const double attackSpeed, const double castingSpeed,
                   const double critialdamage_set, const double increasedamage_set,
                   const double elementdamage_set, const int totalTime, int fantasyConfig)
    : Person(attributes, critical, quickness, lucky, Proficient, almighty,
             atk, refindatk, elementatk, attackSpeed, castingSpeed,
             critialdamage_set, increasedamage_set, elementdamage_set, totalTime)
{
    // 1. 设置职业特有系数（必须设置）
    this->attributeRatio            // 属性转化率：智力→攻击力
    this->ProficientRatio           // 精通转化率：精通→元素增伤
    this->AlmightyRatio             // 全能转化率：全能→全能增伤
    this->maxResourceNum            // 最大资源数（玄冰）
    this->castingSpeedRatio         // 施法速度转化率：迅捷→施法速度
    this->attackSpeedRatio          // 攻击速度转化率：迅捷→攻击速度
    this->coolDownReduce            // 冷却缩减

    this->damageReduce              // 伤害减免（魔法为10%，物理为30%）

    // 2. 设置基础攻击力
    setATK();
    
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

| 系数              | 说明                          | 示例值     |
| :---:             | :-----:                       | :---:      |
| attributeRatio    | 属性转化率(智力转攻击力)      | 0.6        |
| ProficientRatio   | 精通转化率(精通转元素增伤)    | 0.65       |
| AlmightyRatio     | 全能转化率(全能转全能增伤)    | 0.35       |
| maxResourceNum    | 最大资源数(如玄冰数)          | 4.00       |
| castingSpeedRatio | 施法速度转化率(急速转施法速度)| 2.00       |
| attackSpeedRatio  | 攻击速度转化率(急速转攻击速度)| 0.20       |
| damageReduce      | 伤害减免(魔法为10%，物理为30%)| 0.1        |

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
    double changeLuckyPersent(double persent) override;
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

double Mage_Icicle::changeLuckyPersent(double persent)
{
    // 原有逻辑
    Person::changeLuckyPersent(persent);
    // 职业的特殊逻辑，此处为冰矛的额外幸运倍率天赋
    changeLuckyMultiplyingByAddMultiplying(0.15 + (this->Lucky - 0.05) / 2);
    return this->Lucky;
}
```

### 5.注册新的幻想配置

如果需要添加新的幻想配置（例如未来新增的幻想组合），只需：
1.在对应职业的枚举中添加新值。
2.在 onProfessionChanged 中为对应职业的下拉框添加新选项。
3.在 Person 构造函数中添加对应的 case 分支。
4.在 Initializer 的 equipSkills() 中添加对应的装备逻辑。
5.如果需要新的自动攻击类，请实现它并在 Person 中创建。
