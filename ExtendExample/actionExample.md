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
    // 1. 每个Action子类需定义静态name；监听器表**不在 Action 里**（见下）
    // 2. execute()中应调用Person相关方法并触发监听器
    // 3. Action通过Person::triggerAction()触发
};
```

> ⚠️ **监听器表已经不在 Action 里了**
>
> 旧版本 `TypedAction` 里有 `inline static std::vector<ListenerPtr> listeners` ——
> 那是**进程级**表，两个 Person 同时存活时会互相污染（A 的 Buff 回调被 B 的事件触发，
> 形成无限递归，实测栈溢出 `0xC00000FD`）。
>
> 现在表在 **`Person::actionListeners`**（`core/ActionListeners.hpp`），每个 Person 一份，
> 按 Action 类型分槽。`TypedAction` 只剩两个类型别名：
>
> ```cpp
> template <typename Derived, typename ListenerType>
> class TypedAction : public Action
> {
> public:
>     using ListenerPtr    = std::unique_ptr<ListenerType>;
>     using ListenerVector = std::vector<ListenerPtr>;
>     std::string getActionName() override { return Derived::name; }
> };
> ```

## 2.拓展新的Action

### 2.1.在Action.h中声明

```cpp
// 第一个模板参数是**这个 Action 自己**（CRTP，用于给该子类生成独有的
// ListenerPtr / ListenerVector 类型别名，监听器表按这个类型分槽），
// 第二个是监听器类型。
class NewAction : public TypedAction<NewAction, ResourceListener> {
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

> 注意 `TypedAction<Derived, ListenerType>` 的第一个参数**必须是派生类自身**。
> 写成 `TypedAction<ResourceRevertAction, ...>` 会让派生类与父类**共用同一个分槽类型**，
> 于是两边的监听器混在一张表里（`listOf<T>()` 用 `typeid` 区分，类型相同就是同一槽）。

### 2.2.在Action.cpp中实现

```cpp
// 定义静态成员
std::string NewAction::name = "NewAction";

// 构造函数
NewAction::NewAction(std::string targetName) : targetName(targetName) {}

// 执行逻辑
void NewAction::execute(double n, Person *p) {
    // 1. 调用Person方法

    // 2. 触发监听器（可选）—— 表在 Person 上，不是 NewAction 上
    for (const auto &listener : p->actionListeners.listOf<NewAction>()) {
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
// 复用父类的监听器槽：本类的 execute 会调用 LuckyCountModifyAction::execute，
// 而父类遍历的是 listOf<LuckyCountModifyAction>()。
//
// ⚠️ 因此不能用 add<LuckyCountModifyAction_Icicle>(...) 注册监听器 ——
//    那会建出一个**没人遍历**的新槽（execute 从来不看它），症状是"注册了但永不触发"。
//    注册与删除都必须用 LuckyCountModifyAction。
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
