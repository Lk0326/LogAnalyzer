
#include "MainWindow.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QWidget>
#include <QTextStream>
#include <QStringList>
#include <QLineEdit>
#include <QComboBox>
#include <QtConcurrent>
#include <QDateTimeEdit>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QMessageBox>
#include <QGridLayout>
#include <QSizePolicy>
#include <QHBoxLayout>
#include <QProgressBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    logWatcher = new QFutureWatcher<QStringList>(this);
    connect(logWatcher, &QFutureWatcher<QStringList>::finished,
            this, &MainWindow::onLogLoadingFinished);
}

void MainWindow::setupUi()
{
    setWindowTitle("LogAnalyzer - 日志分析工具");
    resize(1000, 650);

    // 创建中央窗口和布局
    QWidget *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    auto *buttonLayout = new QHBoxLayout();

    // 创建按钮
    openButton = new QPushButton("打开日志文件", central);
    auto *demoButton = new QPushButton("加载示例日志", central);
    auto *clearButton = new QPushButton("清空日志", central);
    auto *resetButton = new QPushButton("重置筛选", central);
    auto *exportButton = new QPushButton("导出筛选结果", central);

    buttonLayout->addWidget(openButton);
    buttonLayout->addWidget(demoButton);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addWidget(resetButton);
    buttonLayout->addWidget(exportButton);
    buttonLayout->addStretch();

    // 重置所有筛选条件
    connect(resetButton, &QPushButton::clicked, this, [this]() {
        searchEdit->clear();
        levelCombo->setCurrentIndex(0);

        startTimeEdit->setDateTime(
            QDateTime(QDate::currentDate(), QTime(0, 0, 0))
            );

        endTimeEdit->setDateTime(
            QDateTime(QDate::currentDate(), QTime(23, 59, 59))
            );

        searchLogs();
    });

    // 导出当前显示的筛选结果
    connect(exportButton, &QPushButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getSaveFileName(
            this,
            "导出筛选结果",
            QDateTime::currentDateTime().toString("'logs_'yyyyMMdd_HHmmss'.txt'"),
            "文本文件 (*.txt)"
            );

        if (filePath.isEmpty()) {
            return;
        }

        QFile file(filePath);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(
                this,
                "导出失败",
                "无法写入文件：" + file.errorString()
                );
            return;
        }

        QTextStream out(&file);

        out << logView->toPlainText();

        file.close();

        QMessageBox::information(
            this,
            "导出成功",
            "筛选结果已导出到：\n" + filePath
            );
    });




    // 创建筛选区域：两列布局
    auto *filterLayout = new QGridLayout();

    // 关键字
    filterLayout->addWidget(new QLabel("关键字：", central), 0, 0);

    searchEdit = new QLineEdit(central);
    searchEdit->setPlaceholderText("输入关键字搜索...");
    filterLayout->addWidget(searchEdit, 0, 1);

    connect(searchEdit, &QLineEdit::textChanged,
            this, &MainWindow::searchLogs);

    // 日志级别
    filterLayout->addWidget(new QLabel("日志级别：", central), 1, 0);

    levelCombo = new QComboBox(central);
    levelCombo->addItem("全部级别");
    levelCombo->addItem("INFO");
    levelCombo->addItem("WARN");
    levelCombo->addItem("ERROR");
    filterLayout->addWidget(levelCombo, 1, 1);

    connect(levelCombo, &QComboBox::currentTextChanged,
            this, &MainWindow::searchLogs);

    // 开始时间
    filterLayout->addWidget(new QLabel("开始时间：", central), 2, 0);

    startTimeEdit = new QDateTimeEdit(central);
    startTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    startTimeEdit->setCalendarPopup(true);
    startTimeEdit->setDateTime(
        QDateTime(QDate::currentDate(), QTime(0, 0, 0))
        );
    filterLayout->addWidget(startTimeEdit, 2, 1);

    connect(startTimeEdit, &QDateTimeEdit::dateTimeChanged,
            this, &MainWindow::searchLogs);

    // 结束时间
    filterLayout->addWidget(new QLabel("结束时间：", central), 3, 0);

    endTimeEdit = new QDateTimeEdit(central);
    endTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    endTimeEdit->setCalendarPopup(true);
    endTimeEdit->setDateTime(
        QDateTime(QDate::currentDate(), QTime(23, 59, 59))
        );
    filterLayout->addWidget(endTimeEdit, 3, 1);

    connect(endTimeEdit, &QDateTimeEdit::dateTimeChanged,
            this, &MainWindow::searchLogs);

    // 让右侧控件列占据多余的横向空间
    filterLayout->setColumnStretch(1, 1);

    mainLayout->addLayout(filterLayout);


    // 创建日志显示区域
    logView = new QTextEdit(central);
    logView->setReadOnly(true);
    logView->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
        );
    logView->document()->setMaximumBlockCount(5000);
    logView->setStyleSheet(
        "QTextEdit {"
        " background-color: #1E1E1E;"
        " color: #D1D5DB;"
        " font-family: Consolas;"
        " font-size: 12px;"
        "}"
        );

    statusLabel = new QLabel("就绪", central);
    // 显示当前日志文件名
    fileNameLabel = new QLabel("当前文件：未加载", central);
    fileNameLabel->setWordWrap(true);
    fileNameLabel->setStyleSheet(
        "QLabel {"
        "color: #4DA3FF;"
        "font-weight: bold;"
        "padding: 4px;"
        "}"
        );
    statsLabel = new QLabel("日志统计：等待加载", this);
    // 创建日志级别统计条
    infoBar = new QProgressBar(central);
    warnBar = new QProgressBar(central);
    errorBar = new QProgressBar(central);
    // 统一统计条范围为 0% 到 100%
    infoBar->setRange(0, 100);
    warnBar->setRange(0, 100);
    errorBar->setRange(0, 100);
    // 设置统计条颜色
    infoBar->setStyleSheet(
        "QProgressBar::chunk { background-color: #4DA3FF; }");

    warnBar->setStyleSheet(
        "QProgressBar::chunk { background-color: #FFD166; }");

    errorBar->setStyleSheet(
        "QProgressBar::chunk { background-color: #FF5555; }");

    // 设置统计条外观
    infoBar->setTextVisible(true);
    warnBar->setTextVisible(true);
    errorBar->setTextVisible(true);

    infoBar->setFixedHeight(22);
    warnBar->setFixedHeight(22);
    errorBar->setFixedHeight(22);

    // 设置统计条最小宽度
    infoBar->setMinimumWidth(200);
    warnBar->setMinimumWidth(200);
    errorBar->setMinimumWidth(200);



    infoBar->setStyleSheet(
        "QProgressBar {"
        "border: 1px solid #444444;"
        "border-radius: 5px;"
        "background-color: #2B2B2B;"
        "color: white;"
        "text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "background-color: #4DA3FF;"
        "border-radius: 4px;"
        "}"
        );

    warnBar->setStyleSheet(
        "QProgressBar {"
        "border: 1px solid #444444;"
        "border-radius: 5px;"
        "background-color: #2B2B2B;"
        "color: white;"
        "text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "background-color: #FFD166;"
        "border-radius: 4px;"
        "}"
        );

    errorBar->setStyleSheet(
        "QProgressBar {"
        "border: 1px solid #444444;"
        "border-radius: 5px;"
        "background-color: #2B2B2B;"
        "color: white;"
        "text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "background-color: #FF5555;"
        "border-radius: 4px;"
        "}"
        );
    // 添加鼠标悬停提示
    infoBar->setToolTip("INFO 日志占当前筛选结果的比例");
    warnBar->setToolTip("WARN 日志占当前筛选结果的比例");
    errorBar->setToolTip("ERROR 日志占当前筛选结果的比例");

    // 设置统计条显示格式
    infoBar->setFormat("INFO 占比：%p%");
    warnBar->setFormat("WARN 占比：%p%");
    errorBar->setFormat("ERROR 占比：%p%");

    // 初始值设为 0
    infoBar->setValue(0);
    warnBar->setValue(0);
    errorBar->setValue(0);

    mainLayout->addLayout(buttonLayout);

    // 创建日志级别颜色图例
    auto *legendLayout = new QHBoxLayout();

    auto *errorLabel = new QLabel("● ERROR：错误", central);
    errorLabel->setStyleSheet("color: #FF5555;");

    auto *warnLabel = new QLabel("● WARN：警告", central);
    warnLabel->setStyleSheet("color: #FFD166;");

    auto *infoLabel = new QLabel("● INFO：信息", central);
    infoLabel->setStyleSheet("color: #4DA3FF;");

    legendLayout->addWidget(errorLabel);
    legendLayout->addWidget(warnLabel);
    legendLayout->addWidget(infoLabel);
    legendLayout->addStretch();

    mainLayout->addLayout(legendLayout);

    mainLayout->addWidget(logView);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(fileNameLabel);
    mainLayout->addSpacing(4);
    mainLayout->addWidget(infoBar);
    mainLayout->addWidget(warnBar);
    mainLayout->addWidget(errorBar);
    mainLayout->addSpacing(4);
    mainLayout->addWidget(statsLabel);

    setCentralWidget(central);

    // 连接按钮与功能
    connect(openButton, &QPushButton::clicked,
            this, [this]() {
                openLogFile();
            });

    connect(demoButton, &QPushButton::clicked,
            this, [this]() {
                loadDemoLogs();
            });

    connect(clearButton, &QPushButton::clicked,
            this, [this]() {
                allLogs.clear();
                searchEdit->clear();
                levelCombo->setCurrentIndex(0);
                logView->clear();
                fileNameLabel->setText("当前文件：未加载");
                updateLogStats();
                statusLabel->setText("日志已清空");
            });
}

void MainWindow::appendLogLine(const QString &line)
{
    QString color = "#D1D5DB";

    if (line.contains("ERROR", Qt::CaseInsensitive)) {
        color = "#F87171";
    } else if (line.contains("WARN", Qt::CaseInsensitive)) {
        color = "#FBBF24";
    } else if (line.contains("INFO", Qt::CaseInsensitive)) {
        color = "#60A5FA";
    }

    // 转义特殊字符，避免日志内容被当作 HTML
    const QString safeLine = line.toHtmlEscaped();

    const QString html =
        QString("<span style='color:%1'>%2</span>")
            .arg(color, safeLine);

    logView->append(html);
}

void MainWindow::loadDemoLogs()
{
    logView->clear();
    allLogs.clear();
    fileNameLabel->setText("当前文件：示例日志");

    const QStringList logs = {
        "2026-10-25 10:00:01 INFO  Application started",
        "2026-10-25 10:00:02 INFO  Connected to server",
        "2026-10-25 10:00:03 WARN  Memory usage is high",
        "2026-10-25 10:00:04 ERROR Failed to read configuration",
        "2026-10-25 10:00:05 INFO  Retrying connection",
        "2026-10-25 10:00:06 ERROR Connection failed"
    };

    for (const QString &line : logs) {
        allLogs.append(line);
        appendLogLine(line);
    }

    updateLogStats();

    statusLabel->setText(
        QString("示例加载完成，共 %1 条日志").arg(logs.size())
        );
}


void MainWindow::openLogFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择日志文件",
        QString(),
        "日志文件 (*.log *.txt *.out);;所有文件 (*)"
        );

    // 用户取消选择时，不执行读取
    if (filePath.isEmpty())
    {
        return;
    }

    // 启动后台读取任务
    loadLogFileAsync(filePath);
}




void MainWindow::searchLogs()
{
    QString keyword = searchEdit->text().trimmed();
    QString level = levelCombo->currentText();

    logView->clear();

    int matchedCount = 0;
    int infoCount = 0;
    int warnCount = 0;
    int errorCount = 0;

    for (qsizetype i = 0; i < allLogs.size(); ++i)
    {
        const QString &line = allLogs.at(i);

        // 解析日志开头的时间
        if (line.size() < 19)
        {
            continue;
        }

        QDateTime logTime = QDateTime::fromString(
            line.left(19), "yyyy-MM-dd HH:mm:ss"
            );

        if (!logTime.isValid())
        {
            continue;
        }

        // 判断日志时间是否在选定范围内
        bool timeMatch =
            logTime >= startTimeEdit->dateTime() &&
            logTime <= endTimeEdit->dateTime();

        // 判断关键字和日志级别
        bool keywordMatch =
            line.contains(keyword, Qt::CaseInsensitive);

        bool levelMatch =
            (level == "全部级别") ||
            line.contains(level, Qt::CaseInsensitive);

        if (keywordMatch && levelMatch && timeMatch)
        {
            appendLogLine(line);
            ++matchedCount;

            if (line.contains("INFO", Qt::CaseInsensitive))
            {
                ++infoCount;
            }
            else if (line.contains("WARN", Qt::CaseInsensitive))
            {
                ++warnCount;
            }
            else if (line.contains("ERROR", Qt::CaseInsensitive))
            {
                ++errorCount;
            }
        }
    }

    // 更新当前筛选结果的统计文字
    statsLabel->setText(
        QString("筛选统计：总数 %1 条 | INFO %2 条 | WARN %3 条 | ERROR %4 条")
            .arg(matchedCount)
            .arg(infoCount)
            .arg(warnCount)
            .arg(errorCount)
        );

    // 更新统计条：显示各级日志占筛选结果的比例
    infoBar->setValue(
        matchedCount > 0 ? qRound(infoCount * 100.0 / matchedCount) : 0
        );
    warnBar->setValue(
        matchedCount > 0 ? qRound(warnCount * 100.0 / matchedCount) : 0
        );
    errorBar->setValue(
        matchedCount > 0 ? qRound(errorCount * 100.0 / matchedCount) : 0
        );

    statusLabel->setText(
        QString("筛选结果：%1 条").arg(matchedCount)
        );
}


void MainWindow::updateLogStats()
{
    int infoCount = 0;
    int warnCount = 0;
    int errorCount = 0;

    for (qsizetype i = 0; i < allLogs.size(); ++i)
    {
        const QString &line = allLogs.at(i);

        if (line.contains("INFO", Qt::CaseInsensitive))
        {
            ++infoCount;
        }
        else if (line.contains("WARN", Qt::CaseInsensitive))
        {
            ++warnCount;
        }
        else if (line.contains("ERROR", Qt::CaseInsensitive))
        {
            ++errorCount;
        }
    }


    statsLabel->setText(
        QString("日志统计：总数 %1 条 | INFO %2 条 | WARN %3 条 | ERROR %4 条")
            .arg(allLogs.size())
            .arg(infoCount)
            .arg(warnCount)
            .arg(errorCount)
        );
    // 更新日志级别统计条
    // 按全部日志数量计算占比
    int totalCount = allLogs.size();

    infoBar->setRange(0, 100);
    warnBar->setRange(0, 100);
    errorBar->setRange(0, 100);

    if (totalCount > 0)
    {
        infoBar->setValue(qRound(infoCount * 100.0 / totalCount));
        warnBar->setValue(qRound(warnCount * 100.0 / totalCount));
        errorBar->setValue(qRound(errorCount * 100.0 / totalCount));
    }
    else
    {
        infoBar->setValue(0);
        warnBar->setValue(0);
        errorBar->setValue(0);
    }

}

void MainWindow::onLogLoadingFinished()
{
    // 获取后台任务读取的日志
    QStringList logs = logWatcher->result();

    // 检查后台读取是否失败
    if (!logs.isEmpty() && logs.first().startsWith("读取失败："))
    {
        statusLabel->setText(logs.first());
        openButton->setEnabled(true);
        return;
    }

    // 重置搜索条件
    searchEdit->clear();
    levelCombo->setCurrentIndex(0);

    // 清空旧日志
    allLogs = logs;
    logView->clear();

    // 显示读取到的日志
    for (qsizetype i = 0; i < allLogs.size(); ++i)
    {
        appendLogLine(allLogs.at(i));
    }

    // 更新日志统计
    updateLogStats();

    // 更新状态栏
    statusLabel->setText(
        QString("日志加载完成，共 %1 条").arg(allLogs.size())
        );
    openButton->setEnabled(true);
}



void MainWindow::loadLogFileAsync(const QString &filePath)
{
    // 显示当前加载的文件名
    fileNameLabel->setText(
        "当前文件：" + QFileInfo(filePath).fileName()
        );
    // 如果上一次的后台任务还在运行，暂时不启动新任务
    if (logWatcher->isRunning())
    {
        statusLabel->setText("日志正在加载，请稍候...");
        return;
    }

    // 清空旧日志，提示用户正在加载
    logView->clear();
    statusLabel->setText("正在后台加载日志...");
    openButton->setEnabled(false);

    // 启动后台读取任务
    logWatcher->setFuture(QtConcurrent::run([filePath]()
     {
         QStringList logs;
         QFile file(filePath);

         if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
             {
             return QStringList{
                 "读取失败：" + file.errorString()
             };
             }

         QTextStream in(&file);
        constexpr int maxLines = 5000;
        int lineCount = 0;

         while (!in.atEnd() && lineCount < maxLines)
        {
             logs.append(in.readLine());
             ++lineCount;
        }

        return logs;
    }));
}
