# Initializer类拓展

## 1.Initializer基类

`Initializer`类是初始化管理器，负责统一管理角色、技能、Buff和自动攻击系统的初始化流程。

```cpp
class Initializer {
protected:
    Person* p;                                      // 关联的角色指针
    std::vector<std::string> equippedSkills{};      // 已装备技能列表
    std::vector<std::string> inherentBuffs{};       // 固有Buff列表
    
    // 必须实现的纯虚函数
    virtual void equipSkills() = 0;                 // 装备可释放技能
    virtual void registerSkills() = 0;              // 注册所有需要的技能
    virtual void registerBuffs() = 0;               // 注册所有需要的Buff
    
    // 工具函数
    void equipCertainSkill(std::string skillName);  // 装备特定技能
    template<typename Skill_> void registerCertainSkill();  // 注册特定技能
    template<typename Buff_> void registerCertainBuff();    // 注册特定Buff
    void initializeAutoAttack();                    // 初始化自动攻击系统
    void initializePerson();                        // 初始化角色状态
    
public:
    void Initialize();                              // 执行完整初始化流程
    Initializer(Person* p);                         // 构造函数
    virtual ~Initializer() = default;               // 虚析构函数
};
```

## 2.初始化流程概览

```cpp
void Initialize()
{
    registerCertainBuff<SkillReleasedTimesStatistics>();  // 统计用，唯一硬编码
    registerSkills();        // 注册所有技能到 SkillCreator
    registerBuffs();         // 注册所有 Buff 到 BuffCreator
    equipSkills();           // 装备角色可释放的技能（会触发幻想的 setPassiveEffect）
    initializePerson();      // 装载固有 Buff、初始化乘区
    initializeAutoAttack();  // 初始化自动攻击系统
}
```

**顺序很关键**：

- `equipSkills()` 早于 `initializePerson()` —— 幻想的 `setPassiveEffect` 在这里跑，
  此时改属性还能影响后面 `initializePerson` 里的推导（如最高副属性 Buff 的判定）；
- `registerCertainBuff<T>()` 会用**编译期** `T::kInherent` 决定要不要把 `T` 加入
  `inherentBuffs`（随后由 `initializePerson` 统一创建），**不再构造临时实例去问**。
  所以固有 Buff 必须在类里写 `static constexpr bool kInherent = true;`，
  同时在构造函数里设 `this->isInherent = true;`（Debug 构建会断言两者一致）。

## 3.拓展新的Initializer子类

### 3.1.在initializer.hpp中声明

```cpp
class Initializer_NewClass : public Initializer {
private: 
    // 必须重写的纯虚函数
    void equipSkills() override;
    void registerSkills() override;
    void registerBuffs() override;
    
public:
    // 构造函数
    explicit Initializer_NewClass(Person* p);
};
```

### 3.2.实现新Initializer子类

```cpp
// 构造函数
Initializer_NewClass::Initializer_NewClass(Person* p) : Initializer(p) {}

// 装备技能（必须实现）
void Initializer_NewClass::equipSkills() {
    // 装备核心技能（必须装备）
    equipCertainSkill("PrimarySkill");
    equipCertainSkill("SecondarySkill");
    equipCertainSkill("UtilitySkill");
    equipCertainSkill("UltimateSkill");
    // ......
}

// 注册技能（必须实现）
void Initializer_NewClass::registerSkills() {
    // 注册技能类
    registerCertainSkill<Skill_1>();
    registerCertainSkill<Skill_2>();
    // ......
}

// 注册Buff（必须实现）
void Initializer_NewClass::registerBuffs() {
    // 注册Buff类
    registerCertainBuff<Buff_1>();
    registerCertainBuff<Buff_2>();
    // ......

}
```
