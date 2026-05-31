# Action类拓展

## 1.基类Action

```cpp
class Action {
    // 重要成员
    static std::string name;                          // action名称（静态）

    // 必须实现的成员函数
    virtual void execute(double n, Person *p) = 0;    // 执行action的逻辑
    virtual std::string getActionName();              // 返回action名称
    
    
    // 使用说明
    // 1. 每个Action子类需定义静态name和listeners
    // 2. execute()中应调用Person相关方法并触发监听器
    // 3. Action通过Person::triggerAction()触发
};
```

## 2.拓展新的Action

### 2.1.在Action.h中声明

```cpp
class NewAction : public TypedAction<ResourceRevertAction, ResourceListener> {
private:
    // 可选：构造函数参数相关成员，如buffName、skillName
    std::string targetName;
    
public:
    static std::string name;// 事件名 

    NewAction();

    /**
     * @brief 事件行为
     * @param n 自定义应用值
     * @param p 执行动作的角色
     */
    void execute(double n, Person *p) override; 
};
```

### 2.2.在Action.cpp中实现

```cpp
// 定义静态成员
std::string NewAction::name = "NewAction";

// 构造函数
NewAction::NewAction(std::string targetName) : targetName(targetName) {}

// 执行逻辑
void NewAction::execute(double n, Person *p) {
    // 1. 调用Person方法

    // 2. 触发监听器（可选）
    for (const auto &listener : NewAction::listeners) {
        if (listener && listener->callback) {
            listener->trigger(参数); // 参数类型需匹配Listener
        }
    }
}
```

## 3.对于属性修改部分职业需要特殊实现时

### 3.1.在Action.h中实现

以冰矛的幸运为例：

```cpp
// 幸运数值
class LuckyCountModifyAction_Icicle : public LuckyCountModifyAction
{
// 直接使用父类的监听器与对应方法，无需再次定义
private:
    static std::string name;

public:
    LuckyCountModifyAction_Icicle();
    void execute(double n, Person *p) override; // n为增加的幸运数值
    std::string getActionName() override;
};


// 幸运百分比
class LuckyPercentModifyAction_Icicle : public LuckyPercentModifyAction
{
private:
    static std::string name;

public:
    LuckyPercentModifyAction_Icicle();
    void execute(double n, Person *p) override; // n为增加的幸运百分比
    std::string getActionName() override;
};
```

### 3.2.在Action.cpp中实现

```cpp
// 幸运数值
std::string LuckyCountModifyAction_Icicle::name = "LuckyCountModifyAction_Icicle";

LuckyCountModifyAction_Icicle::LuckyCountModifyAction_Icicle()
    : LuckyCountModifyAction() {}

void LuckyCountModifyAction_Icicle::execute(double n, Person *p)
{
    // 调用父类的excute()
    LuckyCountModifyAction::execute(n,p);
    // 子类的特殊实现，此处为冰矛的额外幸运倍率天赋
    p->changeLuckyMultiplyingByAddMultiplying(0.15 + (p->Lucky - 0.05) / 2);
}

std::string LuckyCountModifyAction_Icicle::getActionName() { return LuckyCountModifyAction_Icicle::name; }


// 幸运百分比
std::string LuckyPercentModifyAction_Icicle::name = "LuckyPercentModifyAction_Icicle";

LuckyPercentModifyAction_Icicle::LuckyPercentModifyAction_Icicle()
    : LuckyPercentModifyAction() {}

void LuckyPercentModifyAction_Icicle::execute(double n, Person *p)
{
    LuckyPercentModifyAction::execute(n,p);
    p->changeLuckyMultiplyingByAddMultiplying(0.15 + (p->Lucky - 0.05) / 2);
}

std::string LuckyPercentModifyAction_Icicle::getActionName() { return LuckyPercentModifyAction_Icicle::name; }
```
