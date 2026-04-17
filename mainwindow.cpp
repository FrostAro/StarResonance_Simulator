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
#include <random>
#include <QDebug>

// 包含您的核心头文件（路径根据实际项目调整）
#include "Mage/Icicle/Person.h"
#include "Mage/Icicle/Initializer.hpp"
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
{
}

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

        // 创建对应职业的人物对象
        std::unique_ptr<Person> person;
        if (m_profession == "icicle") {
            person = std::make_unique<Mage_Icicle>(
                m_primaryAttr, m_crit, m_quickness, m_lucky, m_proficient, m_almighty,
                m_atk, m_refineAtk, m_elementAtk,
                m_attackSpeed, m_castingSpeed,  // 注意：这里直接传入百分比值，不在内部除以100（与控制台版本一致）
                m_critDmgSet, m_incSet, m_eleIncSet,
                m_maxTime, m_fantasyConfig);
        } else {
            person = std::make_unique<Mage_Beam>(
                m_primaryAttr, m_crit, m_quickness, m_lucky, m_proficient, m_almighty,
                m_atk, m_refineAtk, m_elementAtk,
                m_attackSpeed, m_castingSpeed,
                m_critDmgSet, m_incSet, m_eleIncSet,
                m_maxTime, m_fantasyConfig);
        }

        // 设置随机种子
        if (m_randomSeed) {
            person->setRandomSeed(std::random_device{}());
        } else {
            person->setRandomSeed(m_seed);
        }

        // 初始化角色（装备技能、Buff等）
        if (m_profession == "icicle") {
            auto init = std::make_unique<Initializer_Mage_Icicle>(person.get(), m_deltaTime, m_fantasyConfig);
            init->Initialize();
        } else {
            auto init = std::make_unique<Initializer_Mage_Beam>(person.get(), m_deltaTime, m_fantasyConfig);
            init->Initialize();
        }

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
// MainWindow 实现
//==============================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_workerThread(nullptr), m_worker(nullptr)
{
    setupUI();
    connect(m_professionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProfessionChanged);
    connect(m_runButton, &QPushButton::clicked, this, &MainWindow::onRunClicked);

    // 初始化职业默认值
    m_defaults["beam"] = {
        {"primaryAttr", 6000}, {"crit", 5.00}, {"quickness", 32.88}, {"lucky", 5.00}, {"proficient", 34.09}, {"almighty", 16.00}, {"atk", 4000}, {"refineAtk", 800}, {"elementAtk", 40}, {"attackSpeed", 0.00}, {"castingSpeed", 0.00}, {"critDmgSet", 0}, {"incSet", 0}, {"eleIncSet", 0}, {"times", 20}, {"maxTime", 18000}, {"deltaTime", 1}, {"seed", 42}};
    m_defaults["icicle"] = {
        {"primaryAttr", 4593}, {"crit", 36.00}, {"quickness", 1.05}, {"lucky", 51.70}, {"proficient", 6.00}, {"almighty", 17.58}, {"atk", 3111}, {"refineAtk", 820}, {"elementAtk", 35}, {"attackSpeed", 10.00}, {"castingSpeed", 0.00}, {"critDmgSet", 0}, {"incSet", 0}, {"eleIncSet", 0}, {"times", 20}, {"maxTime", 18000}, {"deltaTime", 1}, {"seed", 42}};
    connect(m_professionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProfessionChanged);
    onProfessionChanged(0); // 初始化冰矛
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
    m_professionCombo->addItem("冰矛 · Icicle");
    m_professionCombo->addItem("射线 · Beam");
    layout->addWidget(m_professionCombo, row++, 1);

    // 1: 幻想配置
    layout->addWidget(new QLabel("幻想配置"), row, 0);
    m_fantasyCombo = new QComboBox;
    // 初始填充冰矛的选项（将在 onProfessionChanged 中动态更新）
    m_fantasyCombo->addItem("姆头 + 尖兵");
    m_fantasyCombo->addItem("姆头 + 博伊斯(仅三次释放)");
    m_fantasyCombo->addItem("姆头 + 伊戈雷乌斯(仅三次释放)");
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
    if (index == 0) { // 冰矛
        m_fantasyCombo->addItem("姆头 + 尖兵");
        m_fantasyCombo->addItem("姆头 + 博伊斯(仅三次释放)");
        m_fantasyCombo->addItem("姆头 + 伊戈雷乌斯(仅三次释放)");
        m_fantasyCombo->setCurrentIndex(0); // 默认姆头+尖兵
    } else { // 射线
        m_fantasyCombo->addItem("姆头 + 尖兵");
        m_fantasyCombo->addItem("姆头 + 伊戈雷乌斯");
        m_fantasyCombo->addItem("姆头 + 嗜血毛球");
        m_fantasyCombo->addItem("嗜血毛球 + 幻妖蟹蛛");
        m_fantasyCombo->addItem("尖兵 + 嗜血毛球");
        m_fantasyCombo->setCurrentIndex(0); // 默认尖兵（与之前保持一致）
    }
    QString prof = (index == 0) ? "icicle" : "beam";
    updateDefaultsForProfession(prof);
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

    appendLog(QString("[CONFIG] 已切换至 %1 默认参数").arg(prof == "icicle" ? "冰矛" : "射线"));
}

void MainWindow::onRunClicked()
{
    if (m_workerThread && m_workerThread->isRunning())
    {
        QMessageBox::warning(this, "模拟中", "已有模拟正在运行，请等待完成。");
        return;
    }

    // 读取所有输入值
    QString prof = m_professionCombo->currentIndex() == 0 ? "icicle" : "beam";
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
    int fantasyConfig = m_fantasyCombo->currentIndex(); // 获取幻想配置索引

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