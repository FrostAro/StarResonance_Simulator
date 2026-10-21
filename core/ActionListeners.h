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
        listOf<ActionT>().push_back(std::move(listener));
    }

    /**
     * @brief 删除属于某个 Buff 的监听器（对应原 TypedAction::deleteListener）
     */
    template <typename ActionT>
    void remove(int buffID)
    {
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

    /// key = Action 类型，value = 该类型的监听器 vector（类型已擦除）
    /// ⚠️ 成员名不能叫 slots：Qt 的 QObject 把 slots 定义成了宏（#define slots），
    ///    在 GUI 目标里会被展开成空，导致 std::unordered_map<...> slots; 变成语法错误。
    std::unordered_map<std::type_index, std::unique_ptr<detail::IListenerSlot>> m_slots;
};
