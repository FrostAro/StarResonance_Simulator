# Buff类拓展

## 1.基类Buff

```cpp
`Buff`类是游戏中所有状态效果的基类，定义了`Buff`的基础属性和生命周期管理机制
class Buff {
protected:
    static std::string name;            // Buff名称（静态）
    static int ID;                      // BuffID计数（静态）
    
    int buffID = 0;                     // Buff唯一ID
    double stack = 0;                   // 层数
    double maxStack = 0;                // 最大层数
    double lastStack = 0;               // 记录上一次更新的层数
    double number = 0;                  // 数值（具体含义由子类定义）
    double duration = 0;                // 持续时间
    double maxDuration = 0;             // 最大持续时间
    
    // 标志位
    bool isStackable = false;           // 是否可堆叠
    bool allowDuplicates = false;       // 允许重复创建，每个独立存在
    bool isInherent = false;            // 是否永久生效
    
    Person *p = nullptr;                // 所属角色指针

public:
    // 必须实现的虚函数
    virtual void update(double deltaTime) = 0;  // 每tick更新逻辑
    virtual bool shouldBeRemoved() = 0;         // 判断是否应被移除
    virtual std::string getBuffName() const;    // 获取Buff名称
    
    // 公共接口
    explicit Buff(Person *p);
    virtual ~Buff() = default;
    
    void addStack(double n);                     // 增加层数
    bool operator==(const Buff &other) const;    // 通过buffID对比
    static int getID();                          // 获取下一个可用ID
    
    // CD和持续时间管理
    void resetDuration();                        // 重置持续时间
    bool reduceDuration(int deltaTime);          // 减少持续时间
    
    // Getter方法
    int getBuffID() const;
    // 下略...
    
    static void resetID();                       // 重置ID计数器（用于测试）
};
```

## 2.拓展新的Buff

### 2.1.在buff.h中声明

```cpp
class NewBuff : public Buff {
public:
    // Buff名称（静态，必须定义）
    static std::string name;

    // 是否为"固有Buff"（常驻，如因子/职业被动），必须与构造函数里的 isInherent 一致：
    //   - 固有Buff：写 static constexpr bool kInherent = true;  构造函数里同时 this->isInherent = true;
    //   - 普通Buff：不写这一行即可（编译期探测会回退为 false）
    // Initializer 靠这个编译期标记决定要不要在初始化时自动装上该Buff，不再构造临时实例去问。
    // 忘记写标记而构造函数里设了 isInherent=true 时，Debug 构建会在 Initialize() 末尾断言报错。
    static constexpr bool kInherent = true;
    
    // 监听器回调函数（可选，根据实际需要三选一）
    void listenerCallback(const DamageInfo &info);        // 伤害监听
    void listenerCallback(double n);                      // 数值监听
    void listenerCallback(const DamageInfo &info) const;  // 常量版本
    
    // 必须实现的虚函数
    void update(double deltaTime) override;               // 每tick更新逻辑
    bool shouldBeRemoved() override;                      // 移除条件
    std::string getBuffName() const override;             // 返回Buff名称
    
    // 构造函数
    explicit NewBuff(Person *p);       // 基础构造函数
    NewBuff(Person *p, double n);      // 带参数构造函数
    
    // 析构函数（可选，用于清理监听器）
    ~NewBuff() override;
    
private:
    // （可选）私有成员变量
    double customTimer = 0;      // 自定义计时器
    int triggerCount = 0;        // 触发计数
    // ... 其他私有成员
};
```

### 2.2 在buff.cpp中实现

```cpp
// 定义静态成员
std::string NewBuff::name = "NewBuff";

// 基础构造函数
NewBuff::NewBuff(Person *p) : Buff(p) {
    // 你必须重写如下成员变量
    this->number;                       // 用作具体数值
    this->maxStack;                     // 最大层数
    this->duration;                     // 持续时间（毫秒）
    this->maxDuration = this->duration; // 最大持续时间
    this->isStackable;                  // 是否可堆叠
    this->allowDuplicates;              // 是否允许重复创建
    this->isInherent;                   // 是否为固有Buff
    
    //（可选）绑定监听器
    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), 
        [this](const DamageInfo &damageInfo) {
            this->listenerCallback(damageInfo);
        }
    );
    AttackAction::addListener(std::move(info));
    
    //（可选）初始化角色属性
    this->p->changeDamageIncrease(this->number);
    // ⚠️ 如果这段增益在游戏里会直接进站街面板（玩家输入已包含），
    //    不能用 triggerAction 再加一遍，应改用「重新归类」：
    //    this->p->applyFantasyPassiveBonus(bonus, true);
    //    详见 personExample.md 第 6 节与 SkillLogic.md 7.4。
}

// 带参数构造函数
NewBuff::NewBuff(Person *p, double n) : Buff(p) {
    this->stack = n;                     // 初始层数
    this->number = 0.1;                  // 用作具体数值
    this->maxStack = 10;
    this->duration = 1000;
    this->maxDuration = this->duration;
    this->isStackable = true;
    
    // 绑定监听器
    auto info = std::make_unique<DamageListener>(
        this->getBuffID(), 
        [this](const DamageInfo &damageInfo) {
            this->listenerCallback(damageInfo);
        }
    );
    AttackAction::addListener(std::move(info));
}

// 监听器回调函数（可选，以出伤回调为例）
void NewBuff::listenerCallback(const DamageInfo &info) {
    // 1. 条件检查（如果需要）
    if (info.skillName != Spear::name) return;  // 只对特定技能生效
    
    // 2. 概率判断（如果需要）
    if (!this->p->isSuccess(this->number)) return;
    
    // 3.触发逻辑......
    
    // 4. 日志记录
    Logger::debugBuff(AutoAttack::getTimer(),
                      this->getBuffName(),
                      " - Stack: " + std::to_string(this->stack) + 
                      " / " + std::to_string(this->maxStack));
}

// 每tick更新逻辑
void NewBuff::update(double deltaTime) {
    // 1. 更新内部计时器（如果有）
    this->customTimer += deltaTime;
    
    // 2. 定时触发逻辑（如果有）
    if (static_cast<int>(this->duration) % 100 == 0) {
        // 每100毫秒触发一次
        this->p->triggerAction<EnergyRevertAction>(this->number);
    }
    
    // 3. 层数检查逻辑（如果有）
    if (this->stack >= this->maxStack && this->maxStack > 0) {
        // 达到最大层数时触发效果
        this->p->triggerAction<CreateSkillAction>(0, "SpecialSkill");
        this->stack = 0;  // 重置层数
    }
    
    // 4. 状态更新
    if (this->customTimer >= 500) {  // 每500毫秒
        this->customTimer = 0;
        // ... 周期性逻辑
    }
}

// 判断是否应被移除
bool NewBuff::shouldBeRemoved() {
    // 1. 持续时间结束
    if (this->duration < 0) return true;
    
    // 2. 特定条件移除
    if (this->p->findBuffInBuffList(RequiredBuff::name) == -1) return true;
    
    // 3. 其他条件...
    
    return false;
}

// 获取Buff名称
std::string NewBuff::getBuffName() const {
    return NewBuff::name;
}

// 析构函数（可选，用于清理监听器）
NewBuff::~NewBuff() {
    // 1. 移除监听器
    AttackAction::deleteListener(this->getBuffID());
    
    // 2. 恢复角色属性
    this->p->changeDamageIncrease(-this->number);
    
    // 3.日志记录
    Logger::debugBuff(AutoAttack::getTimer(),
                      this->getBuffName(),
                      " - destroyed, removed listener and restored stats");
}
```
