#include "SimulationLog.h"
#include "Logger.h"
#include "Person.h"
#include "AutoAttack.h"
#include "creators.hpp"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <typeinfo>

#ifdef _WIN32
#include <windows.h>
#endif

std::ofstream SimulationLog::m_file;
bool SimulationLog::m_started = false;
bool SimulationLog::m_active = false;

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
    std::filesystem::create_directories(logBase + "/log");
    const std::string filename = logBase + "/log/log-" + currentTimestamp() + "-" + profession + ".txt";
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

    // 把 Logger 输出实时写入文件
    Logger::setLogCallback([](const std::string& msg) { SimulationLog::writeLine(msg); });
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
    Logger::setLogCallback(nullptr);  // 恢复（控制台原本无回调）
    m_file.flush();
    m_file.close();
    m_active = false;
}

bool SimulationLog::isActive()
{
    return m_active;
}
