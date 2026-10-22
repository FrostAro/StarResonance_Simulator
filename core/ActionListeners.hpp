#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

// ============================================================================
// 每个 Person 一份的监听器注册表
//
// 背景：监听器原本挂在 TypedAction<Derived, Listener> 的 `inline static` 成员上，
// 于是整张表是**进程级**的 —— 两个 Person 同时存活时，A 的 Buff 注册的回调会被
// B 触发的事件调用，互相激活形成无限递归（实测退出码 0xC00000FD 栈溢出），
// 也让 Buff::ID 被迫全局唯一。
//
// 这个类把表搬到 Person 实例上：key 是 Action 类型，value 是该 Action 的监听器 vector。
//
// 难点：C++ 容器是同质的，而每种 Action 的 vector 元素类型都不同
// （unique_ptr<DamageListener> / unique_ptr<EnergyListener> / ...）。
// 解法是类型擦除 —— 键里记类型，值里只存"一个对象"：
//
//     key   = std::type_index(typeid(ActionT))   // 运行期类型身份证（本质是个指针）
//     value = std::unique_ptr<IListenerSlot>     // 虚基类，删除逻辑类型无关
//
// 为什么这里**不能**用 shared_ptr<void>：
//   std::shared_ptr<void> 能工作是因为它把删除器存在控制块里（运行期）；
//   而 shared_ptr 可拷贝，一旦 ActionListeners 被拷贝，两个 Person 会静默共享
//   同一批 vector —— 正好是本次要修掉的 bug。unique_ptr 会让这种拷贝编译失败。
//   （unique_ptr<void> 则根本编不过：delete void* 非法。）
//
// ⚠️ 不变量：键为 typeid(ActionT) 的槽，内部一定是 ListenerSlot<ActionT::ListenerVector>。
//    这条由本类保证（只有 slotOf 会建槽，且只用模板参数决定的类型建），
//    编译器无法验证，所以取槽处用 static_cast 并在 debug 下配合断言。
// ============================================================================

// ---------------------------------------------------------------------------
// 调试期不变量：**派发某个 Action 期间，不允许 add/remove 同一 Action 的监听器**
//
// execute() 里遍历的是 `listOf<T>()` 返回的 vector。若某个回调在遍历过程中
// 注册/注销**同一类型** T 的监听器，push_back 可能重分配、erase 会挪动元素，
// 正在使用的迭代器立刻失效 → 未定义行为（可能不崩，只是结果错乱）。
//
// 这个不变量由 debug 断言守护：
//   · Person::triggerAction<T>() 进入时 enterDispatch<T>()、退出时 leaveDispatch<T>()
//   · add<T>() / remove<T>() 断言当前 T 的派发深度为 0
// Release 下这些代码全部被 #ifndef NDEBUG 去掉，零开销。
//
// 已实测（Debug）：射线黄金/并行、冰矛、beam_max、对比模式四条路径均不触发。
// 若将来有人写出"监听器在回调里给自己注册"的 Buff，Debug 会直接 abort，
// 而不是留一个只在特定分配顺序下错的幽灵 bug。
// ---------------------------------------------------------------------------

namespace detail
{
    /**
     * @brief 类型擦除后的槽基类
     *
     * 只暴露"按 buffID 删除"这一个类型无关的操作。
     * 遍历必须拿到具体类型（要调 ->callback(...)），所以不放在基类里。
     */
    class IListenerSlot
    {
    public:
        virtual ~IListenerSlot() = default;
        virtual void removeByBuffID(int buffID) = 0;
    };

    /**
     * @brief 具体槽：持有一个 Action 的监听器 vector
     */
    template <typename Vec>
    class ListenerSlot final : public IListenerSlot
    {
    public:
        Vec vec;

        void removeByBuffID(int buffID) override
        {
            // 语义与原 TypedAction::deleteListener 一致：只删**第一个**匹配项。
            // 若将来某个 Buff 对同一 Action 注册多次，需要改成 erase-remove 全删，
            // 但那会改变行为 → 必须用黄金基线确认数值不变。
            const auto it = std::find_if(
                vec.begin(), vec.end(),
                [buffID](const auto &item)
                {
                    return item && item->buffID == buffID;
                });

            if (it != vec.end())
            {
                vec.erase(it);
            }
        }
    };
} // namespace detail

class ActionListeners
{
public:
    ActionListeners() = default;

    // 独占所有权：禁止拷贝（拷贝会静默共享监听器表，正是要避免的事故）
    ActionListeners(const ActionListeners &) = delete;
    ActionListeners &operator=(const ActionListeners &) = delete;

    ActionListeners(ActionListeners &&) = default;
    ActionListeners &operator=(ActionListeners &&) = default;

    /**
     * @brief 取某一类 Action 的监听器 vector，没有就现建
     *
     * 返回值是 vector 的引用，在 ActionListeners 存活期间始终有效：
     * 槽本身在堆上（unique_ptr），unordered_map 扩容只重挂桶、不移动节点。
     *
     * ⚠️ 注意：这是"读并可能写"的操作 —— 即使只想查询也会建出一个空槽。
     */
    template <typename ActionT>
    auto &listOf()
    {
        using Vec = typename ActionT::ListenerVector;

        auto &slot = m_slots[std::type_index(typeid(ActionT))];
        if (!slot)
        {
            slot = std::make_unique<detail::ListenerSlot<Vec>>();
        }

        // 不变量检查：键与实际类型必须一致（见文件头说明）。
        // Release 下 static_cast 零成本；这里不做 dynamic_cast，避免热路径上的 vtable 查找。
        assert(dynamic_cast<detail::ListenerSlot<Vec> *>(slot.get()) != nullptr &&
               "ActionListeners: 槽的真实类型与 type_index 键不匹配");
        return static_cast<detail::ListenerSlot<Vec> *>(slot.get())->vec;
    }

    /**
     * @brief 注册一个监听器
     *
     * ⚠️ ActionT 必须显式指定（非推导上下文）：unique_ptr<DamageListener>
     *    无法反推出它属于哪个 Action。
     */
    template <typename ActionT>
    void add(typename ActionT::ListenerPtr listener)
    {
#ifndef NDEBUG
        // 派发 T 的过程中又注册 T 的监听器 → push_back 可能重分配，
        // 正在被遍历的 vector 迭代器失效（UB）
        const auto guardIt = m_dispatchDepth.find(std::type_index(typeid(ActionT)));
        assert((guardIt == m_dispatchDepth.end() || guardIt->second == 0) &&
               "add<T>() 在 T 的派发过程中被调用：迭代器可能失效");
#endif
        listOf<ActionT>().push_back(std::move(listener));
    }

    /**
     * @brief 删除属于某个 Buff 的监听器（对应原 TypedAction::deleteListener）
     */
    template <typename ActionT>
    void remove(int buffID)
    {
#ifndef NDEBUG
        // 派发 T 的过程中注销 T 的监听器 → erase 会让迭代器失效
        const auto guardIt = m_dispatchDepth.find(std::type_index(typeid(ActionT)));
        assert((guardIt == m_dispatchDepth.end() || guardIt->second == 0) &&
               "remove<T>() 在 T 的派发过程中被调用：迭代器可能失效");
#endif
        slotOf<ActionT>().removeByBuffID(buffID);
    }

private:
    /**
     * @brief 只取槽、不关心 vector 类型（供 remove 走虚函数）
     */
    template <typename ActionT>
    detail::IListenerSlot &slotOf()
    {
        using Vec = typename ActionT::ListenerVector;

        auto &slot = m_slots[std::type_index(typeid(ActionT))];
        if (!slot)
        {
            slot = std::make_unique<detail::ListenerSlot<Vec>>();
        }
        return *slot;
    }

public:
#ifndef NDEBUG
    /// 进入/离开某类 Action 的派发（供 add/remove 检查自修改）
    template <typename ActionT>
    void enterDispatch() { ++m_dispatchDepth[std::type_index(typeid(ActionT))]; }
    template <typename ActionT>
    void leaveDispatch()
    {
        // 只递减、不删除：避免每次 triggerAction 都 new/delete 一个 map 节点
        // （键最多几十个，常驻无害）
        const auto it = m_dispatchDepth.find(std::type_index(typeid(ActionT)));
        if (it != m_dispatchDepth.end()) --it->second;
    }
private:
#endif

    /// key = Action 类型，value = 该类型的监听器 vector（类型已擦除）
    /// ⚠️ 成员名不能叫 slots：Qt 的 QObject 把 slots 定义成了宏（#define slots），
    ///    在 GUI 目标里会被展开成空，导致 std::unordered_map<...> slots; 变成语法错误。
    std::unordered_map<std::type_index, std::unique_ptr<detail::IListenerSlot>> m_slots;

#ifndef NDEBUG
    std::unordered_map<std::type_index, int> m_dispatchDepth;   // 派发深度（检测"派发期间自我注册/注销"）
#endif
};
