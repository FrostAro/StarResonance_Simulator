#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QThread>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStatusBar>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <random>
#include <utility>
#include <QDebug>

// 包含您的核心头文件（路径根据实际项目调整）
#include "Mage/Beam/Person.h"
#include "Mage/Beam/Initializer.hpp"
#include "Mage/Beam/SimulationHelper.h"
#include "core/Logger.h"
#include "core/Statistics.h"
#include "core/Action.h"
#include "core/SimulationLog.h"
#include <sstream>

//==============================================================================
// SimulationWorker 实现
//==============================================================================
SimulationWorker::SimulationWorker(const QString& profession,
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
                                   int fantasyConfig)
    : m_profession(profession),
      m_primaryAttr(primaryAttr),
      m_crit(crit),
      m_quickness(quickness),
      m_lucky(lucky),
      m_proficient(proficient),
      m_almighty(almighty),
      m_atk(atk),
      m_refineAtk(refineAtk),
      m_elementAtk(elementAtk),
      m_attackSpeed(attackSpeed),
      m_castingSpeed(castingSpeed),
      m_critDmgSet(critDmgSet),
      m_incSet(incSet),
      m_eleIncSet(eleIncSet),
      m_times(times),
      m_maxTime(maxTime),
      m_deltaTime(deltaTime),
      m_randomSeed(randomSeed),
      m_seed(seed),
      m_fantasyConfig(fantasyConfig)
{}

SimulationWorker::~SimulationWorker()
{
    // 日志回调只在 run() 末尾清理，不在析构中清——否则旧 worker 的析构
    // 可能在新的 run() 已设置回调之后执行，把新回调清掉（竞态）
}

void SimulationWorker::run()
{
    // 模拟期间压低日志级别，避免 buff/技能 DEBUG 日志海量刷屏
    const Logger::Level oldLevel = Logger::getLevel();
    Logger::setLevel(Logger::Level::WARNING);

    // 设置日志回调
    auto callback = [this](const std::string &msg) {
        emit logMessage(QString::fromStdString(msg));
    };
    Logger::setLogCallback(callback);

    std::vector<std::unordered_map<std::string, DamageStatistics>> damageStatisticsList;

    bool loggedFirst = false;
    for (int i = 0; i < m_times; ++i) {
        // 重置自动攻击计时器
        AutoAttack::setTimer() = 0;

        // 创建 Beam 职业人物对象
        std::unique_ptr<Person> person = std::make_unique<Mage_Beam>(
            m_primaryAttr, m_crit, m_quickness, m_lucky, m_proficient, m_almighty,
            m_atk, m_refineAtk, m_elementAtk,
            m_attackSpeed, m_castingSpeed,
            m_critDmgSet, m_incSet, m_eleIncSet,
            m_maxTime, m_fantasyConfig);

        // 设置随机种子
        if (m_randomSeed) {
            person->setRandomSeed(std::random_device{}());
        } else {
            person->setRandomSeed(m_seed);
        }

        // 初始化角色（装备技能、Buff等）
        auto init = std::make_unique<Initializer_Mage_Beam>(person.get(), m_deltaTime, m_fantasyConfig);
        init->Initialize();

        // 首次模拟：开启DEBUG供调试日志文件捕获（命名含 -gui-编译方式）
        if (i == 0)
        {
            Logger::setLevel(Logger::Level::DEBUG);
            std::ostringstream params;
            params << "三维=" << m_primaryAttr << ", 暴击=" << m_crit << ", 急速=" << m_quickness
                   << ", 幸运=" << m_lucky << ", 精通=" << m_proficient << ", 全能=" << m_almighty
                   << ", 攻击=" << m_atk << ", 精炼=" << m_refineAtk << ", 元素=" << m_elementAtk
                   << ", 攻速=" << m_attackSpeed << ", 施速=" << m_castingSpeed
                   << ", 爆伤=" << m_critDmgSet << ", 增伤=" << m_incSet << ", 元素增伤=" << m_eleIncSet
                   << ", fantasyConfig=" << m_fantasyConfig;
            loggedFirst = SimulationLog::begin("gui", person.get(), params.str());
        }

        emit logMessage(QString("开始第 %1 次模拟...").arg(i + 1));

        int currentTime = 0;
        while (currentTime < m_maxTime && !QThread::currentThread()->isInterruptionRequested()) {
            person->autoAttackPtr->update(m_deltaTime);
            currentTime += m_deltaTime;
            AutoAttack::setTimer() += m_deltaTime;
        }

        // 首次模拟结束：收尾日志文件并恢复 WARNING 级别
        if (i == 0 && loggedFirst)
        {
            SimulationLog::end();
            Logger::setLevel(Logger::Level::WARNING);
        }

        // 收集统计
        person->calculateDamageStatistics();
        damageStatisticsList.push_back(person->damageStatsMap);

        emit logMessage(QString("第 %1 次模拟完成").arg(i + 1));
    }

    // 准备表格数据
    QVector<QVector<QVariant>> tableData;
    if (m_times == 1) {
        // 单次结果直接转换
        const auto& statsMap = damageStatisticsList[0];
        for (const auto& [skillName, stat] : statsMap) {
            double totalDmg = stat.damage + stat.luckyDamage;
            double dps = totalDmg / (m_maxTime / 100.0);
            double critRate = stat.damageCount > 0 ? (stat.CritDamageCount / stat.damageCount) * 100.0 : 0.0;
            QVector<QVariant> row;
            row << QString::fromStdString(skillName)
                << stat.damage
                << stat.damageCount
                << stat.luckyDamage
                << stat.luckyDamageCount
                << dps
                << critRate;
            tableData.append(row);
        }
    } else {
        // 多次模拟：累加所有结果
        std::unordered_map<std::string, DamageStatistics> accumulatedMap;
        for (const auto& statsMap : damageStatisticsList) {
            for (const auto& [skillName, stat] : statsMap) {
                auto& acc = accumulatedMap[skillName];
                acc.skillName = skillName;
                acc.damage += stat.damage;
                acc.damageCount += stat.damageCount;
                acc.luckyDamage += stat.luckyDamage;
                acc.luckyDamageCount += stat.luckyDamageCount;
                acc.CritDamageCount += stat.CritDamageCount;
            }
        }
        // 计算平均值
        for (auto& [skillName, acc] : accumulatedMap) {
            acc.damage /= m_times;
            acc.damageCount /= m_times;
            acc.luckyDamage /= m_times;
            acc.luckyDamageCount /= m_times;
            acc.CritDamageCount /= m_times;
        }
        // 转换为表格数据
        for (const auto& [skillName, acc] : accumulatedMap) {
            double totalDmg = acc.damage + acc.luckyDamage;
            double dps = totalDmg / (m_maxTime / 100.0);
            double critRate = acc.damageCount > 0 ? (acc.CritDamageCount / acc.damageCount) * 100.0 : 0.0;
            QVector<QVariant> row;
            row << QString::fromStdString(skillName)
                << acc.damage
                << acc.damageCount
                << acc.luckyDamage
                << acc.luckyDamageCount
                << dps
                << critRate;
            tableData.append(row);
        }
        emit logMessage(QString("已完成 %1 次模拟，输出平均值。").arg(m_times));
    }

    emit simulationFinished(tableData, m_maxTime);

    // 移除日志回调并恢复日志级别
    Logger::setLogCallback(nullptr);
    Logger::setLevel(oldLevel);
}

//==============================================================================
// 对比模式：射线单次模拟函数（共享 Mage/Beam/SimulationHelper.h）
//==============================================================================

//==============================================================================
// ComparisonWorker 实现
//==============================================================================
ComparisonWorker::~ComparisonWorker()
{
    // 日志回调只在 run() 末尾清理，不在析构中清——避免竞态清掉新 worker 的回调
}

void ComparisonWorker::run()
{
    // 压低日志级别：配对对比会产生大量buff/技能DEBUG日志，避免刷屏卡顿GUI
    const Logger::Level oldLevel = Logger::getLevel();
    Logger::setLevel(Logger::Level::WARNING);

    auto callback = [this](const std::string &msg) {
        emit logMessage(QString::fromStdString(msg));
    };
    Logger::setLogCallback(callback);

    emit logMessage(QString("开始配对对比：基准 + %1 个候选，%2 对，种子 %3..%4")
                        .arg(m_candidates.size())
                        .arg(m_pairs)
                        .arg(m_seed)
                        .arg(m_seed + m_pairs - 1));

    // 支持窗口关闭/中断时提前停止（配合 shouldStop 回调，避免关闭窗口时卡死）
    auto shouldStop = []() { return QThread::currentThread()->isInterruptionRequested(); };
    auto results = runPairedComparison(m_base, m_candidates, m_seed, m_pairs,
                                       m_maxTime, m_deltaTime, m_simulate, shouldStop);

    if (QThread::currentThread()->isInterruptionRequested())
        emit logMessage("对比已中断，仅输出已完成的候选。");

    QVector<QVector<QVariant>> rows;
    rows.reserve(results.size());
    for (const auto& r : results)
    {
        QVector<QVariant> row;
        row << QString::fromStdString(r.label)
            << r.baseDps << r.candDps << r.deltaDps << r.deltaPercent
            << r.deltaStd << r.tValue;
        rows.append(row);
    }

    emit comparisonFinished(rows, m_pairs);

    // 移除日志回调并恢复日志级别
    Logger::setLogCallback(nullptr);
    Logger::setLevel(oldLevel);
}

//==============================================================================
// 明/暗两套主题样式
//==============================================================================
static const char* kLightStyle = R"(
    QMainWindow { background-color: #eef1f6; }
    QGroupBox { background-color: #ffffff; border: 1px solid #dfe3ea; border-radius: 8px; margin-top: 14px; padding: 10px 12px 12px 12px; }
    QGroupBox::title { subcontrol-origin: margin; left: 10px; top: 0px; padding: 0 4px; color: #111827; font-weight: bold; font-size: 13px; }
    QLabel { color: #374151; }
    QLineEdit, QComboBox, QPlainTextEdit { background: #ffffff; border: 1px solid #d1d5db; border-radius: 6px; padding: 5px 8px; selection-background-color: #2563eb; }
    QLineEdit:focus, QComboBox:focus, QPlainTextEdit:focus { border-color: #2563eb; }
    QLineEdit:disabled, QComboBox:disabled, QPlainTextEdit:disabled { background: #f3f4f6; color: #9ca3af; }
    QPushButton { background: #2563eb; color: #ffffff; border: none; border-radius: 6px; padding: 8px 16px; font-weight: bold; }
    QPushButton:hover { background: #1d4ed8; }
    QPushButton:pressed { background: #1e40af; }
    QPushButton:disabled { background: #9ca3af; }
    QCheckBox { spacing: 6px; color: #374151; }
    QCheckBox::indicator { width: 18px; height: 18px; border: 2px solid #2563eb; border-radius: 4px; background: #ffffff; }
    QCheckBox::indicator:checked { background: #2563eb; }
    QCheckBox#accentCheck { color: #1f2937; font-weight: bold; background-color: #e2e8f0; border: 1px solid #94a3b8; border-radius: 6px; padding: 5px 8px; }
    QTableWidget { background: #ffffff; border: 1px solid #dfe3ea; border-radius: 6px; gridline-color: #eef1f6; selection-background-color: #dbeafe; selection-color: #111827; }
    QHeaderView::section { background: #f3f4f6; border: none; border-bottom: 1px solid #dfe3ea; padding: 6px 8px; font-weight: bold; color: #374151; }
    QScrollArea { border: none; background: transparent; }
    QScrollBar:vertical { background: transparent; width: 10px; }
    QScrollBar::handle:vertical { background: #cbd5e1; border-radius: 5px; min-height: 30px; }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    QMenuBar { background-color: #ffffff; color: #111827; }
    QMenuBar::item:selected { background-color: #eef1f6; }
    QMenu { background-color: #ffffff; color: #111827; border: 1px solid #dfe3ea; }
    QMenu::item:selected { background-color: #2563eb; color: #ffffff; }
    QStatusBar { background-color: #f3f4f6; color: #374151; }
    QComboBox QAbstractItemView { background-color: #ffffff; color: #111827; selection-background-color: #dbeafe; }
    #centralArea { background-color: #eef1f6; }
    #inputViewport, #inputPanel { background-color: transparent; }
)";

static const char* kDarkStyle = R"(
    QMainWindow { background-color: #16181d; }
    QGroupBox { background-color: #1e2229; border: 1px solid #2c313a; border-radius: 8px; margin-top: 14px; padding: 10px 12px 12px 12px; }
    QGroupBox::title { subcontrol-origin: margin; left: 10px; top: 0px; padding: 0 4px; color: #e5e7eb; font-weight: bold; font-size: 13px; }
    QLabel { color: #9ca3af; }
    QLineEdit, QComboBox, QPlainTextEdit { background: #232830; border: 1px solid #37404c; border-radius: 6px; padding: 5px 8px; color: #e5e7eb; selection-background-color: #2563eb; }
    QLineEdit:focus, QComboBox:focus, QPlainTextEdit:focus { border-color: #3b82f6; }
    QLineEdit:disabled, QComboBox:disabled, QPlainTextEdit:disabled { background: #1a1d23; color: #4b5563; }
    QPushButton { background: #2563eb; color: #ffffff; border: none; border-radius: 6px; padding: 8px 16px; font-weight: bold; }
    QPushButton:hover { background: #3b82f6; }
    QPushButton:pressed { background: #1d4ed8; }
    QPushButton:disabled { background: #374151; }
    QCheckBox { spacing: 6px; color: #9ca3af; }
    QCheckBox::indicator { width: 18px; height: 18px; border: 2px solid #3b82f6; border-radius: 4px; background: #232830; }
    QCheckBox::indicator:checked { background: #3b82f6; }
    QCheckBox#accentCheck { color: #e5e7eb; font-weight: bold; background-color: #2a303c; border: 1px solid #4b5563; border-radius: 6px; padding: 5px 8px; }
    QTableWidget { background: #1e2229; border: 1px solid #2c313a; border-radius: 6px; gridline-color: #232830; color: #e5e7eb; selection-background-color: #1e40af; selection-color: #ffffff; }
    QHeaderView::section { background: #232830; border: none; border-bottom: 1px solid #2c313a; padding: 6px 8px; font-weight: bold; color: #9ca3af; }
    QScrollArea { border: none; background: transparent; }
    QScrollBar:vertical { background: transparent; width: 10px; }
    QScrollBar::handle:vertical { background: #374151; border-radius: 5px; min-height: 30px; }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    QMenuBar { background-color: #1e2229; color: #e5e7eb; }
    QMenuBar::item:selected { background-color: #374151; }
    QMenu { background-color: #232830; color: #e5e7eb; border: 1px solid #37404c; }
    QMenu::item:selected { background-color: #1e40af; color: #ffffff; }
    QStatusBar { background-color: #1e2229; color: #9ca3af; }
    QComboBox QAbstractItemView { background-color: #232830; color: #e5e7eb; selection-background-color: #1e40af; }
    #centralArea { background-color: #16181d; }
    #inputViewport, #inputPanel { background-color: transparent; }
)";

//==============================================================================
// MainWindow 实现
//==============================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_workerThread(nullptr), m_worker(nullptr), m_compareWorker(nullptr)
{
    // 默认浅色主题 + 更大默认窗口
    setStyleSheet(kLightStyle);
    resize(1400, 900);

    setupUI();

    // 视图菜单：暗色主题切换
    m_darkModeAction = new QAction("暗色主题", this);
    m_darkModeAction->setCheckable(true);
    connect(m_darkModeAction, &QAction::toggled, [this](bool dark) { applyTheme(dark); });
    QMenu *viewMenu = menuBar()->addMenu("视图");
    viewMenu->addAction(m_darkModeAction);

    // 状态栏
    m_statusLabel = new QLabel("就绪");
    statusBar()->addWidget(m_statusLabel);

    connect(m_professionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProfessionChanged);
    connect(m_runButton, &QPushButton::clicked, this, &MainWindow::onRunClicked);

    // 初始化职业默认值
    m_defaults["beam"] = {
        {"primaryAttr", 6760}, {"crit", 18.00}, {"quickness", 40.88}, {"lucky", 48.00}, 
        {"proficient", 48.09}, {"almighty", 21.00}, {"atk", 4533}, {"refineAtk", 1000}, 
        {"elementAtk", 230}, {"attackSpeed", 0.00}, {"castingSpeed", 0.00}, {"critDmgSet", 0}, 
        {"incSet", 0}, {"eleIncSet", 0}, {"times", 1}, {"maxTime", 18000}, {"deltaTime", 1}, {"seed", 42}};
    onProfessionChanged(0); // 初始化 Beam
}

MainWindow::~MainWindow()
{
    if (m_workerThread)
    {
        m_workerThread->requestInterruption();
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void MainWindow::setupUI()
{
    setWindowTitle("DPS Simulator (Beta)  by 星玥");
    setMinimumSize(1280, 840);

    QWidget *central = new QWidget(this);
    central->setObjectName("centralArea");
    setCentralWidget(central);

    // 主垂直布局：上部分左右两栏，下部分表格
    QVBoxLayout *mainVerticalLayout = new QVBoxLayout(central);
    mainVerticalLayout->setSpacing(10);
    mainVerticalLayout->setContentsMargins(10, 10, 10, 10);

    // 上部分：水平布局，包含输入面板和调试面板
    // 输入面板行数较多，套入滚动区域避免底部（对比模式等）被窗口高度裁掉
    QHBoxLayout *topHorizontalLayout = new QHBoxLayout();
    QScrollArea *inputScroll = new QScrollArea;
    inputScroll->viewport()->setObjectName("inputViewport");
    inputScroll->setWidgetResizable(true);
    inputScroll->setWidget(createInputPanel());
    topHorizontalLayout->addWidget(inputScroll, 3);
    topHorizontalLayout->addWidget(createDebugPanel(), 2);
    mainVerticalLayout->addLayout(topHorizontalLayout, 2); // 2 为拉伸因子

    // 下部分：结果面板
    mainVerticalLayout->addWidget(createResultPanel(), 1);
}

QWidget *MainWindow::createInputPanel()
{
    // 面板容器（子分组自带标题，外层不再套 GroupBox）
    QWidget *panel = new QWidget;
    panel->setObjectName("inputPanel");
    QVBoxLayout *mainLayout = new QVBoxLayout(panel);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    // 便捷：向网格添加"标签 + 输入框"一行
    auto addField = [](QGridLayout* gl, int row, const QString& label, QWidget* field) {
        gl->addWidget(new QLabel(label), row, 0);
        gl->addWidget(field, row, 1);
    };
    auto makeGroup = [](const QString& title) {
        QGroupBox* box = new QGroupBox(title);
        QGridLayout* gl = new QGridLayout(box);
        gl->setColumnStretch(0, 0);
        gl->setColumnStretch(1, 1);
        return std::make_pair(box, gl);
    };

    // ---- 职业与幻想 ----
    auto [classBox, classLayout] = makeGroup("职业");
    addField(classLayout, 0, "职业", m_professionCombo = new QComboBox);
    m_professionCombo->addItem("射线 · Beam");
    addField(classLayout, 1, "幻想配置", m_fantasyCombo = new QComboBox);
    m_fantasyCombo->addItem("无幻想");
    m_fantasyCombo->addItem("姆头 + 尖兵");
    m_fantasyCombo->addItem("姆头 + 伊戈雷乌斯");
    m_fantasyCombo->addItem("姆头 + 嗜血毛球");
    m_fantasyCombo->addItem("嗜血毛球 + 幻妖蟹蛛");
    m_fantasyCombo->addItem("尖兵 + 嗜血毛球");
    m_fantasyCombo->addItem("掠食蜘蛛 + 嗜血毛球");
    mainLayout->addWidget(classBox);

    // ---- 基础属性 ----
    auto [baseBox, baseLayout] = makeGroup("基础属性");
    addField(baseLayout, 0, "三维属性", m_primaryAttrEdit = new QLineEdit("4593"));
    addField(baseLayout, 1, "暴击 (%)", m_critEdit = new QLineEdit("36.00"));
    addField(baseLayout, 2, "急速 (%)", m_quicknessEdit = new QLineEdit("1.05"));
    addField(baseLayout, 3, "幸运 (%)", m_luckyEdit = new QLineEdit("51.70"));
    addField(baseLayout, 4, "精通 (%)", m_proficientEdit = new QLineEdit("6.00"));
    addField(baseLayout, 5, "全能 (%)", m_almightyEdit = new QLineEdit("17.58"));
    mainLayout->addWidget(baseBox);

    // ---- 攻击 ----
    auto [atkBox, atkLayout] = makeGroup("攻击");
    addField(atkLayout, 0, "攻击", m_atkEdit = new QLineEdit("3111"));
    addField(atkLayout, 1, "精炼攻击", m_refineAtkEdit = new QLineEdit("820"));
    addField(atkLayout, 2, "元素攻击", m_elementAtkEdit = new QLineEdit("35"));
    addField(atkLayout, 3, "额外攻击速度 (%)", m_attackSpeedEdit = new QLineEdit("10.00"));
    addField(atkLayout, 4, "额外施法速度 (%)", m_castingSpeedEdit = new QLineEdit("0.00"));
    mainLayout->addWidget(atkBox);

    // ---- 额外乘区 ----
    auto [boostBox, boostLayout] = makeGroup("额外乘区");
    addField(boostLayout, 0, "爆伤额外值", m_critDmgSetEdit = new QLineEdit("0"));
    addField(boostLayout, 1, "增伤额外值", m_incSetEdit = new QLineEdit("0"));
    addField(boostLayout, 2, "元素增伤额外值", m_eleIncSetEdit = new QLineEdit("0"));
    mainLayout->addWidget(boostBox);

    // ---- 模拟参数 ----
    auto [simBox, simLayout] = makeGroup("模拟参数");
    addField(simLayout, 0, "模拟循环次数", m_timesEdit = new QLineEdit("1"));
    addField(simLayout, 1, "最大运行时间 (0.01s)", m_maxTimeEdit = new QLineEdit("18000"));
    addField(simLayout, 2, "deltaTime (0.01s)", m_deltaTimeEdit = new QLineEdit("1"));
    m_randomSeedCheck = new QCheckBox("使用随机种子");
    m_randomSeedCheck->setObjectName("accentCheck");
    simLayout->addWidget(m_randomSeedCheck, 3, 0, 1, 2);
    addField(simLayout, 4, "固定种子", m_seedEdit = new QLineEdit("42"));
    m_seedEdit->setEnabled(false); // 初始禁用，因为随机种子默认未勾选
    mainLayout->addWidget(simBox);

    // 连接随机种子复选框与种子输入框的启用状态
    connect(m_randomSeedCheck, &QCheckBox::toggled, [this](bool checked){
        m_seedEdit->setEnabled(!checked);
    });

    // ---- 对比模式（同种子配对）----
    auto [compareBox, compareLayout] = makeGroup("对比模式（同种子配对）");
    m_compareCheck = new QCheckBox("启用对比模式");
    m_compareCheck->setObjectName("accentCheck");
    m_compareCheck->setToolTip(
        "勾选后：以当前输入面板为基准，按下方候选配置逐行对比DPS提升。\n"
        "基准与每个候选在相同随机种子下配对模拟，程序随机互相抵消，1%级提升也能测出。");
    compareLayout->addWidget(m_compareCheck, 0, 0, 1, 2);
    addField(compareLayout, 1, "配对次数", m_comparePairsEdit = new QLineEdit("20"));
    compareLayout->addWidget(new QLabel("候选配置(每行一个)"), 2, 0);
    m_candidatesEdit = new QPlainTextEdit;
    m_candidatesEdit->setPlainText(
        "# 每行一个候选：关键词 增量\n"
        "暴击 +5\n"
        "攻击 +100\n"
        "精通 +10\n"
        "# 幻想用绝对值：-1=无幻想，0-5=具体幻想(见上面板)，如：幻想 -1");
    m_candidatesEdit->setFixedHeight(80);
    compareLayout->addWidget(m_candidatesEdit, 2, 1);
    mainLayout->addWidget(compareBox);

    // 勾选对比模式时：启用对比配置，禁用无关的模拟循环次数；反之恢复
    connect(m_compareCheck, &QCheckBox::toggled, [this](bool on){
        m_comparePairsEdit->setEnabled(on);
        m_candidatesEdit->setEnabled(on);
        m_timesEdit->setEnabled(!on);
    });
    m_comparePairsEdit->setEnabled(false);
    m_candidatesEdit->setEnabled(false);

    // ---- 运行按钮 ----
    m_runButton = new QPushButton("运行模拟");
    m_runButton->setMinimumHeight(40);
    mainLayout->addWidget(m_runButton);

    return panel;
}

QWidget *MainWindow::createDebugPanel()
{
    QGroupBox *groupBox = new QGroupBox("实时日志");
    groupBox->setStyleSheet("QGroupBox { font-weight: bold; }");

    QVBoxLayout *layout = new QVBoxLayout(groupBox);
    m_logText = new QTextEdit;
    m_logText->setReadOnly(true);
    m_logText->setFontFamily("Courier New");
    m_logText->setStyleSheet("background-color: #1e1e2e; color: #d4d4d4;");
    layout->addWidget(m_logText);

    QPushButton *clearBtn = new QPushButton("清空");
    connect(clearBtn, &QPushButton::clicked, [this]()
            { m_logText->clear(); });
    layout->addWidget(clearBtn);

    return groupBox;
}

QWidget *MainWindow::createResultPanel()
{
    QGroupBox *groupBox = new QGroupBox("伤害统计结果");
    groupBox->setStyleSheet("QGroupBox { font-weight: bold; }");

    QVBoxLayout *layout = new QVBoxLayout(groupBox);
    m_resultTable = new QTableWidget;
    m_resultTable->setColumnCount(7);
    QStringList headers = {"技能", "总伤害", "攻击次数", "幸运伤害", "幸运次数", "DPS", "暴击率"};
    m_resultTable->setHorizontalHeaderLabels(headers);
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_resultTable);

    return groupBox;
}

void MainWindow::onProfessionChanged(int index)
{
    m_fantasyCombo->clear(); // 清空原有选项
    m_fantasyCombo->addItem("无幻想");
    m_fantasyCombo->addItem("姆头 + 尖兵");
    m_fantasyCombo->addItem("姆头 + 伊戈雷乌斯");
    m_fantasyCombo->addItem("姆头 + 嗜血毛球");
    m_fantasyCombo->addItem("嗜血毛球 + 幻妖蟹蛛");
    m_fantasyCombo->addItem("尖兵 + 嗜血毛球");
    m_fantasyCombo->addItem("掠食蜘蛛 + 嗜血毛球");
    m_fantasyCombo->setCurrentIndex(0);
    updateDefaultsForProfession("beam");
}

void MainWindow::updateDefaultsForProfession(const QString &prof)
{
    auto def = m_defaults[prof];
    m_primaryAttrEdit->setText(QString::number(def["primaryAttr"]));
    m_critEdit->setText(QString::number(def["crit"], 'f', 2));
    m_quicknessEdit->setText(QString::number(def["quickness"], 'f', 2));
    m_luckyEdit->setText(QString::number(def["lucky"], 'f', 2));
    m_proficientEdit->setText(QString::number(def["proficient"], 'f', 2));
    m_almightyEdit->setText(QString::number(def["almighty"], 'f', 2));
    m_atkEdit->setText(QString::number(def["atk"]));
    m_refineAtkEdit->setText(QString::number(def["refineAtk"]));
    m_elementAtkEdit->setText(QString::number(def["elementAtk"]));
    m_attackSpeedEdit->setText(QString::number(def["attackSpeed"], 'f', 2));
    m_castingSpeedEdit->setText(QString::number(def["castingSpeed"], 'f', 2));
    m_critDmgSetEdit->setText(QString::number(def["critDmgSet"], 'f', 2));
    m_incSetEdit->setText(QString::number(def["incSet"], 'f', 2));
    m_eleIncSetEdit->setText(QString::number(def["eleIncSet"], 'f', 2));
    m_timesEdit->setText(QString::number(def["times"]));
    m_maxTimeEdit->setText(QString::number(def["maxTime"]));
    m_deltaTimeEdit->setText(QString::number(def["deltaTime"]));
    m_seedEdit->setText(QString::number(def["seed"]));

    appendLog("[CONFIG] 已切换至 射线 默认参数");
}

// 从输入面板构建基准配置（SimConfig）
SimConfig MainWindow::buildBaseConfig()
{
    SimConfig cfg;
    cfg.label = "基准";
    cfg.primaryAttributes = m_primaryAttrEdit->text().toDouble();
    cfg.critical = m_critEdit->text().toDouble();
    cfg.quickness = m_quicknessEdit->text().toDouble();
    cfg.lucky = m_luckyEdit->text().toDouble();
    cfg.proficient = m_proficientEdit->text().toDouble();
    cfg.almighty = m_almightyEdit->text().toDouble();
    cfg.atk = m_atkEdit->text().toInt();
    cfg.refineATK = m_refineAtkEdit->text().toInt();
    cfg.elementATK = m_elementAtkEdit->text().toInt();
    cfg.attackSpeed = m_attackSpeedEdit->text().toDouble();
    cfg.castingSpeed = m_castingSpeedEdit->text().toDouble();
    cfg.criticaldamage_set = m_critDmgSetEdit->text().toDouble();
    cfg.increasedamage_set = m_incSetEdit->text().toDouble();
    cfg.elementdamage_set = m_eleIncSetEdit->text().toDouble();
    int fantasyConfig = m_fantasyCombo->currentIndex() - 1;  // 面板index：0=无幻想 → -1，1..6 → 0..5
    cfg.fantasyConfig = fantasyConfig;  // -1 = 无幻想，直接命中 Person.cpp 的 default 分支
    return cfg;
}

// 解析候选配置文本框：每行 "关键词 增量"，# 开头为注释
// 关键词：三维/primary、暴击/crit、急速/quickness、幸运/lucky、精通/proficient、全能/almighty、
//        攻击/atk、精炼/refineAtk、元素/elementAtk、攻速/attackSpeed、施速/castingSpeed、
//        爆伤/critDmgSet、增伤/incSet、元素增伤/eleIncSet、幻想/fantasy(绝对值)
std::vector<SimConfig> MainWindow::parseCandidates(const SimConfig& base, const QString& text)
{
    std::vector<SimConfig> out;
    const QStringList lines = text.split('\n');
    for (QString line : lines)
    {
        line = line.trimmed();
        int hash = line.indexOf('#');
        if (hash >= 0) line = line.left(hash).trimmed();
        if (line.isEmpty()) continue;

        const QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() < 2) continue;
        const QString& key = parts[0];
        bool ok = false;
        double value = parts[1].toDouble(&ok);
        if (!ok) continue;

        SimConfig c = base;
        if      (key == "三维"    || key == "primary")     c.primaryAttributes += value;
        else if (key == "暴击"    || key == "crit")        c.critical += value;
        else if (key == "急速"    || key == "quickness")   c.quickness += value;
        else if (key == "幸运"    || key == "lucky")       c.lucky += value;
        else if (key == "精通"    || key == "proficient")  c.proficient += value;
        else if (key == "全能"    || key == "almighty")    c.almighty += value;
        else if (key == "攻击"    || key == "atk")         c.atk += static_cast<int>(value);
        else if (key == "精炼"    || key == "refineAtk")   c.refineATK += static_cast<int>(value);
        else if (key == "元素"    || key == "elementAtk")  c.elementATK += static_cast<int>(value);
        else if (key == "攻速"    || key == "attackSpeed") c.attackSpeed += value;
        else if (key == "施速"    || key == "castingSpeed")c.castingSpeed += value;
        else if (key == "爆伤"    || key == "critDmgSet")  c.criticaldamage_set += value;
        else if (key == "增伤"    || key == "incSet")      c.increasedamage_set += value;
        else if (key == "元素增伤"|| key == "eleIncSet")   c.elementdamage_set += value;
        else if (key == "幻想"    || key == "fantasy")     c.fantasyConfig = static_cast<int>(value);
        else continue;  // 未知关键词，跳过该行

        c.label = line.toStdString();
        out.push_back(std::move(c));
    }
    return out;
}

void MainWindow::onRunClicked()
{
    if (m_workerThread && m_workerThread->isRunning())
    {
        QMessageBox::warning(this, "模拟中", "已有模拟正在运行，请等待完成。");
        return;
    }

    // 输入校验：空输入 toInt()=0 会引发死循环(deltaTime=0)或除零(maxTime=0)
    if (m_maxTimeEdit->text().toInt() <= 0)
    {
        QMessageBox::warning(this, "参数错误", "「最大运行时间」必须大于 0。");
        return;
    }
    if (m_deltaTimeEdit->text().toInt() <= 0)
    {
        QMessageBox::warning(this, "参数错误", "「deltaTime」必须 ≥ 1。");
        return;
    }

    // 对比模式：基准 = 当前输入面板，候选 = 下方候选配置文本框
    if (m_compareCheck->isChecked())
    {
        SimConfig base = buildBaseConfig();
        std::vector<SimConfig> candidates = parseCandidates(base, m_candidatesEdit->toPlainText());
        if (candidates.empty())
        {
            QMessageBox::warning(this, "对比", "没有有效的候选配置，请检查候选配置文本框（每行：关键词 增量）。");
            return;
        }
        int pairs = m_comparePairsEdit->text().toInt();
        if (pairs <= 0) pairs = 20;
        int maxTime = m_maxTimeEdit->text().toInt();
        int deltaTime = m_deltaTimeEdit->text().toInt();
        uint32_t seed = m_randomSeedCheck->isChecked() ? std::random_device{}() : m_seedEdit->text().toUInt();

        m_logText->clear();
        m_resultTable->setRowCount(0);

        m_workerThread = new QThread(this);
        m_compareWorker = new ComparisonWorker(base, std::move(candidates), pairs, seed,
                                               maxTime, deltaTime, runBeamSimulationOnce);
        m_compareWorker->moveToThread(m_workerThread);

        connect(m_workerThread, &QThread::started, m_compareWorker, &ComparisonWorker::run);
        connect(m_compareWorker, &ComparisonWorker::logMessage, this, &MainWindow::appendLog);
        connect(m_compareWorker, &ComparisonWorker::comparisonFinished, this, &MainWindow::onComparisonFinished);
        connect(m_compareWorker, &ComparisonWorker::comparisonFinished, m_workerThread, &QThread::quit);
        connect(m_workerThread, &QThread::finished, m_compareWorker, &QObject::deleteLater);
        connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);

        m_workerThread->start();
        m_runButton->setEnabled(false);
        m_statusLabel->setText("对比模拟中...");
        return;
    }

    // 读取所有输入值
    QString prof = "beam";
    double primaryAttr = m_primaryAttrEdit->text().toDouble();
    double crit = m_critEdit->text().toDouble();
    double quickness = m_quicknessEdit->text().toDouble();
    double lucky = m_luckyEdit->text().toDouble();
    double proficient = m_proficientEdit->text().toDouble();
    double almighty = m_almightyEdit->text().toDouble();
    int atk = m_atkEdit->text().toInt();
    int refineAtk = m_refineAtkEdit->text().toInt();
    int elementAtk = m_elementAtkEdit->text().toInt();
    double attackSpeed = m_attackSpeedEdit->text().toDouble();
    double castingSpeed = m_castingSpeedEdit->text().toDouble();
    double critDmgSet = m_critDmgSetEdit->text().toDouble();
    double incSet = m_incSetEdit->text().toDouble();
    double eleIncSet = m_eleIncSetEdit->text().toDouble();
    int times = m_timesEdit->text().toInt();
    if (times <= 0)
    {
        QMessageBox::warning(this, "参数错误", "「模拟循环次数」必须 ≥ 1。");
        return;
    }
    int maxTime = m_maxTimeEdit->text().toInt();
    int deltaTime = m_deltaTimeEdit->text().toInt();
    bool randomSeed = m_randomSeedCheck->isChecked();
    uint32_t seed = m_seedEdit->text().toUInt();
    int fantasyConfig = m_fantasyCombo->currentIndex() - 1; // 面板index：0=无幻想 → -1，1..6 → 0..5
    // -1 = 无幻想，直接传给 Person.cpp 的 default 分支（无需再转 999）

    // 清空之前的日志和表格
    m_logText->clear();
    m_resultTable->setRowCount(0);

    // 创建并启动工作线程
    m_workerThread = new QThread(this);
    m_worker = new SimulationWorker(prof,
                                    primaryAttr,
                                    crit,
                                    quickness,
                                    lucky,
                                    proficient,
                                    almighty,
                                    atk,
                                    refineAtk,
                                    elementAtk,
                                    attackSpeed,
                                    castingSpeed,
                                    critDmgSet,
                                    incSet,
                                    eleIncSet,
                                    times,
                                    maxTime,
                                    deltaTime,
                                    randomSeed,
                                    seed,
                                    fantasyConfig);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &SimulationWorker::run);
    connect(m_worker, &SimulationWorker::logMessage, this, &MainWindow::appendLog);
    connect(m_worker, &SimulationWorker::simulationFinished, this, &MainWindow::onSimulationFinished);
    connect(m_worker, &SimulationWorker::simulationFinished, m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);

    m_workerThread->start();
    m_runButton->setEnabled(false);
    m_statusLabel->setText("模拟中...");
}

void MainWindow::appendLog(const QString &msg)
{
    qDebug() << "appendLog:" << msg.left(50); // 只输出前50字符避免刷屏
    m_logText->append(msg);
    m_logText->ensureCursorVisible();
}

void MainWindow::onSimulationFinished(const QVector<QVector<QVariant>> &stats, int totalTime)
{
    m_runButton->setEnabled(true);
    m_statusLabel->setText("模拟完成");

    // 恢复普通伤害统计的列头（对比模式可能改过列头）
    QStringList headers = {"技能", "总伤害", "攻击次数", "幸运伤害", "幸运次数", "DPS", "暴击率"};
    m_resultTable->setColumnCount(7);
    m_resultTable->setHorizontalHeaderLabels(headers);

    m_resultTable->setRowCount(stats.size());
    for (int i = 0; i < stats.size(); ++i)
    {
        const auto &row = stats[i];
        for (int j = 0; j < row.size(); ++j)
        {
            QTableWidgetItem *item = new QTableWidgetItem(row[j].toString());
            if (j >= 1)
            { // 数值列右对齐
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
            m_resultTable->setItem(i, j, item);
        }
    }

    // 添加总计行（简化示例，实际应计算总和）
    double totalDamage = 0, totalLucky = 0;
    int totalCount = 0, totalLuckyCount = 0;
    for (int i = 0; i < stats.size(); ++i)
    {
        totalDamage += stats[i][1].toDouble();
        totalLucky += stats[i][3].toDouble();
        totalCount += stats[i][2].toInt();
        totalLuckyCount += stats[i][4].toInt();
    }
    double totalDps = (totalTime > 0) ? (totalDamage + totalLucky) / (totalTime / 100.0) : 0.0;  // 防御除零
    int row = m_resultTable->rowCount();
    m_resultTable->insertRow(row);
    m_resultTable->setItem(row, 0, new QTableWidgetItem("🔥 总计"));
    m_resultTable->setItem(row, 1, new QTableWidgetItem(QString::number(totalDamage, 'f', 0)));
    m_resultTable->setItem(row, 2, new QTableWidgetItem(QString::number(totalCount)));
    m_resultTable->setItem(row, 3, new QTableWidgetItem(QString::number(totalLucky, 'f', 0)));
    m_resultTable->setItem(row, 4, new QTableWidgetItem(QString::number(totalLuckyCount)));
    m_resultTable->setItem(row, 5, new QTableWidgetItem(QString::number(totalDps, 'f', 2)));
    m_resultTable->setItem(row, 6, new QTableWidgetItem(""));

    // 清空指针，防止下次点击时访问已销毁对象
    m_workerThread = nullptr;
    m_worker = nullptr;
}

void MainWindow::onComparisonFinished(const QVector<QVector<QVariant>>& rows, int pairs)
{
    m_runButton->setEnabled(true);
    m_statusLabel->setText("对比完成");

    QStringList headers = {"候选配置", "基准DPS", "候选DPS", "ΔDPS", "Δ%", "配对σ", "t值"};
    m_resultTable->setColumnCount(7);
    m_resultTable->setHorizontalHeaderLabels(headers);
    m_resultTable->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i)
    {
        const auto &row = rows[i];
        for (int j = 0; j < row.size(); ++j)
        {
            QTableWidgetItem *item = new QTableWidgetItem(row[j].toString());
            if (j >= 1)
            {
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
            // ΔDPS / Δ% 列按涨跌着色：绿涨红跌
            if (j == 3 || j == 4)
            {
                double v = row[j].toDouble();
                item->setForeground(v > 0 ? QColor("#10b981")
                                    : v < 0 ? QColor("#ef4444")
                                            : QColor("#6b7280"));
            }
            m_resultTable->setItem(i, j, item);
        }
    }
    appendLog(QString("对比完成：%1 对。配对σ远小于基准DPS表示CRN生效；|t值|≥2视为差异显著。").arg(pairs));

    // 清空指针，防止下次点击时访问已销毁对象
    m_workerThread = nullptr;
    m_compareWorker = nullptr;
}

void MainWindow::applyTheme(bool dark)
{
    setStyleSheet(dark ? kDarkStyle : kLightStyle);
}