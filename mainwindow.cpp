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
#include <random>
#include <QDebug>

// 包含您的核心头文件（路径根据实际项目调整）
#include "Mage/Beam/Person.h"
#include "Mage/Beam/Initializer.hpp"
#include "core/Logger.h"
#include "core/Statistics.h"
#include "core/Action.h"

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
    // 确保移除日志回调，避免悬挂指针   
    Logger::setLogCallback(nullptr);
}

void SimulationWorker::run()
{
    // 设置日志回调
    auto callback = [this](const std::string &msg) {
        emit logMessage(QString::fromStdString(msg));
    };
    Logger::setLogCallback(callback);

    std::vector<std::unordered_map<std::string, DamageStatistics>> damageStatisticsList;

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

        emit logMessage(QString("开始第 %1 次模拟...").arg(i + 1));

        int currentTime = 0;
        while (currentTime < m_maxTime && !QThread::currentThread()->isInterruptionRequested()) {
            person->autoAttackPtr->update(m_deltaTime);
            currentTime += m_deltaTime;
            AutoAttack::setTimer() += m_deltaTime;
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

    // 移除日志回调
    Logger::setLogCallback(nullptr);
}

//==============================================================================
// 对比模式：射线单次模拟函数（供配对对比使用）
//==============================================================================
static std::unordered_map<std::string, DamageStatistics> beamSimulateOnce(
    const SimConfig& cfg, std::uint32_t seed, int maxTime, int deltaTime)
{
    auto person = std::make_unique<Mage_Beam>(
        cfg.primaryAttributes, cfg.critical, cfg.quickness, cfg.lucky, cfg.proficient, cfg.almighty,
        cfg.atk, cfg.refineATK, cfg.elementATK,
        cfg.attackSpeed, cfg.castingSpeed,
        cfg.critialdamage_set, cfg.increasedamage_set, cfg.elementdamage_set,
        maxTime, cfg.fantasyConfig);
    person->setRandomSeed(seed);

    auto init = std::make_unique<Initializer_Mage_Beam>(person.get(), deltaTime, cfg.fantasyConfig);
    init->Initialize();

    int currentTime = 0;
    while (currentTime < maxTime)
    {
        person->autoAttackPtr->update(deltaTime);
        currentTime += deltaTime;
        person->autoAttackPtr->setTimer() += deltaTime;
    }

    person->calculateDamageStatistics();
    return person->damageStatsMap;
}

//==============================================================================
// ComparisonWorker 实现
//==============================================================================
ComparisonWorker::~ComparisonWorker()
{
    // 确保移除日志回调，避免悬挂指针
    Logger::setLogCallback(nullptr);
}

void ComparisonWorker::run()
{
    // 压低日志级别：配对对比会产生大量buff/技能DEBUG日志，避免刷屏卡顿GUI
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

    // 移除日志回调
    Logger::setLogCallback(nullptr);
}

//==============================================================================
// MainWindow 实现
//==============================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_workerThread(nullptr), m_worker(nullptr), m_compareWorker(nullptr)
{
    setupUI();
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
    setMinimumSize(1200, 800);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    // 主垂直布局：上部分左右两栏，下部分表格
    QVBoxLayout *mainVerticalLayout = new QVBoxLayout(central);
    mainVerticalLayout->setSpacing(10);
    mainVerticalLayout->setContentsMargins(10, 10, 10, 10);

    // 上部分：水平布局，包含输入面板和调试面板
    QHBoxLayout *topHorizontalLayout = new QHBoxLayout();
    topHorizontalLayout->addWidget(createInputPanel(), 3);
    topHorizontalLayout->addWidget(createDebugPanel(), 2);
    mainVerticalLayout->addLayout(topHorizontalLayout, 2); // 2 为拉伸因子

    // 下部分：结果面板
    mainVerticalLayout->addWidget(createResultPanel(), 1);
}

QWidget *MainWindow::createInputPanel()
{
    QGroupBox *groupBox = new QGroupBox("角色配置");
    groupBox->setStyleSheet("QGroupBox { font-weight: bold; }");

    QGridLayout *layout = new QGridLayout(groupBox);
    int row = 0; // 当前行计数器

    // 设置列拉伸：第0列固定大小（标签），第1列自动拉伸
    layout->setColumnStretch(0, 0);
    layout->setColumnStretch(1, 1);

    // 0: 职业选择
    layout->addWidget(new QLabel("职业 *"), row, 0);
    m_professionCombo = new QComboBox;
    m_professionCombo->addItem("射线 · Beam");
    layout->addWidget(m_professionCombo, row++, 1);

    // 1: 幻想配置
    layout->addWidget(new QLabel("幻想配置"), row, 0);
    m_fantasyCombo = new QComboBox;
    // 初始填充 Beam 的选项
    m_fantasyCombo->addItem("无幻想");
    m_fantasyCombo->addItem("姆头 + 尖兵");
    m_fantasyCombo->addItem("姆头 + 伊戈雷乌斯");
    m_fantasyCombo->addItem("姆头 + 嗜血毛球");
    m_fantasyCombo->addItem("嗜血毛球 + 幻妖蟹蛛");
    m_fantasyCombo->addItem("尖兵 + 嗜血毛球");
    m_fantasyCombo->addItem("掠食蜘蛛 + 嗜血毛球");
    layout->addWidget(m_fantasyCombo, row++, 1);

    // 2: 三维属性
    layout->addWidget(new QLabel("三维属性"), row, 0);
    m_primaryAttrEdit = new QLineEdit("4593");
    layout->addWidget(m_primaryAttrEdit, row++, 1);

    // 3: 暴击 (%)
    layout->addWidget(new QLabel("暴击 (%)"), row, 0);
    m_critEdit = new QLineEdit("36.00");
    layout->addWidget(m_critEdit, row++, 1);

    // 4: 急速 (%)
    layout->addWidget(new QLabel("急速 (%)"), row, 0);
    m_quicknessEdit = new QLineEdit("1.05");
    layout->addWidget(m_quicknessEdit, row++, 1);

    // 5: 幸运 (%)
    layout->addWidget(new QLabel("幸运 (%)"), row, 0);
    m_luckyEdit = new QLineEdit("51.70");
    layout->addWidget(m_luckyEdit, row++, 1);

    // 6: 精通 (%)
    layout->addWidget(new QLabel("精通 (%)"), row, 0);
    m_proficientEdit = new QLineEdit("6.00");
    layout->addWidget(m_proficientEdit, row++, 1);

    // 7: 全能 (%)
    layout->addWidget(new QLabel("全能 (%)"), row, 0);
    m_almightyEdit = new QLineEdit("17.58");
    layout->addWidget(m_almightyEdit, row++, 1);

    // 8: 攻击
    layout->addWidget(new QLabel("攻击"), row, 0);
    m_atkEdit = new QLineEdit("3111");
    layout->addWidget(m_atkEdit, row++, 1);

    // 9: 精炼攻击
    layout->addWidget(new QLabel("精炼攻击"), row, 0);
    m_refineAtkEdit = new QLineEdit("820");
    layout->addWidget(m_refineAtkEdit, row++, 1);

    // 10: 元素攻击
    layout->addWidget(new QLabel("元素攻击"), row, 0);
    m_elementAtkEdit = new QLineEdit("35");
    layout->addWidget(m_elementAtkEdit, row++, 1);

    // 11: 攻击速度 (%)
    layout->addWidget(new QLabel("额外攻击速度 (%)"), row, 0);
    m_attackSpeedEdit = new QLineEdit("10.00");
    layout->addWidget(m_attackSpeedEdit, row++, 1);

    // 12: 施法速度 (%)
    layout->addWidget(new QLabel("额外施法速度 (%)"), row, 0);
    m_castingSpeedEdit = new QLineEdit("0.00");
    layout->addWidget(m_castingSpeedEdit, row++, 1);

    // 13: 爆伤额外值
    layout->addWidget(new QLabel("爆伤额外值"), row, 0);
    m_critDmgSetEdit = new QLineEdit("0");
    layout->addWidget(m_critDmgSetEdit, row++, 1);

    // 14: 增伤额外值
    layout->addWidget(new QLabel("增伤额外值"), row, 0);
    m_incSetEdit = new QLineEdit("0");
    layout->addWidget(m_incSetEdit, row++, 1);

    // 15: 元素增伤额外值
    layout->addWidget(new QLabel("元素增伤额外值"), row, 0);
    m_eleIncSetEdit = new QLineEdit("0");
    layout->addWidget(m_eleIncSetEdit, row++, 1);

    // 16: 模拟循环次数
    layout->addWidget(new QLabel("模拟循环次数"), row, 0);
    m_timesEdit = new QLineEdit("1");
    layout->addWidget(m_timesEdit, row++, 1);

    // 17: 最大运行时间 (0.01s)
    layout->addWidget(new QLabel("最大运行时间 (0.01s)"), row, 0);
    m_maxTimeEdit = new QLineEdit("18000");
    layout->addWidget(m_maxTimeEdit, row++, 1);

    // 18: deltaTime (0.01s)
    layout->addWidget(new QLabel("deltaTime (0.01s)"), row, 0);
    m_deltaTimeEdit = new QLineEdit("1");
    layout->addWidget(m_deltaTimeEdit, row++, 1);

    // 19: 随机种子选项（跨两列）
    m_randomSeedCheck = new QCheckBox("使用随机种子");
    layout->addWidget(m_randomSeedCheck, row, 0, 1, 2); // 占两列
    row++; // 手动递增行号

    // 20: 固定种子
    layout->addWidget(new QLabel("固定种子"), row, 0);
    m_seedEdit = new QLineEdit("42");
    m_seedEdit->setEnabled(false); // 初始禁用，因为随机种子默认未勾选
    layout->addWidget(m_seedEdit, row++, 1);

    // 21: 运行按钮（跨两列）
    m_runButton = new QPushButton("运行模拟");
    m_runButton->setStyleSheet("QPushButton { background-color: #1d4ed8; color: white; font-weight: bold; padding: 8px; }");
    layout->addWidget(m_runButton, row, 0, 1, 2); // 跨两列
    row++; // 可选，后续不再使用

    // 22: 对比模式（同种子配对）
    m_compareCheck = new QCheckBox("启用对比模式（同种子配对）");
    layout->addWidget(m_compareCheck, row, 0, 1, 2);
    row++;
    layout->addWidget(new QLabel("配对次数"), row, 0);
    m_comparePairsEdit = new QLineEdit("20");
    layout->addWidget(m_comparePairsEdit, row++, 1);
    layout->addWidget(new QLabel("候选配置(每行一个)"), row, 0);
    m_candidatesEdit = new QPlainTextEdit;
    m_candidatesEdit->setPlainText(
        "# 每行一个候选：关键词 增量\n"
        "暴击 +5\n"
        "攻击 +100\n"
        "精通 +10\n"
        "# 幻想用绝对值，如：幻想 0");
    m_candidatesEdit->setFixedHeight(90);
    layout->addWidget(m_candidatesEdit, row++, 1);

    // 连接随机种子复选框与种子输入框的启用状态
    connect(m_randomSeedCheck, &QCheckBox::toggled, [this](bool checked){
        m_seedEdit->setEnabled(!checked);
    });

    return groupBox;
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
    cfg.critialdamage_set = m_critDmgSetEdit->text().toDouble();
    cfg.increasedamage_set = m_incSetEdit->text().toDouble();
    cfg.elementdamage_set = m_eleIncSetEdit->text().toDouble();
    int fantasyConfig = m_fantasyCombo->currentIndex() - 1;  // 0=无幻想 → -1 → 999
    cfg.fantasyConfig = (fantasyConfig < 0) ? 999 : fantasyConfig;
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
        else if (key == "爆伤"    || key == "critDmgSet")  c.critialdamage_set += value;
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
                                               maxTime, deltaTime, beamSimulateOnce);
        m_compareWorker->moveToThread(m_workerThread);

        connect(m_workerThread, &QThread::started, m_compareWorker, &ComparisonWorker::run);
        connect(m_compareWorker, &ComparisonWorker::logMessage, this, &MainWindow::appendLog);
        connect(m_compareWorker, &ComparisonWorker::comparisonFinished, this, &MainWindow::onComparisonFinished);
        connect(m_compareWorker, &ComparisonWorker::comparisonFinished, m_workerThread, &QThread::quit);
        connect(m_workerThread, &QThread::finished, m_compareWorker, &QObject::deleteLater);
        connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);

        m_workerThread->start();
        m_runButton->setEnabled(false);
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
    int maxTime = m_maxTimeEdit->text().toInt();
    int deltaTime = m_deltaTimeEdit->text().toInt();
    bool randomSeed = m_randomSeedCheck->isChecked();
    uint32_t seed = m_seedEdit->text().toUInt();
    int fantasyConfig = m_fantasyCombo->currentIndex() - 1; // 偏移：0=无幻想 → -1，1=姆头尖兵 → 0...
    if (fantasyConfig < 0) fantasyConfig = 999;             // 无幻想走 default 分支

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
    double totalDps = (totalDamage + totalLucky) / (totalTime / 100.0);
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
            m_resultTable->setItem(i, j, item);
        }
    }
    appendLog(QString("对比完成：%1 对。配对σ远小于基准DPS表示CRN生效；|t值|≥2视为差异显著。").arg(pairs));

    // 清空指针，防止下次点击时访问已销毁对象
    m_workerThread = nullptr;
    m_compareWorker = nullptr;
}