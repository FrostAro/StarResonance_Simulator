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
// 推荐直接继承 TypedAction<自身, 监听器类型>，listeners / addListener / deleteListener
// 都由它提供，不用手写。
class NewAction : public TypedAction<NewAction, NewEventListener> {
public:
    static std::string name;

    void execute(double n, Person* p) override {
        // 1. 执行逻辑

        // 2. 触发监听器（遍历 TypedAction 提供的 static listeners）
        for (const auto &listener : NewAction::listeners) {
            if (listener && listener->callback) {
                listener->trigger(info);
            }
        }
    }
};
```

> 如果确实要手写监听器容器，记得 `addListener` / `deleteListener` 要成对使用：
> Buff 构造里 `addListener`，析构里必须 `deleteListener(this->getBuffID())`，
> 否则静态表里会留下指向已析构对象的回调。

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
        NewAction::addListener(std::move(listener));
    }
    
    ~NewBuff() override {
        // 清理时移除监听器
        NewAction::deleteListener(this->getBuffID());
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
