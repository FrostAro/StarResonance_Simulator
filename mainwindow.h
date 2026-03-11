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
#include <memory>
#include "core/Person.h"
#include "core/AutoAttack.h"

// 在 MainWindow 类前定义幻想配置枚举
enum class FantasyBeamConfig {
    MukuChief_MukuScout = 0,
    MukuChief_YGLWS,
    MukuChief_SXMQ
};

enum class FantasyIcicleConfig {
    MukuChief_MukuScout = 0,
    MukuChief_BYS,
    MukuChief_YGLWS
};

// ============================================================================
// 模拟循环实现

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
                              int fantasyConfig);
    ~SimulationWorker(); 
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

private:
    void setupUI();
    void updateDefaultsForProfession(const QString& prof);
    QWidget* createInputPanel();
    QWidget* createDebugPanel();
    QWidget* createResultPanel();

    // 输入控件
    QComboBox* m_professionCombo;
    QComboBox* m_fantasyCombo;  // 新增幻想配置下拉框
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
    QLineEdit* m_maxTimeEdit;
    QLineEdit* m_deltaTimeEdit;
    QCheckBox* m_randomSeedCheck;
    QLineEdit* m_seedEdit;
    QPushButton* m_runButton;

    QTextEdit* m_logText;
    QTableWidget* m_resultTable;

    QThread* m_workerThread;
    SimulationWorker* m_worker;

    // 职业默认值映射
    QMap<QString, QMap<QString, double>> m_defaults;
};
#endif // MAINWINDOW_H