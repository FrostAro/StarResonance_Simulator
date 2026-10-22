#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

/**
 * @file ActionListeners.hpp
 * @brief 每个 Person 一份的监听器注册表
 *
 * @details 监听器原先挂在 TypedAction 的 inline static 成员上，整张表是进程级的：
 *          两个 Person 同时存活时，A 的 Buff 回调会被 B 的事件触发，互相激活形成
 *          无限递归（实测栈溢出 0xC00000FD），也迫使 Buff::ID 全局唯一。
 *          这里把表搬到 Person 实例上：键是 Action 类型，值是该类型的监听器 vector。
 *
 *          C++ 容器是同质的，而各 Action 的 vector 元素类型不同，所以用类型擦除：
 *          键存 std::type_index(typeid(ActionT))，值存 unique_ptr<IListenerSlot>，
 *          具体类型由 ListenerSlot<Vec> 提供，删除逻辑走虚函数。
 *
 * @note 不能用 shared_ptr<void>：它可拷贝，一旦 ActionListeners 被拷贝，两个 Person
 *       会静默共享同一批 vector，正好是要修的 bug。unique_ptr 让这种拷贝编译失败。
 *       （unique_ptr<void> 本身编不过：delete void* 非法。）
 *
 * @warning 不变量：键为 typeid(ActionT) 的槽，内部一定是 ListenerSlot<ActionT::ListenerVector>。
 *          只有 slotOf 会建槽且类型由模板参数决定，编译器无法验证，因此取槽处用
 *          static_cast 并在 debug 下配合断言。
 */

/**
 * @note 调试期不变量：派发某个 Action 期间不允许 add/remove 同一 Action 的监听器。
 *       execute() 遍历的就是 listOf<T>() 返回的 vector，遍历中注册会让 push_back
 *       重分配、注销会让 erase 挪动元素，迭代器立刻失效。由 Person::triggerAction<T>()
 *       记录派发深度、add/remove 断言深度为 0 来守护，Release 下零开销。
 */

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
            // 语义与原 TypedAction::deleteListener 一致：只删第一个匹配项。
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
