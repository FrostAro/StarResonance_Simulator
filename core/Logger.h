#pragma once  // 防止头文件被多次包含

// ============================================================================
// 标准库头文件包含
// ============================================================================
#include <iostream>       // 标准输入输出流
#include <string>         // 字符串类
#include <mutex>          // 互斥锁（线程同步）
#include <chrono>         // 时间库（C++11）
#include <iomanip>        // 输入输出格式化
#include <sstream>        // 字符串流
#include <vector>         // 动态数组
#include <memory>         // 智能指针
#include <unordered_map>  // 哈希表
#include <queue>          // 队列
#include <condition_variable> // 条件变量（线程同步）
#include <thread>         // 线程库
#include <atomic>         // 原子操作
#include <type_traits>    // 类型特征
#include <functional>

/**
 * @class Logger
 * @brief 线程安全的C++日志库
 * 
 * 主要特性：
 * 1. 多级别日志（TRACE到CRITICAL）
 * 2. 线程安全（支持多线程同时写日志）
 * 3. 同步/异步模式可选
 * 4. ANSI颜色支持
 * 5. 可配置的输出格式
 * 6. 统计功能
 * 7. 条件日志记录
 * 8. 格式化字符串支持
 */
class Logger {
public:
    // ============================================================================
    // 日志级别枚举
    // ============================================================================
    
    /**
     * @enum Level
     * @brief 日志级别枚举
     * 
     * 定义6个日志级别，按严重程度递增排序：
     * TRACE < DEBUG < INFO < WARNING < ERROR < CRITICAL
     * 
     * 还有一个特殊级别NONE，用于关闭所有日志
     */
    enum class Level {
        TRACE,      // 最详细的日志，用于跟踪程序流程（开发调试用）
        DEBUG,      // 调试信息，用于开发阶段的调试
        INFO,       // 一般信息，程序正常运行时的关键信息
        WARNING,    // 警告信息，潜在问题但程序仍可运行
        ERROR,      // 错误信息，程序功能受影响
        CRITICAL,   // 严重错误，可能导致程序崩溃
        NONE        // 关闭所有日志输出
    };

    using LogCallback = std::function<void(const std::string&)>;

private:
    // ============================================================================
    // 颜色常量结构（ANSI转义码）
    // ============================================================================
    
    /**
     * @struct Colors
     * @brief ANSI颜色代码定义
     * 
     * 包含常用ANSI转义码，用于控制台彩色输出
     * 格式：\033[XXm（XX为颜色/样式代码）
     */
    struct Colors {
        static constexpr const char* RESET = "\033[0m";     // 重置所有属性
        static constexpr const char* BLACK = "\033[30m";    // 黑色
        static constexpr const char* RED = "\033[31m";      // 红色
        static constexpr const char* GREEN = "\033[32m";    // 绿色
        static constexpr const char* YELLOW = "\033[33m";   // 黄色
        static constexpr const char* BLUE = "\033[34m";     // 蓝色
        static constexpr const char* MAGENTA = "\033[35m";  // 洋红色
        static constexpr const char* CYAN = "\033[36m";     // 青色
        static constexpr const char* WHITE = "\033[37m";    // 白色
        static constexpr const char* BOLD = "\033[1m";      // 粗体
        static constexpr const char* UNDERLINE = "\033[4m"; // 下划线
    };

    // ============================================================================
    // 日志级别信息结构
    // ============================================================================
    
    /**
     * @struct LevelInfo
     * @brief 日志级别的详细信息
     * 
     * 包含每个日志级别的：
     * 1. 显示名称
     * 2. 颜色代码
     * 3. 严重程度数值（用于比较）
     */
    struct LevelInfo {
        std::string name;    // 级别名称（如 "DEBUG"）
        const char* color;   // 对应的颜色代码
        int severity;        // 严重程度数值，数字越大越严重
    };

    /**
     * @brief 日志级别信息映射表（静态成员）
     * 
     * 在Logger.cpp中初始化，将Level枚举映射到LevelInfo结构
     */
    static const std::unordered_map<Level, LevelInfo> levelInfoMap_;

    static LogCallback s_callback;
    static std::mutex s_callbackMutex; // 保护回调设置

public:
    // ============================================================================
    // 配置结构体
    // ============================================================================
    
    /**
     * @struct Config
     * @brief 日志系统配置
     * 
     * 包含所有可配置的日志选项，支持灵活的日志格式控制
     */
    struct Config {
        Level level;                // 当前日志级别（低于此级别的日志被过滤）
        bool useColors;             // 是否使用ANSI颜色
        bool showTimestamp;         // 是否显示时间戳
        bool showLevel;             // 是否显示日志级别
        bool showThreadId;          // 是否显示线程ID
        bool flushImmediately;      // 是否每次输出后立即刷新缓冲区
        std::string timestampFormat; // 时间戳格式（strftime格式）

        /**
         * @brief 默认构造函数
         * 
         * 提供合理的默认配置：
         * - 级别：INFO
         * - 启用颜色
         * - 显示时间戳和级别
         * - 不显示线程ID
         * - 不立即刷新（性能优化）
         * - 时间格式：年-月-日 时:分:秒
         */
        Config() :
            level(Level::INFO),
            useColors(true),
            showTimestamp(false),
            showLevel(true),
            showThreadId(false),
            flushImmediately(false),
            timestampFormat("%Y-%m-%d %H:%M:%S")
        {}

        /**
         * @brief 带参数的构造函数
         * 
         * @param lvl 日志级别
         * @param colors 是否使用颜色
         * @param timestamp 是否显示时间戳
         * @param showLvl 是否显示级别
         * @param threadId 是否显示线程ID
         * @param flush 是否立即刷新
         * @param timeFmt 时间戳格式
         */
        Config(Level lvl, bool colors = true, bool timestamp = false,
               bool showLvl = true, bool threadId = false, bool flush = false,
               const std::string& timeFmt = "%Y-%m-%d %H:%M:%S") :
            level(lvl), useColors(colors), showTimestamp(timestamp),
            showLevel(showLvl), showThreadId(threadId),
            flushImmediately(flush), timestampFormat(timeFmt)
        {}
    };

private:
    // ============================================================================
    // 单例模式实现
    // ============================================================================
    
    Logger() = default;                        // 私有构造函数
    Logger(const Logger&) = delete;            // 禁止拷贝构造
    Logger& operator=(const Logger&) = delete; // 禁止拷贝赋值

    // ============================================================================
    // 成员变量
    // ============================================================================
    
    Config config_;                       // 当前配置
    std::mutex mutex_;                    // 保护配置和输出的主互斥锁
    std::atomic<bool> initialized_{false}; // 初始化状态（原子操作）
    std::atomic<bool> shutdown_{false};   // 关闭标志（原子操作）
    
    // 异步日志相关成员
    std::unique_ptr<std::thread> asyncThread_; // 异步处理线程
    std::queue<std::string> asyncQueue_;       // 日志消息队列
    std::condition_variable asyncCondition_;   // 队列条件变量
    std::mutex asyncMutex_;                    // 队列互斥锁
    bool useAsync_{false};                     // 是否使用异步模式

    // ============================================================================
    // 私有成员函数声明
    // ============================================================================
    
    /**
     * @brief 内部日志记录核心函数
     * 
     * 根据同步/异步模式处理日志消息
     */
    void logInternal(Level level, const std::string& category, const std::string& message);
    
    /**
     * @brief 格式化日志消息
     * 
     * 根据配置添加时间戳、级别、颜色等
     */
    std::string formatMessage(Level level, const std::string& category, 
                             const std::string& message);
    
    /**
     * @brief 获取当前时间戳字符串
     */
    std::string getTimestamp() const;
    
    /**
     * @brief 获取当前线程ID字符串
     */
    std::string getThreadId() const;
    
    /**
     * @brief 异步日志处理线程函数
     */
    void asyncThreadFunction();
    
    /**
     * @brief 安全的字符串格式化函数（模板）
     */
    template<typename... Args>
    static std::string formatString(const std::string& format, Args... args);

    /**
     * @brief 获取单例实例
     * 
     * @return Logger& 全局唯一的Logger实例引用
     * 
     * @details 使用Meyer's Singleton模式，C++11保证线程安全
     */
    static Logger& instance() {
        static Logger instance;  // 第一次调用时构造
        return instance;
    }

public:
    // ============================================================================
    // 公开接口声明
    // ============================================================================
    
    ~Logger();  // 析构函数声明

    // ------------ 初始化与配置 ------------
    
    /**
     * @brief 使用完整配置初始化日志系统
     */
    static void initialize(const Config& config = Config());
    
    /**
     * @brief 使用指定级别初始化日志系统（简化版）
     */
    static void initialize(Level level = Level::INFO);
    
    /**
     * @brief 关闭日志系统
     */
    static void shutdown();
    
    /**
     * @brief 动态设置日志级别
     */
    static void setLevel(Level level);

    /**
     * @brief 获取当前日志级别
     */
    static Level getLevel();
    
    /**
     * @brief 设置立即刷新模式
     */
    static void setFlushImmediately(bool flush);
    
    /**
     * @brief 启用/禁用异步日志模式
     */
    static void enableAsync(bool enable = true);
    
    // ------------ 核心日志函数 ------------
    
    /**
     * @brief 通用日志记录函数（模板）
     * 
     * 支持可变参数和格式化字符串
     */
    template<typename... Args>
    static void log(Level level, const std::string& category, 
                   const std::string& format, Args... args);
    
    // ------------ 便捷日志函数 ------------
    
    /**
     * @brief TRACE级别日志
     */
    template<typename... Args>
    static void trace(const std::string& category, const std::string& format, 
                     Args... args);
    
    /**
     * @brief DEBUG级别日志
     */
    template<typename... Args>
    static void debug(const std::string& category, const std::string& format, 
                     Args... args);
    
    /**
     * @brief INFO级别日志
     */
    template<typename... Args>
    static void info(const std::string& category, const std::string& format, 
                    Args... args);
    
    /**
     * @brief WARNING级别日志
     */
    template<typename... Args>
    static void warning(const std::string& category, const std::string& format, 
                       Args... args);
    
    /**
     * @brief ERROR级别日志
     */
    template<typename... Args>
    static void error(const std::string& category, const std::string& format, 
                     Args... args);
    
    /**
     * @brief CRITICAL级别日志
     */
    template<typename... Args>
    static void critical(const std::string& category, const std::string& format, 
                        Args... args);
    
    // ------------ 兼容现有代码的特殊函数 ------------
    
    static void debugAutoAttack(double timer, const std::string& message);
    static void debugSkill(double timer, const std::string& skillName, 
                          const std::string& message);
    static void debugBuff(double timer, const std::string& buffName, 
                         const std::string& message);
    static void debugAction(double timer, const std::string& actionName, 
                           const std::string& message);
    static void debugError(double timer, const std::string& skillName, 
                          const std::string& errorType, const std::string& message);
    
    // ------------ 实用函数 ------------
    
    /**
     * @brief 强制刷新输出缓冲区
     */
    static void flush();
    
    /**
     * @brief 检查指定级别是否启用
     */
    static bool isEnabled(Level level);
    
    /**
     * @brief 级别枚举转字符串
     */
    static std::string levelToString(Level level);
    
    /**
     * @brief 字符串转级别枚举
     */
    static Level stringToLevel(const std::string& levelStr);
    
    // ------------ 统计信息 ------------
    
    /**
     * @struct Statistics
     * @brief 日志统计信息
     */
    struct Statistics {
        size_t traceCount = 0;    // TRACE级别计数
        size_t debugCount = 0;    // DEBUG级别计数
        size_t infoCount = 0;     // INFO级别计数
        size_t warningCount = 0;  // WARNING级别计数
        size_t errorCount = 0;    // ERROR级别计数
        size_t criticalCount = 0; // CRITICAL级别计数
        
        void reset();            // 重置所有计数器
        size_t total() const;    // 计算总日志数
    };
    
    static Statistics getStatistics();  // 获取当前统计
    static void resetStatistics();      // 重置统计
    
    // ------------ 条件日志 ------------
    
    /**
     * @brief 条件日志记录
     * 
     * 只有条件为true时才记录日志
     */
    template<typename... Args>
    static void logIf(bool condition, Level level, const std::string& category,
                     const std::string& format, Args... args);

    static void setLogCallback(LogCallback cb);
    static LogCallback getLogCallback();
};

// ============================================================================
// detail 命名空间 - 内部辅助函数声明
// ============================================================================

namespace detail {
    /**
     * @brief 通用类型转字符串
     * 
     * 使用std::ostringstream将任意类型转换为字符串
     */
    template<typename T>
    std::string toString(const T& value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }
    
    /**
     * @brief std::string特化
     * 
     * 直接返回字符串本身，避免不必要的转换
     */
    template<>
    inline std::string toString<std::string>(const std::string& value) {
        return value;
    }
    
    /**
     * @brief const char*特化
     * 
     * 处理C风格字符串，空指针转为"(null)"
     */
    template<>
    inline std::string toString<const char*>(const char* const& value) {
        return value ? std::string(value) : "(null)";
    }
    
    /**
     * @brief char*特化
     * 
     * 处理可变C风格字符串
     */
    template<>
    inline std::string toString<char*>(char* const& value) {
        return value ? std::string(value) : "(null)";
    }
    
    /**
     * @brief 替换字符串中的占位符
     * 
     * 实现细节见Logger.cpp
     */
    std::string replacePlaceholders(const std::string& format, 
                                   const std::vector<std::string>& args);
}

// ============================================================================
// 模板函数实现（必须在头文件中）
// ============================================================================

/**
 * @brief 安全的字符串格式化函数实现
 * 
 * 将可变参数转换为字符串并替换格式字符串中的占位符
 * 
 * @tparam Args 可变模板参数类型
 * @param format 格式字符串，包含 {0}, {1} 等占位符
 * @param args 要替换的参数
 * @return std::string 格式化后的字符串
 * 
 * @details 实现步骤：
 * 1. 使用展开表达式将每个参数转换为字符串
 * 2. 收集到字符串向量中
 * 3. 调用replacePlaceholders进行替换
 */
template<typename... Args>
std::string Logger::formatString(const std::string& format, Args... args) {
    // 使用展开表达式将每个参数转换为字符串
    // detail::toString(args)... 会为每个args调用toString
    std::vector<std::string> argStrings = { detail::toString(args)... };
    
    // 替换格式字符串中的占位符
    return detail::replacePlaceholders(format, argStrings);
}

/**
 * @brief 通用日志记录函数实现
 * 
 * 所有日志函数的底层实现，执行：
 * 1. 检查初始化状态
 * 2. 检查级别过滤
 * 3. 格式化消息
 * 4. 调用内部记录函数
 * 
 * @tparam Args 可变模板参数类型
 * @param level 日志级别
 * @param category 日志类别
 * @param format 格式字符串
 * @param args 格式化参数
 */
template<typename... Args>
void Logger::log(Level level, const std::string& category, 
                const std::string& format, Args... args) {
    Logger& logger = instance();  // 获取单例实例
    
    // 1. 检查日志系统状态
    if (!logger.initialized_ || logger.shutdown_) return;
    
    // 2. 级别过滤（跳过低于当前级别的日志）
    if (level < logger.config_.level) return;
    
    // 3. 安全的字符串格式化
    std::string formatted = formatString(format, args...);
    
    // 4. 调用内部实现记录日志
    logger.logInternal(level, category, formatted);
}

// ------------ 便捷日志函数实现 ------------

/**
 * @brief TRACE级别日志实现
 * 
 * 调用log函数，固定级别为Level::TRACE
 */
template<typename... Args>
void Logger::trace(const std::string& category, const std::string& format, 
                  Args... args) {
    log(Level::TRACE, category, format, args...);
}

/**
 * @brief DEBUG级别日志实现
 */
template<typename... Args>
void Logger::debug(const std::string& category, const std::string& format, 
                  Args... args) {
    log(Level::DEBUG, category, format, args...);
}

/**
 * @brief INFO级别日志实现
 */
template<typename... Args>
void Logger::info(const std::string& category, const std::string& format, 
                 Args... args) {
    log(Level::INFO, category, format, args...);
}

/**
 * @brief WARNING级别日志实现
 */
template<typename... Args>
void Logger::warning(const std::string& category, const std::string& format, 
                    Args... args) {
    log(Level::WARNING, category, format, args...);
}

/**
 * @brief ERROR级别日志实现
 */
template<typename... Args>
void Logger::error(const std::string& category, const std::string& format, 
                  Args... args) {
    log(Level::ERROR, category, format, args...);
}

/**
 * @brief CRITICAL级别日志实现
 */
template<typename... Args>
void Logger::critical(const std::string& category, const std::string& format, 
                     Args... args) {
    log(Level::CRITICAL, category, format, args...);
}

// ------------ 条件日志实现 ------------

/**
 * @brief 条件日志记录实现
 * 
 * 只有条件为true时才记录日志，避免不必要的参数计算
 * 
 * @tparam Args 可变模板参数类型
 * @param condition 记录条件
 * @param level 日志级别
 * @param category 日志类别
 * @param format 格式字符串
 * @param args 格式化参数
 */
template<typename... Args>
void Logger::logIf(bool condition, Level level, const std::string& category,
                  const std::string& format, Args... args) {
    if (condition) {
        log(level, category, format, args...);
    }
}