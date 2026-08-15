#include "Logger.h"
#include <algorithm>
#include <cctype>
#include <ctime>

// ============================================================================
// 静态成员初始化
// ============================================================================

/**
 * @brief 日志级别信息映射表的静态初始化
 * 
 * 为每个日志级别定义：
 * 1. 显示名称
 * 2. 控制台颜色（ANSI转义码）
 * 3. 严重程度值（用于级别比较）
 */
const std::unordered_map<Logger::Level, Logger::LevelInfo> Logger::levelInfoMap_ = {
    {Logger::Level::TRACE,    {"TRACE",    Colors::CYAN,   0}},  // 青色，严重程度0
    {Logger::Level::DEBUG,    {"DEBUG",    Colors::GREEN,  1}},  // 绿色，严重程度1
    {Logger::Level::INFO,     {"INFO",     Colors::BLUE,   2}},  // 蓝色，严重程度2
    {Logger::Level::WARNING,  {"WARNING",  Colors::YELLOW, 3}},  // 黄色，严重程度3
    {Logger::Level::ERROR,    {"ERROR",    Colors::RED,    4}},  // 红色，严重程度4
    {Logger::Level::CRITICAL, {"CRITICAL", Colors::MAGENTA,5}}   // 洋红色，严重程度5
};

Logger::LogCallback Logger::s_callback = nullptr;
std::mutex Logger::s_callbackMutex;

// ============================================================================
// detail 命名空间 - 内部辅助函数
// ============================================================================

namespace detail {
    /**
     * @brief 替换字符串中的占位符
     * 
     * 将格式字符串中的 {0}, {1}, {2} 等占位符替换为对应的参数值
     * 支持重复使用同一占位符，例如 "{0} and {0}" 会被替换两次
     * 
     * @param format 原始格式字符串，包含占位符
     * @param args 要替换的参数值向量
     * @return std::string 替换后的完整字符串
     * 
     * @details 算法流程：
     * 1. 复制输入格式字符串到结果变量
     * 2. 遍历所有参数（从0开始索引）
     * 3. 为每个参数构建占位符字符串：{i}
     * 4. 在结果字符串中查找该占位符
     * 5. 使用 while 循环替换所有出现的该占位符
     *    （支持同一占位符在字符串中出现多次）
     */
    std::string replacePlaceholders(const std::string& format, 
                                   const std::vector<std::string>& args) {
        std::string result = format;  // 创建副本以便修改
        
        // 遍历所有参数
        for (size_t i = 0; i < args.size(); ++i) {
            // 构建占位符字符串，如 "{0}", "{1}" 等
            std::string placeholder = "{" + std::to_string(i) + "}";
            size_t pos = result.find(placeholder);
            
            // 替换所有出现的该占位符
            while (pos != std::string::npos) {
                // 用实际参数值替换占位符
                result.replace(pos, placeholder.length(), args[i]);
                // 从替换后的位置继续搜索（避免无限循环）
                pos = result.find(placeholder, pos + args[i].length());
            }
        }
        
        return result;
    }
}

// ============================================================================
// 析构函数
// ============================================================================

/**
 * @brief Logger 类析构函数
 * 
 * 确保在对象销毁时正确关闭日志系统：
 * 1. 停止异步日志线程
 * 2. 刷新所有待处理的日志消息
 * 3. 清理资源
 * 
 * @details 调用 shutdown() 执行清理工作
 */
Logger::~Logger() {
    shutdown();  // 确保资源被正确释放
}

// ============================================================================
// 初始化与配置函数
// ============================================================================

/**
 * @brief 初始化日志系统（完全配置版本）
 * 
 * 使用完整的配置对象初始化日志系统：
 * 1. 获取单例实例
 * 2. 加锁保证线程安全
 * 3. 如果已经初始化，先关闭再重新初始化
 * 4. 复制配置
 * 5. 根据配置启用异步日志
 * 6. 标记为已初始化
 * 7. 输出初始化信息日志
 * 
 * @param config 日志配置对象，包含所有可配置项
 * 
 * @note 这是静态函数，通过单例模式操作全局唯一的日志实例
 */
void Logger::initialize(const Config& config) {
    // 获取单例实例
    Logger& logger = instance();
    
    // 加锁保护配置修改
    std::lock_guard<std::mutex> lock(logger.mutex_);
    
    // 如果已经初始化，先关闭（允许重新初始化）
    if (logger.initialized_) {
        shutdown();
    }
    
    // 保存配置
    logger.config_ = config;
    
    // 根据配置启用异步日志
    if (logger.useAsync_) {
        logger.enableAsync(true);
    }
    
    // 标记为已初始化
    logger.initialized_ = true;
    
    // 输出初始化成功的日志
    logger.info("Logger", "Logger initialized. Level: {0}", levelToString(config.level));
}

/**
 * @brief 初始化日志系统（简化版本）
 * 
 * 使用默认配置，只指定日志级别：
 * 1. 创建默认配置对象
 * 2. 设置指定的日志级别
 * 3. 调用完全版初始化函数
 * 
 * @param level 初始日志级别
 */
void Logger::initialize(Level level) {
    Config config(level);  // 使用指定级别创建默认配置
    initialize(config);     // 调用完全版初始化
}

/**
 * @brief 关闭日志系统
 * 
 * 安全地关闭日志系统：
 * 1. 设置关闭标志
 * 2. 停止异步日志线程（如果启用）
 * 3. 等待异步线程结束
 * 4. 清理资源
 * 5. 重置初始化状态
 * 
 * @details 这是优雅关闭的关键函数，确保所有日志消息都被处理
 */
void Logger::shutdown() {
    Logger& logger = instance();
    
    // 如果未初始化，直接返回
    if (!logger.initialized_) return;
    
    // 设置关闭标志
    logger.shutdown_ = true;
    
    // 停止异步日志线程
    if (logger.useAsync_) {
        // 清除异步标志
        logger.useAsync_ = false;
        
        // 通知等待中的异步线程
        logger.asyncCondition_.notify_all();
        
        // 等待异步线程安全结束
        if (logger.asyncThread_ && logger.asyncThread_->joinable()) {
            logger.asyncThread_->join();
        }
    }
    
    // 重置初始化状态
    logger.initialized_ = false;
}

/**
 * @brief 动态设置日志级别
 * 
 * 允许在运行时修改日志过滤级别：
 * 1. 加锁保证线程安全
 * 2. 更新配置中的日志级别
 * 3. 记录级别变更日志（使用debug级别）
 * 
 * @param level 新的日志级别
 * 
 * @note 使用debug级别记录变更，因此如果当前级别高于DEBUG，变更不会被记录
 */
void Logger::setLevel(Level level) {
    Logger& logger = instance();
    std::lock_guard<std::mutex> lock(logger.mutex_);

    // 更新配置
    logger.config_.level = level;

    // 记录级别变更（使用debug级别）
    logger.debug("Logger", "Log level changed to: {0}", levelToString(level));
}

Logger::Level Logger::getLevel() {
    Logger& logger = instance();
    std::lock_guard<std::mutex> lock(logger.mutex_);
    return logger.config_.level;
}

/**
 * @brief 设置立即刷新模式
 * 
 * 控制是否每次输出日志后立即刷新缓冲区：
 * 1. 加锁保证线程安全
 * 2. 更新配置中的flushImmediately标志
 * 
 * @param flush true=立即刷新，false=缓冲输出
 * 
 * @note 立即刷新会影响性能但能确保日志实时可见
 */
void Logger::setFlushImmediately(bool flush) {
    Logger& logger = instance();
    std::lock_guard<std::mutex> lock(logger.mutex_);
    logger.config_.flushImmediately = flush;
}

/**
 * @brief 启用/禁用异步日志
 * 
 * 切换同步和异步日志模式：
 * 1. 加锁保证线程安全
 * 2. 如果从同步切换到异步：启动后台线程
 * 3. 如果从异步切换到同步：停止后台线程
 * 
 * @param enable true=启用异步，false=使用同步
 * 
 * @details 异步模式将日志消息放入队列，由专用线程处理
 *          可以提高性能，避免日志输出阻塞主线程
 */
void Logger::enableAsync(bool enable) {
    Logger& logger = instance();
    std::lock_guard<std::mutex> lock(logger.mutex_);
    
    // 启用异步日志
    if (enable && !logger.useAsync_) {
        logger.useAsync_ = true;
        logger.shutdown_ = false;  // 确保关闭标志清除
        
        // 创建异步处理线程
        logger.asyncThread_ = std::make_unique<std::thread>(
            [&logger]() { logger.asyncThreadFunction(); });
    }
    // 禁用异步日志
    else if (!enable && logger.useAsync_) {
        logger.useAsync_ = false;
        
        // 通知异步线程停止
        logger.asyncCondition_.notify_all();
        
        // 等待异步线程结束
        if (logger.asyncThread_ && logger.asyncThread_->joinable()) {
            logger.asyncThread_->join();
        }
    }
}

// ============================================================================
// 核心日志处理函数
// ============================================================================

/**
 * @brief 内部日志记录函数（核心实现）
 * 
 * 根据配置决定使用同步或异步方式记录日志：
 * 1. 异步模式：格式化消息后放入队列，通知异步线程处理
 * 2. 同步模式：直接格式化并输出到控制台
 * 
 * @param level 日志级别
 * @param category 日志类别/模块名
 * @param message 原始日志消息（已格式化）
 * 
 * @note 这是所有日志函数的最终调用点
 */
void Logger::logInternal(Level level, const std::string& category, 
                        const std::string& message) {
    // 异步处理模式
    if (useAsync_) {
        // 1. 格式化消息（包含时间戳、级别等）
        std::string formatted = formatMessage(level, category, message);

        // 2.调用回调（如果有）
        {
            std::lock_guard<std::mutex> lock(s_callbackMutex);
            if (s_callback) {
                s_callback(formatted);
            }
        }
        
        // 3. 加锁保护队列，添加消息
        {
            std::lock_guard<std::mutex> lock(asyncMutex_);
            asyncQueue_.push(std::move(formatted));  // 使用move避免复制
        }
        
        // 4. 通知异步线程有新消息
        asyncCondition_.notify_one();
    }
    // 同步处理模式
    else {
        // 1. 直接格式化消息
        std::string formatted = formatMessage(level, category, message);
        
        // 2. 输出到标准输出
        std::cout << formatted << std::endl;
        
        // 3. 如果需要，立即刷新缓冲区
        if (config_.flushImmediately) {
            std::cout.flush();
        }
    }
}

/**
 * @brief 格式化日志消息
 * 
 * 根据配置将原始消息格式化为完整日志行：
 * 1. 时间戳（可选）
 * 2. 线程ID（可选）
 * 3. 日志级别（带颜色，可选）
 * 4. 类别（带颜色，可选）
 * 5. 原始消息
 * 
 * @param level 日志级别
 * @param category 日志类别
 * @param message 原始消息
 * @return std::string 格式化后的完整日志行
 */
std::string Logger::formatMessage(Level level, const std::string& category, 
                                 const std::string& message) {
    std::ostringstream oss;  // 使用字符串流构建输出
    
    // 1. 查找日志级别信息
    auto it = levelInfoMap_.find(level);
    if (it == levelInfoMap_.end()) {
        // 如果未找到，使用INFO作为默认
        it = levelInfoMap_.find(Level::INFO);
    }
    
    const LevelInfo& info = it->second;
    
    // 2. 时间戳部分
    if (config_.showTimestamp) {
        oss << "[" << getTimestamp() << "] ";
    }
    
    // 3. 线程ID部分
    if (config_.showThreadId) {
        oss << "[" << getThreadId() << "] ";
    }
    
    // 4. 日志级别部分
    if (config_.showLevel) {
        if (config_.useColors) {
            // 带颜色的级别显示
            oss << info.color << "[" << info.name << "]" << Colors::RESET << " ";
        } else {
            // 不带颜色的级别显示
            oss << "[" << info.name << "] ";
        }
    }
    
    // 5. 类别部分
    if (!category.empty()) {
        if (config_.useColors) {
            // 加粗显示的类别名
            oss << Colors::BOLD << category << Colors::RESET << ": ";
        } else {
            oss << category << ": ";
        }
    }
    
    // 6. 消息部分
    oss << message;
    
    return oss.str();
}

/**
 * @brief 获取当前时间戳字符串
 * 
 * 生成格式化的时间戳，包含毫秒精度：
 * 1. 获取当前系统时间
 * 2. 转换为本地时间
 * 3. 格式化为字符串
 * 4. 添加毫秒部分
 * 
 * @return std::string 格式化的时间戳字符串
 * 
 * @details 使用线程安全的平台特定API（localtime_r/localtime_s）
 */
std::string Logger::getTimestamp() const {
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    
    // 转换为time_t（秒精度）
    auto time = std::chrono::system_clock::to_time_t(now);
    
    // 计算毫秒部分
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    // 转换为本地时间结构体（平台兼容）
    std::tm tm;
#if defined(_WIN32)
    localtime_s(&tm, &time);  // Windows安全版本
#else
    localtime_r(&time, &tm);  // POSIX安全版本
#endif
    
    // 格式化时间部分
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), config_.timestampFormat.c_str(), &tm);
    
    // 构建完整时间戳（秒.毫秒）
    std::ostringstream oss;
    oss << buffer << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

/**
 * @brief 获取当前线程ID字符串
 * 
 * 将线程ID转换为可读的字符串格式
 * 
 * @return std::string 线程ID的字符串表示
 */
std::string Logger::getThreadId() const {
    std::ostringstream oss;
    oss << std::this_thread::get_id();  // 使用标准库获取线程ID
    return oss.str();
}

/**
 * @brief 异步日志处理线程函数
 * 
 * 后台线程的主函数，负责：
 * 1. 从队列中获取日志消息
 * 2. 批量处理消息（提高性能）
 * 3. 输出到控制台
 * 4. 按需刷新缓冲区
 * 
 * @details 使用条件变量等待新消息，避免忙等待
 *          批量处理减少锁竞争和系统调用
 */
void Logger::asyncThreadFunction() {
    // 主循环：继续运行直到禁用异步且队列为空
    while (useAsync_ || !asyncQueue_.empty()) {
        std::vector<std::string> messages;  // 批量处理容器
        
        {
            // 1. 获取队列锁
            std::unique_lock<std::mutex> lock(asyncMutex_);
            
            // 2. 如果队列为空且异步模式仍启用，则等待新消息
            if (asyncQueue_.empty() && useAsync_) {
                // 条件变量等待：有新消息或异步被禁用时唤醒
                asyncCondition_.wait(lock, [this]() { 
                    return !asyncQueue_.empty() || !useAsync_; 
                });
            }
            
            // 3. 批量取出消息（最多100条，减少锁竞争）
            while (!asyncQueue_.empty() && messages.size() < 100) {
                messages.push_back(std::move(asyncQueue_.front()));  // 移动语义避免复制
                asyncQueue_.pop();
            }
        }  // 锁在此作用域结束时释放
        
        // 4. 批量输出消息（不需要队列锁，但需要配置锁）
        if (!messages.empty()) {
            std::lock_guard<std::mutex> lock(mutex_);  // 保护配置访问
            for (const auto& msg : messages) {
                std::cout << msg << std::endl;
            }
            
            // 5. 按配置刷新缓冲区
            if (config_.flushImmediately) {
                std::cout.flush();
            }
        }
    }
}

// ============================================================================
// 兼容现有代码的特殊函数
// ============================================================================

/**
 * @brief 自动攻击调试日志（特殊格式）
 * 
 * 为游戏/自动化系统中的自动攻击提供特定格式的调试日志
 * 
 * @param timer 计时器值
 * @param message 调试消息
 */
void Logger::debugAutoAttack(double timer, const std::string& message) {
    debug("Auto  ", "[DEBUG,timer={0}]: Auto    - {1}", timer, message);
}

/**
 * @brief 技能调试日志（特殊格式）
 * 
 * 为游戏技能系统提供特定格式的调试日志
 * 
 * @param timer 计时器值
 * @param skillName 技能名称
 * @param message 调试消息
 */
void Logger::debugSkill(double timer, const std::string& skillName, 
                       const std::string& message) {
    debug("Skill ", "[DEBUG,timer={0}]: Skill   - skill: {1} - {2}", 
          timer, skillName, message);
}

/**
 * @brief 增益效果调试日志（特殊格式）
 * 
 * 为游戏增益效果系统提供特定格式的调试日志
 * 
 * @param timer 计时器值
 * @param buffName 增益效果名称
 * @param message 调试消息
 */
void Logger::debugBuff(double timer, const std::string& buffName, 
                      const std::string& message) {
    debug("Buff  ", "[DEBUG,timer={0}]: Buff    - buff: {1} - {2}", 
          timer, buffName, message);
}

/**
 * @brief 动作调试日志（特殊格式）
 * 
 * 为游戏动作系统提供特定格式的调试日志
 * 
 * @param timer 计时器值
 * @param actionName 动作名称
 * @param message 调试消息
 */
void Logger::debugAction(double timer, const std::string& actionName, 
                        const std::string& message) {
    debug("Action", "[DEBUG,timer={0}]: Action  - {1} - {2}", 
          timer, actionName, message);
}

/**
 * @brief 错误调试日志（特殊格式）
 * 
 * 为技能系统中的错误提供特定格式的调试日志
 * 
 * @param timer 计时器值
 * @param skillName 技能名称
 * @param errorType 错误类型
 * @param message 错误消息
 */
void Logger::debugError(double timer, const std::string& skillName, 
                       const std::string& errorType, const std::string& message) {
    debug("Error", "[DEBUG,timer={0}]: Error   - skill: {1} - {2}: {3}", 
          timer, skillName, errorType, message);
}

// ============================================================================
// 实用函数
// ============================================================================

/**
 * @brief 强制刷新输出缓冲区
 * 
 * 立即将缓冲区中的所有数据写入输出设备
 */
void Logger::flush() {
    std::cout.flush();
}

/**
 * @brief 检查指定日志级别是否启用
 * 
 * 根据当前配置检查特定级别的日志是否会被输出：
 * 1. 检查日志系统是否已初始化
 * 2. 检查指定级别是否不低于当前配置级别
 * 
 * @param level 要检查的日志级别
 * @return bool true=该级别日志会被输出，false=被过滤
 */
bool Logger::isEnabled(Level level) {
    Logger& logger = instance();
    return logger.initialized_ && level >= logger.config_.level;
}

/**
 * @brief 将日志级别枚举转换为字符串
 * 
 * @param level 日志级别枚举值
 * @return std::string 对应的字符串表示
 */
std::string Logger::levelToString(Level level) {
    auto it = levelInfoMap_.find(level);
    return it != levelInfoMap_.end() ? it->second.name : "UNKNOWN";
}

/**
 * @brief 将字符串转换为日志级别枚举
 * 
 * 支持多种输入格式：
 * 1. 精确匹配（不区分大小写）
 * 2. 部分匹配（如 "WARN" 匹配 WARNING）
 * 
 * @param levelStr 级别字符串
 * @return Level 对应的日志级别枚举值，默认为 INFO
 */
Logger::Level Logger::stringToLevel(const std::string& levelStr) {
    // 转换为大写以进行不区分大小写的比较
    std::string upperStr = levelStr;
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);
    
    // 1. 尝试精确匹配
    for (const auto& pair : levelInfoMap_) {
        if (pair.second.name == upperStr) {
            return pair.first;
        }
    }
    
    // 2. 尝试部分匹配（容错处理）
    if (upperStr.find("TRACE") != std::string::npos) return Level::TRACE;
    if (upperStr.find("DEBUG") != std::string::npos) return Level::DEBUG;
    if (upperStr.find("INFO") != std::string::npos) return Level::INFO;
    if (upperStr.find("WARN") != std::string::npos) return Level::WARNING;
    if (upperStr.find("ERROR") != std::string::npos) return Level::ERROR;
    if (upperStr.find("CRIT") != std::string::npos) return Level::CRITICAL;
    
    // 3. 默认值
    return Level::INFO;
}

/**
 * @brief 重置统计信息
 * 
 * 将所有级别的计数器清零
 */
void Logger::Statistics::reset() {
    traceCount = 0;
    debugCount = 0;
    infoCount = 0;
    warningCount = 0;
    errorCount = 0;
    criticalCount = 0;
}

/**
 * @brief 计算总日志数量
 * 
 * 汇总所有级别的日志数量
 * 
 * @return size_t 总日志数
 */
size_t Logger::Statistics::total() const {
    return traceCount + debugCount + infoCount + warningCount + 
           errorCount + criticalCount;
}

/**
 * @brief 获取当前统计信息
 * 
 * 注意：当前简化实现返回空统计，实际使用时应维护全局计数器
 * 
 * @return Statistics 日志统计信息
 */
Logger::Statistics Logger::getStatistics() {
    // 简化实现：返回空的统计信息
    // 实际实现应该在logInternal中更新计数器
    return Statistics();
}

/**
 * @brief 重置全局统计信息
 * 
 * 注意：当前简化实现无操作，实际使用时应重置全局计数器
 */
void Logger::resetStatistics() {
    // 简化实现：无操作
    // 实际实现应该重置全局统计计数器
}

void Logger::setLogCallback(LogCallback cb) {
    std::lock_guard<std::mutex> lock(s_callbackMutex);
    s_callback = cb;
}

Logger::LogCallback Logger::getLogCallback() {
    std::lock_guard<std::mutex> lock(s_callbackMutex);
    return s_callback;
}