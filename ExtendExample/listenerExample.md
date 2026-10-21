# Listener类拓展说明

## 1.Listener概述

Listener系统是一个基于观察者模式的事件监听机制，用于实现游戏中的事件响应和回调逻辑。

**系统架构:**

BaseListener (抽象基类)
    ├── TypedListener\<T\> (模板基类)
    │   ├── DamageListener (伤害监听器)
    │   ├── EnergyListener (能量监听器)
    │   ├── ResourceListener (资源监听器)
    │   ├── CDListener (冷却监听器)
    │   ├── CreateSkillListener (技能创建监听器)
    │   ├── CreateBuffListener (Buff创建监听器)
    │   └── ... (其他监听器)

## 2.核心基类说明

### 2.1.基类BaseListener

```cpp
class BaseListener {
public:
    int buffID;  // 关联的Buff ID
    BaseListener() : buffID(0) {}
    virtual ~BaseListener() = 0;  // 纯虚析构函数
};
```

### 2.2.模板基类TypedListener

```cpp
// T 是事件数据类型（如 DamageInfo / double），回调接收 T&
template <typename T>
class TypedListener : public BaseListener {
public:
    std::function<void(T&)> callback;  // 回调函数（注意是引用，不是 const T）
    
    TypedListener(int id, std::function<void(T&)> func) 
                : callback(std::move(func))
    {
        this->buffID = id;
    }
};
```

> `TypedListener` 本身没有 `trigger()`，各具体监听器（如 `DamageListener`）自行提供。
> 也没有 `isValid()`——要判断"还能不能用"，只能靠 `callback` 是否为空或 `buffID` 是否有效。

## 3.拓展新的Listener类型

### 3.1.在listener.hpp中声明

```cpp
class NewEventListener : public TypedListener<EventInfoType> {
public:
    // 构造函数
    NewEventListener(const int id, const std::function<void(
        const xxx/*根据需要使用不同的类型*/)> func)
        : TypedListener<xxx>(id, func) {}
    
    // 触发方法
    void trigger(const xxx info) const {
        if (callback) callback(info);
    }
};
```

### 3.2.在Action中添加Listener支持

```cpp
// 继承 TypedAction<自身, 监听器类型> 只是拿到 ListenerPtr / ListenerVector 两个别名。
// 监听器表**不在这里** —— 它在 Person::actionListeners（每个 Person 一份），
// 用 typeid(ActionT) 分槽，所以同一个 Action 在不同 Person 上互不相干。
class NewAction : public TypedAction<NewAction, NewEventListener> {
public:
    static std::string name;

    void execute(double n, Person* p) override {
        // 1. 执行逻辑

        // 2. 触发监听器（表在 p 上，用 Action 类型分槽）
        for (const auto &listener : p->actionListeners.listOf<NewAction>()) {
            if (listener && listener->callback) {
                listener->trigger(info);
            }
        }
    }
};
```

> **监听器表为什么挂在 Person 上而不是 Action 上**
>
> 旧实现是 `TypedAction::inline static std::vector<ListenerPtr> listeners` —— 进程级全局。
> 后果：两个 Person 同时存活时，A 的 Buff 注册的回调会被 B 触发的事件调用，
> 互相激活形成无限递归（实测栈溢出 `0xC00000FD`），并且 `Buff::ID` 被迫全局唯一。
>
> 现在表在 `Person::actionListeners`，天然做到：
> · 不同 Person 的表互不相干 → 可以多线程并行跑
> · 同一个 Person 仍只允许单线程访问（`unordered_map` 非线程安全）
>
> ⚠️ **注册与注销必须成对且类型一致**：
> 构造里 `add<NewAction>(...)`，析构里必须 `remove<NewAction>(this->getBuffID())`。
> 类型写错（例如注册用 `NewAction`、注销用 `NewAction_Icicle`）不会报错，
> 只会让监听器永远留在表里 —— 表现为"第二遍模拟结果和第一遍不同"（金丝雀测试会抓）。

### 3.3.在Buff中创建和使用Listener

```cpp
class NewBuff : public Buff {
public:
    static std::string name;
    
    NewBuff(Person* p, double n) : Buff(p) {
        // 初始化属性
        this->duration = 1000;
        this->maxDuration = this->duration;
        
        // 创建并注册监听器
        auto listener = std::make_unique<NewEventListener>(
            this->getBuffID(),
            [this](const xxx& info) {
                this->onEventTriggered(info);
            }
        );
        
        // 添加到对应的Action中
        p->actionListeners.add<NewAction>(std::move(listener));
    }
    
    ~NewBuff() override {
        // 清理时移除监听器
        p->actionListeners.remove<NewAction>(this->getBuffID());
    }
    
private:
    // 监听器回调函数
    void listenerCallBack(const xxx& info) {
        // 处理事件
        if (shouldRespondToEvent(info)) {
            respondToEvent(info);
        }
    }
    
    bool shouldRespondToEvent(const xxx& info) {
        // 判断是否响应此事件
        return true;
    }
    
    void respondToEvent(const xxx& info) {
        // 具体响应逻辑
        Logger::debugBuff(AutoAttack::getTimer(),
                          this->getBuffName(),
                          " triggered by event");
    }
};
```
