#pragma once
#include <fstream>
#include <functional>
#include <string>

class Person;

// ============================================================================
// SimulationLog - 模拟调试日志输出
//
// 用法：
//   1. 在第一次模拟的 Person+Initializer 初始化后调用 begin(profession, person, params)，
//      会创建一个 log-<时间>-<职业>.txt，先写入配置头（Person构造参数/注册技能/注册Buff/
//      AutoAttack类），再把 Logger 的 debug 信息实时同步写入文件。
//   2. 该次模拟结束后调用 end() 恢复。
//   3. 多次模拟只取第一次（begin 内部用静态标志保证，第二次起返回 false）。
//
// 依赖：SkillCreator/BuffCreator 的注册表（需在 Initializer::Initialize() 之后调用），
//      以及 Logger 的 setLogCallback（现有接口，无需改 Logger）。
// ============================================================================
class SimulationLog
{
public:
    // 开始记录（仅首次模拟生效）。返回 true = 本次开始记录。
    static bool begin(const std::string& profession, const Person* person, const std::string& personParams);
    // 结束记录：清 Logger 回调、关闭文件
    static void end();
    // 是否正在记录
    static bool isActive();

private:
    static void writeLine(const std::string& line);

    static std::ofstream m_file;
    static bool m_started;   // 进程内是否已创建过（首次模拟专用）
    static bool m_active;    // 当前是否正在记录
    static std::function<void(const std::string&)> m_prevCallback;  // 保存的原回调（如GUI日志面板）
};
