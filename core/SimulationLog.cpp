#include "SimulationLog.h"
#include "Logger.h"
#include "Person.h"
#include "AutoAttack.h"
#include "creators.hpp"
#include <cerrno>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <typeinfo>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

std::ofstream SimulationLog::m_file;
bool SimulationLog::m_started = false;
bool SimulationLog::m_active = false;
std::function<void(const std::string&)> SimulationLog::m_prevCallback;

// 编译方式（Debug/Release），用于文件名
static std::string buildType()
{
#if defined(_DEBUG)
    return "Debug";
#elif defined(NDEBUG)
    return "Release";
#else
    return "Release";
#endif
}

// 获取 exe 所在目录（含结尾分隔符），用于把 log 固定到 exe 旁边，与启动目录无关
static std::string getExecutableDir()
{
#ifdef _WIN32
    char buf[MAX_PATH];
    const DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (len == 0)
        return "";
    std::string path(buf, len);
    const auto pos = path.find_last_of("\\/");
    return (pos == std::string::npos) ? "" : path.substr(0, pos + 1);
#else
    return "";  // 非Windows：退回当前目录
#endif
}

// 创建单级目录，已存在视为成功
static bool createSingleDirectory(const std::string& path)
{
#ifdef _WIN32
    return CreateDirectoryA(path.c_str(), nullptr) != 0 ||
           GetLastError() == ERROR_ALREADY_EXISTS;
#else
    return ::mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

// 递归创建多级目录（替代 std::filesystem::create_directories，
// GCC 8.1/MinGW 的 <filesystem> 头存在编译错误，且 MSVC 下也更轻量）
static void createDirectoriesRecursively(const std::string& path)
{
    if (path.empty())
        return;

    std::string prefix;
    for (size_t i = 0; i < path.size(); ++i)
    {
        const char ch = path[i];
        prefix += ch;

        if (ch == '/' || ch == '\\' || i + 1 == path.size())
        {
            // 跳过根目录与盘符根（如 "/"、"C:"、"C:/"）
            if (prefix == "/" || prefix == "\\")
                continue;
            if (prefix.size() == 2 && prefix[1] == ':')
                continue;
            if (prefix.size() == 3 && prefix[1] == ':' &&
                (prefix[2] == '/' || prefix[2] == '\\'))
                continue;

            createSingleDirectory(prefix);
        }
    }
}

// 生成时间戳：YYYYMMDD-HHMMSS（用于文件名）
static std::string currentTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_MSC_VER)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d-%H%M%S");
    return oss.str();
}

bool SimulationLog::begin(const std::string& profession, const Person* person, const std::string& personParams)
{
    if (m_started)
        return false;  // 多次模拟只取第一次

    m_started = true;

    // 统一写到【项目根目录】的 log/（CMake 传入 PROJECT_ROOT_PATH，Debug/Release/各构建目录一致）
#ifdef PROJECT_ROOT_PATH
    const std::string logBase = PROJECT_ROOT_PATH;
#else
    const std::string logBase = getExecutableDir();  // 未定义时退回 exe 目录
#endif
    createDirectoriesRecursively(logBase + "/log");
    // 命名：log-时间-入口(beam/icicle/gui)-编译方式(Debug/Release).txt
    const std::string filename = logBase + "/log/log-" + currentTimestamp() + "-" + profession + "-" + buildType() + ".txt";
    m_file.open(filename, std::ios::out | std::ios::trunc);
    if (!m_file.is_open())
        return false;

    // ===== 配置头（在 debug 输出之前）=====
    m_file << "==== 模拟配置 ====\n";
    m_file << "职业: " << profession << "\n";
    m_file << "Person 构造参数: " << personParams << "\n";

    // 注册的技能
    m_file << "注册技能: ";
    {
        const auto skills = SkillCreator::getSupportedTypes();
        for (size_t i = 0; i < skills.size(); ++i)
        {
            if (i > 0)
                m_file << ", ";
            m_file << skills[i];
        }
        m_file << "\n";
    }

    // 注册的Buff
    m_file << "注册Buff: ";
    {
        const auto buffs = BuffCreator::getSupportedTypes();
        for (size_t i = 0; i < buffs.size(); ++i)
        {
            if (i > 0)
                m_file << ", ";
            m_file << buffs[i];
        }
        m_file << "\n";
    }

    // 使用的AutoAttack类（typeid 名称含类名）
    if (person && person->getAutoAttack())
    {
        m_file << "AutoAttack: " << typeid(*person->getAutoAttack()).name() << "\n";
    }

    m_file << "==== 调试日志 ====\n";
    m_file.flush();

    // Logger 的 setLogCallback 只在异步模式下触发，故启用异步日志
    Logger::enableAsync(true);

    // 保存原回调（如GUI日志面板），组合：写文件 + 转发原回调，互不冲突
    m_prevCallback = Logger::getLogCallback();
    Logger::setLogCallback([](const std::string& msg)
    {
        SimulationLog::writeLine(msg);
        if (SimulationLog::m_prevCallback)
            SimulationLog::m_prevCallback(msg);
    });
    m_active = true;
    return true;
}

void SimulationLog::writeLine(const std::string& line)
{
    if (m_file.is_open())
    {
        m_file << line << "\n";
        m_file.flush();  // 同步写入
    }
}

void SimulationLog::end()
{
    if (!m_active)
        return;
    Logger::setLogCallback(m_prevCallback);  // 恢复原回调（控制台为 null，GUI 恢复面板回调）
    m_prevCallback = nullptr;
    m_file.flush();
    m_file.close();
    m_active = false;
}

void SimulationLog::reset()
{
    if (m_file.is_open())
    {
        m_file.flush();
        m_file.close();
    }
    m_started = false;
    m_active = false;
    m_prevCallback = nullptr;
}

bool SimulationLog::isActive()
{
    return m_active;
}
