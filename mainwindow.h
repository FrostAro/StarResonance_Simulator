#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QVariant>
#include <QThread>
#include <QMainWindow>
#include <QTextEdit>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QElapsedTimer>
#include <QTimer>
#include <atomic>
#include <memory>
#include "core/Person.h"
#include "core/AutoAttack.h"
#include "core/Comparison.h"

// 在 MainWindow 类前定义幻想配置枚举
enum class FantasyBeamConfig {
    MukuChief_MukuScout = 0,
    MukuChief_YGLWS,
    MukuChief_SXMQ,
    SXMQ_HYXZ,
    SXMQ_MukuScout
};

// ============================================================================
// 模拟循环实现

class QAction;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class SimulationWorker : public QObject
{
    Q_OBJECT
public:
        explicit SimulationWorker(const QString& profession,
                              double primaryAttr,
                              double crit,
                              double quickness,
                              double lucky,
                              double proficient,
                              double almighty,
                              int atk,
                              int refineAtk,
                              int elementAtk,
                              double attackSpeed,
                              double castingSpeed,
                              double critDmgSet,
                              double incSet,
                              double eleIncSet,
                              int times,
                              int maxTime,
                              int deltaTime,
                              bool randomSeed,
                              uint32_t seed,
                              int fantasyConfig,
                              int flowConfig,
                              int workerCount,
                              const WeaponConfig& weapon);
    ~SimulationWorker(); 
    /// 请求取消（可从 GUI 线程调用；内部是原子写）
    void requestCancel() { m_cancel.store(true, std::memory_order_relaxed); }
public slots:
    void run();  // 在子线程中执行模拟循环
signals:
    void logMessage(const QString& msg);
    void simulationFinished(const QVector<QVector<QVariant>>& stats, int totalTime);
private:
    QString m_profession;
    double m_primaryAttr;
    double m_crit;
    double m_quickness;
    double m_lucky;
    double m_proficient;
    double m_almighty;
    int m_atk;
    int m_refineAtk;
    int m_elementAtk;
    double m_attackSpeed;
    double m_castingSpeed;
    double m_critDmgSet;
    double m_incSet;
    double m_eleIncSet;
    int m_times;
    int m_maxTime;
    int m_deltaTime;
    bool m_randomSeed;
    uint32_t m_seed;
    int m_fantasyConfig;  // 保存幻想配置索引
    int m_flowConfig;     // 保存流派配置：0=急速精通流，1=幸运流
    int m_workerCount;    // 并行线程数（times>1 时生效）
    WeaponConfig m_weapon;    // 武器第一/第二属性（与 CLI 同口径）
    std::atomic<bool> m_cancel{false};   // 取消标志（times>1 时被多个工作线程并发读）
};

/**
 * @brief 配对对比工作对象：在独立 QThread 里跑完整一轮 CRN 对比
 *
 * @details 生命周期：构造 → moveToThread(m_compareThread) → QThread::started 触发
 *          run() → 末尾 emit comparisonFinished（连接到 QThread::quit）→
 *          QThread::finished 时 deleteLater 回收。
 *
 * @note run() 只在 m_compareThread 上执行，GUI 侧不要直接调用；对 UI 的反馈一律
 *       通过 signal（Qt 默认 QueuedConnection，跨线程安全）。
 *
 * @warning 取消必须用本类自己的 std::atomic<bool>（requestCancel）。
 *          不能用 QThread::currentThread()->isInterruptionRequested()：
 *          内部线程池里的工作线程不是 Qt 线程，那个调用在那里恒为 false，
 *          取消会静默失效。
 */
class ComparisonWorker : public QObject
{
    Q_OBJECT
public:
    /**
     * @param base        基准配置（当前输入面板）
     * @param candidates  候选配置列表，每行一个（由候选文本框解析得到）
     * @param pairs       配对数 = 使用的种子个数（建议 20+）
     * @param seed        起始种子，实际使用 seed..seed+pairs-1
     * @param maxTime     每次模拟时长（毫秒）
     * @param deltaTime   模拟步长（毫秒）
     * @param workerCount 并行线程数，1 = 串行（结果与并行逐位相同）
     * @param simulate    单次模拟函数（射线/冰矛二选一，由调用方决定）
     */
    explicit ComparisonWorker(const SimConfig& base,
                              std::vector<SimConfig> candidates,
                              int pairs,
                              std::uint32_t seed,
                              int maxTime,
                              int deltaTime,
                              int workerCount,
                              SimulateFn simulate)
        : m_base(base), m_candidates(std::move(candidates)), m_pairs(pairs),
          m_seed(seed), m_maxTime(maxTime), m_deltaTime(deltaTime),
          m_workerCount(workerCount), m_simulate(std::move(simulate)) {}
    ~ComparisonWorker();

    /// 请求取消（可从 GUI 线程调用；内部是原子写）
    void requestCancel() { m_cancel.store(true, std::memory_order_relaxed); }

    /// 本次使用的并行线程数（状态栏显示用）
    int workerCount() const { return m_workerCount; }

public slots:
    void run();  // 在子线程中执行配对对比

signals:
    void logMessage(const QString& msg);  // 进度提示（一行文字）
    /// 每个候选一行结果；finished = false 表示被取消（rows 只含已完成的候选）
    void comparisonFinished(const QVector<QVector<QVariant>>& rows, int pairs, bool finished);

private:
    SimConfig m_base;                       // 基准配置
    std::vector<SimConfig> m_candidates;    // 候选配置（每个只与基准差一项）
    int m_pairs;                            // 配对数（种子个数）
    std::uint32_t m_seed;                   // 起始种子
    int m_maxTime;                          // 每次模拟时长（毫秒）
    int m_deltaTime;                        // 步长（毫秒）
    int m_workerCount;                      // 并行线程数
    std::atomic<bool> m_cancel{false};      // 取消标志（多线程并发读）
    SimulateFn m_simulate;                  // 职业相关的单次模拟函数
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onRunClicked();
    void onProfessionChanged(int index);
    void appendLog(const QString& msg);
    void onSimulationFinished(const QVector<QVector<QVariant>>& stats, int totalTime);
    void onComparisonFinished(const QVector<QVector<QVariant>>& rows, int pairs, bool finished);

private:
    void setupUI();
    void updateDefaultsForProfession(const QString& prof);
    QWidget* createInputPanel();
    QWidget* createDebugPanel();
    QWidget* createResultPanel();
    void applyTheme(bool dark);  // 切换明/暗主题

    // 中断并回收仍在运行的模拟/对比线程（析构与重跑前调用）
    void stopWorkerThreads();

    // 对比模式：从界面输入构建基准配置 / 解析候选配置
    SimConfig buildBaseConfig();
    std::vector<SimConfig> parseCandidates(const SimConfig& base, const QString& text);

    // 从「武器」输入组读取武器第一/第二属性（GUI 与 CLI 共用同一套口径）
    WeaponConfig buildWeaponConfig() const;

    // 输入控件
    QComboBox* m_professionCombo;
    QComboBox* m_fantasyCombo;  // 新增幻想配置下拉框
    QComboBox* m_weaponFirstCombo;   // 武器第一属性
    QComboBox* m_weaponSecondCombo;  // 武器第二属性
    QLineEdit* m_weaponEnhanceEdit;  // 武器增效
    QLineEdit* m_weaponExtraEdit;    // 武器额外攻击加成
    QLineEdit* m_primaryAttrEdit;
    QLineEdit* m_critEdit;
    QLineEdit* m_quicknessEdit;
    QLineEdit* m_luckyEdit;
    QLineEdit* m_proficientEdit;
    QLineEdit* m_almightyEdit;
    QLineEdit* m_atkEdit;
    QLineEdit* m_refineAtkEdit;
    QLineEdit* m_elementAtkEdit;
    QLineEdit* m_attackSpeedEdit;
    QLineEdit* m_castingSpeedEdit;
    QLineEdit* m_critDmgSetEdit;
    QLineEdit* m_incSetEdit;
    QLineEdit* m_eleIncSetEdit;
    QLineEdit* m_timesEdit;
    QLineEdit* m_threadsEdit;   // 非对比模式的并行线程数（times>1 时生效）
    QLineEdit* m_maxTimeEdit;
    QLineEdit* m_deltaTimeEdit;
    QCheckBox* m_randomSeedCheck;
    QLineEdit* m_seedEdit;
    QPushButton* m_runButton;

    // 对比模式控件
    QCheckBox* m_compareCheck;
    QLineEdit* m_comparePairsEdit;
    QLineEdit* m_compareThreadsEdit;   // 并行线程数（1 = 串行）
    QPlainTextEdit* m_candidatesEdit;

    QTextEdit* m_logText;
    QTableWidget* m_resultTable;
    QLabel* m_statusLabel;   // 状态栏文字
    QAction* m_darkModeAction;  // 暗色主题切换

    // 对比耗时刷新（A 档进度：只显示已用时间）
    QTimer* m_compareTickTimer;
    QElapsedTimer m_compareClock;

    QThread* m_workerThread;
    QThread* m_compareThread;    // 对比模式线程（与 m_workerThread 互斥使用，析构时两者都要回收）
    SimulationWorker* m_worker;
    ComparisonWorker* m_compareWorker;

    // 职业默认值映射
    QMap<QString, QMap<QString, double>> m_defaults;
};
#endif // MAINWINDOW_H