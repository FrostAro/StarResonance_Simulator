#pragma once
#include <string>
#include <map>
#include <stdexcept>

class DamageInfo;
class Person;

// ============================================================================
// Buff 类声明
// ============================================================================

/**
 * @class Buff
 * @brief 游戏增益效果（Buff）基类
 * 
 * 主要特性：
 * 1. 实现游戏中需要长期存在的属性增益效果
 * 2. 支持层数堆叠和时间持续机制
 * 3. 可通过监听器回调函数响应游戏事件
 * 4. 支持去重和独立存在控制
 * 
 * 工作流程：
 * 1. 通过update()函数每tick更新状态
 * 2. 通过shouldBeRemoved()判断是否需要移除
 * 3. 通过监听器响应各种游戏事件
 * 
 * @note 这是一个抽象基类，需要子类实现具体的buff逻辑
 */
class Buff
{
protected:
    // ============================================================================
    // 静态成员变量
    // ============================================================================
    
    static std::string name;  // Buff类型名称（类级别）
    static int ID;            // 静态ID计数器（类级别）

    // ============================================================================
    // 实例成员变量
    // ============================================================================
    
    int buffID = 0;           // Buff唯一标识符（实例级别）
    
    double stack = 0;         // 当前层数
    double maxStack = 0;      // 最大可堆叠层数
    double lastStack = 0;     // 上一次更新的层数（用于变化检测）
    double number = 0;        // 数值效果（如攻击力增加量）
    double duration = 0;      // 剩余持续时间（毫秒）
    double maxDuration = 0;   // 最大持续时间（毫秒）
    
    bool isStackable = false;       // 是否可堆叠（层数叠加）
    bool allowDuplicates = false;   // 是否允许重复创建（独立存在）
    bool isInherent = false;        // 是否永久生效（如天赋buff）
    
    Person* p = nullptr;  // 所属的人物对象指针
    
public:
    friend Person;
    // ============================================================================
    // 构造与析构函数
    // ============================================================================
    
    /**
     * @brief 构造函数
     * 
     * @param p 所属的人物对象指针
     * @details 分配唯一的buffID并递增静态计数器
     */
    explicit Buff(Person* p);
    
    /**
     * @brief 虚析构函数
     * @details 确保派生类对象被正确销毁
     */
    virtual ~Buff() = default;
    
    // ============================================================================
    // 纯虚函数声明（需要子类实现）
    // ============================================================================
    
    /**
     * @brief 每tick更新buff逻辑
     * 
     * @param deltaTime 距离上次更新的时间间隔
     * @note 在游戏主循环中调用，用于处理buff的持续效果
     */
    virtual void update(double deltaTime) = 0;
    
    /**
     * @brief 判断buff是否应该被移除
     * 
     * @return bool true=应该移除，false=继续保留
     * @details 通常基于时间、层数或其他条件判断
     */
    virtual bool shouldBeRemoved();
    
    // ============================================================================
    // 公共成员函数
    // ============================================================================
    
    /**
     * @brief 获取buff名称
     * 
     * @return std::string buff名称
     */
    virtual std::string getBuffName() const;
    
    /**
     * @brief 添加层数
     * 
     * @param n 要添加的层数
     * @details 受最大层数限制，会保存上一次的层数
     */
    void addStack(double n);
    
    /**
     * @brief 重载等于操作符
     * 
     * @param other 要比较的另一个buff
     * @return bool true=相同（ID相等），false=不同
     * @details 通过buffID判断两个buff是否为同一个实例
     */
    bool operator==(const Buff& other) const;
    
    // ============================================================================
    // 静态成员函数
    // ============================================================================
    
    /**
     * @brief 获取下一个可用的buffID
     * 
     * @return int 下一个buffID
     * @note 主要用于构造函数中分配新ID
     */
    static int getID();
    
    // ============================================================================
    // 持续时间管理
    // ============================================================================
    
    /**
     * @brief 重置持续时间
     * @details 将剩余持续时间重置为最大持续时间
     */
    void resetDuration();
    
    /**
     * @brief 减少持续时间
     * 
     * @param deltaTime 要减少的时间量
     * @return bool true=成功减少，false=持续时间已无效
     */
    bool reduceDuration(int deltaTime);

    // 通过将duration设置为0来强行结束buff
    void stop();
    
    // ============================================================================
    // 访问器函数
    // ============================================================================
    
    int getBuffID() const;
    double getStack() const;
    double getMaxStack() const;
    double getLastStack() const;
    double getNumber() const;
    double getDuration() const;
    double getMaxDuration() const;
    bool getIsStackable() const;
    bool getAllowDuplicates() const;
    bool getIsInherent() const;
    
    // ============================================================================
    // 静态工具函数
    // ============================================================================
    
    /**
     * @brief 重置静态ID计数器
     * @details 用于重新开始计数（如重新开始游戏时）
     */
    static void resetID();
};

class Factor : public Buff     // 因子效果
{     
public:  
    Factor(Person* p) : Buff(p) {};
    virtual ~Factor() = default;
};

class RealFactor : public Buff     // 真实因子效果
{     
    double maxEnergy = 1200;
    double presentEnergy = 1200;
public:  
    void changeEnergy(double n)
    {
        this->presentEnergy += n;
        if (this->presentEnergy > this->maxEnergy)
        {
            this->presentEnergy = this->maxEnergy;
        }
        if(this->presentEnergy < 0)
        {
            this->presentEnergy = 0;
        }
    }

    double getMaxEnergy() const { return this->maxEnergy; }
    double getPresentEnergy() const { return this->presentEnergy; }

    RealFactor(Person* p) : Buff(p) {};
    virtual ~RealFactor() = default;
};